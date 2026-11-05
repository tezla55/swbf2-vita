#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace rsc {

struct V3 { float x = 0, y = 0, z = 0; V3() {} V3(float a, float b, float c) : x(a), y(b), z(c) {} };
static inline V3 operator+(V3 a, V3 b) { return V3(a.x + b.x, a.y + b.y, a.z + b.z); }
static inline V3 operator-(V3 a, V3 b) { return V3(a.x - b.x, a.y - b.y, a.z - b.z); }
static inline V3 operator*(V3 a, float s) { return V3(a.x * s, a.y * s, a.z * s); }
static inline float Dot(V3 a, V3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static inline V3 Cross(V3 a, V3 b) { return V3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x); }
static inline float Len(V3 a) { return sqrtf(Dot(a, a)); }
static inline V3 Norm(V3 a) { const float l = Len(a); return l > 1e-8f ? a * (1.0f / l) : V3(0, 1, 0); }

static const float kRadius        = 0.57f;
static const float kLowerY        = 0.57f;
static const float kForward       = 0.15f;
static const float kGroundNy      = 0.6f;
static const float kGroundSep     = 0.1f;
static const float kSinkIn        = 0.02f;
static const float kMaxPush       = 2.0f;
static const float kSteepNy       = 0.7f;
static const float kGravity       = 18.0f;
static const float kAirGrace      = 1.0f;

static inline float UpperY(float scale, bool crouch = false)
{
    const float u = scale * (crouch ? 1.6f : 1.9f) - 0.57f;
    return u <= 0.57f ? 0.627f : u + 0.15f;
}

struct Cyl { V3 c; float r, halfH; };
struct Sph { V3 c; float r; };
struct Box { V3 c, h, ax, ay, az; };

struct CellList
{
    const uint32_t* b = nullptr; const uint32_t* e = nullptr;
    const uint32_t* begin() const { return b; }
    const uint32_t* end() const { return e; }
    size_t size() const { return (size_t)(e - b); }
    bool empty() const { return b == e; }
};
class CellGrid
{
public:
    void Add(int ix, int iz, uint32_t id) { pend_.push_back({ (int16_t)ix, (int16_t)iz, id }); dirty_ = true; }
    CellList Cell(int ix, int iz) const
    {
        if (dirty_) Freeze();
        CellList L;
        if (dense_)
        {
            const int x = ix - x0_, z = iz - z0_;
            if (x < 0 || z < 0 || x >= nx_ || z >= nz_) return L;
            const size_t c = (size_t)z * (size_t)nx_ + (size_t)x;
            L.b = ids_.data() + off_[c]; L.e = ids_.data() + off_[c + 1];
            return L;
        }
        const int64_t k = Key(ix, iz);
        const auto it = std::lower_bound(keys_.begin(), keys_.end(), k);
        if (it == keys_.end() || *it != k) return L;
        const size_t c = (size_t)(it - keys_.begin());
        L.b = ids_.data() + off_[c]; L.e = ids_.data() + off_[c + 1];
        return L;
    }
    size_t CellCount() const { if (dirty_) Freeze(); return cells_; }
    size_t IdCount() const { if (dirty_) Freeze(); return ids_.size(); }
    template <class F> void ForEachCell(F f) const
    {
        if (dirty_) Freeze();
        if (dense_) { for (int z = 0; z < nz_; ++z) for (int x = 0; x < nx_; ++x) { CellList L = Cell(x + x0_, z + z0_); if (!L.empty()) f(x + x0_, z + z0_, L); } }
        else for (size_t c = 0; c < keys_.size(); ++c) { CellList L; L.b = ids_.data() + off_[c]; L.e = ids_.data() + off_[c + 1]; f((int)(keys_[c] >> 32), (int)(int32_t)(uint32_t)keys_[c], L); }
    }
    void Freeze() const
    {
        dirty_ = false;

        std::vector<Rec> all;
        all.reserve(ids_.size() + pend_.size());
        if (!ids_.empty())
            ForEachCellFrozen([&](int ix, int iz, CellList L) { for (uint32_t id : L) all.push_back({ (int16_t)ix, (int16_t)iz, id }); });
        all.insert(all.end(), pend_.begin(), pend_.end());
        std::vector<Rec>().swap(pend_);
        ids_.clear(); off_.clear(); keys_.clear(); cells_ = 0;
        if (all.empty()) { dense_ = true; nx_ = nz_ = 0; off_.assign(1, 0); return; }
        int minX = all[0].ix, maxX = minX, minZ = all[0].iz, maxZ = minZ;
        for (const Rec& r : all) { minX = std::min<int>(minX, r.ix); maxX = std::max<int>(maxX, r.ix); minZ = std::min<int>(minZ, r.iz); maxZ = std::max<int>(maxZ, r.iz); }
        const int64_t span = (int64_t)(maxX - minX + 1) * (int64_t)(maxZ - minZ + 1);
        ids_.resize(all.size());
        if (span <= 1500000)
        {
            dense_ = true; x0_ = minX; z0_ = minZ; nx_ = maxX - minX + 1; nz_ = maxZ - minZ + 1;
            off_.assign((size_t)span + 1, 0);
            for (const Rec& r : all) ++off_[(size_t)(r.iz - z0_) * (size_t)nx_ + (size_t)(r.ix - x0_) + 1];
            for (size_t c = 0; c < (size_t)span; ++c) { if (off_[c + 1]) ++cells_; off_[c + 1] += off_[c]; }
            std::vector<uint32_t> pos(off_.begin(), off_.end() - 1);
            for (const Rec& r : all) ids_[pos[(size_t)(r.iz - z0_) * (size_t)nx_ + (size_t)(r.ix - x0_)]++] = r.id;
        }
        else
        {
            dense_ = false;
            std::vector<uint32_t> order(all.size());
            for (size_t i = 0; i < order.size(); ++i) order[i] = (uint32_t)i;
            std::stable_sort(order.begin(), order.end(), [&](uint32_t a, uint32_t b) { return Key(all[a].ix, all[a].iz) < Key(all[b].ix, all[b].iz); });
            for (size_t i = 0; i < order.size(); ++i)
            {
                const Rec& r = all[order[i]]; const int64_t k = Key(r.ix, r.iz);
                if (keys_.empty() || keys_.back() != k) { keys_.push_back(k); off_.push_back((uint32_t)i); }
                ids_[i] = r.id;
            }
            off_.push_back((uint32_t)ids_.size());
            cells_ = keys_.size();
        }
    }
    static int64_t Key(int ix, int iz) { return ((int64_t)ix << 32) ^ (int64_t)(uint32_t)iz; }
private:
    struct Rec { int16_t ix, iz; uint32_t id; };
    template <class F> void ForEachCellFrozen(F f) const
    {
        if (dense_) { for (int z = 0; z < nz_; ++z) for (int x = 0; x < nx_; ++x) { const size_t c = (size_t)z * (size_t)nx_ + (size_t)x; if (off_[c + 1] > off_[c]) { CellList L; L.b = ids_.data() + off_[c]; L.e = ids_.data() + off_[c + 1]; f(x + x0_, z + z0_, L); } } }
        else for (size_t c = 0; c < keys_.size(); ++c) { CellList L; L.b = ids_.data() + off_[c]; L.e = ids_.data() + off_[c + 1]; f((int)(keys_[c] >> 32), (int)(int32_t)(uint32_t)keys_[c], L); }
    }
    mutable std::vector<Rec> pend_;
    mutable std::vector<uint32_t> ids_, off_;
    mutable std::vector<int64_t> keys_;
    mutable bool dirty_ = false, dense_ = true;
    mutable int x0_ = 0, z0_ = 0, nx_ = 0, nz_ = 0;
    mutable size_t cells_ = 0;
};

struct World
{
    std::vector<V3> tri;
    std::vector<Cyl> cyl;
    std::vector<Sph> sph;
    std::vector<Box> box;

    int32_t curObj = -1;
    std::vector<int32_t> triObj, boxObj;
    std::vector<uint8_t> objOff;
    bool Off(int32_t o) const { return o >= 0 && (size_t)o < objOff.size() && objOff[(size_t)o] != 0; }
    void SetObjectCollision(int32_t o, bool on) { if (o < 0) return; if ((size_t)o >= objOff.size()) objOff.resize((size_t)o + 1, 0); objOff[(size_t)o] = on ? 0 : 1; }
    float cell = 4.0f;
    CellGrid grid;
    static int64_t Key(int ix, int iz) { return ((int64_t)ix << 32) ^ (int64_t)(uint32_t)iz; }
    void Insert(uint32_t id, float x0, float z0, float x1, float z1)
    {
        const int ax = (int)floorf(x0 / cell), az = (int)floorf(z0 / cell), bx = (int)floorf(x1 / cell), bz = (int)floorf(z1 / cell);
        if ((int64_t)(bx - ax + 1) * (bz - az + 1) > 4096) return;
        for (int ix = ax; ix <= bx; ++ix) for (int iz = az; iz <= bz; ++iz) grid.Add(ix, iz, id);
    }

    void BeginBatch() {}
    void EndBatch() { grid.Freeze(); }
    void AddTri(V3 a, V3 b, V3 c)
    {
        if (Len(Cross(b - a, c - a)) < 1e-6f) return;
        const uint32_t id = (uint32_t)(tri.size() / 3);
        tri.push_back(a); tri.push_back(b); tri.push_back(c); triObj.push_back(curObj);
        Insert(id, fminf(a.x, fminf(b.x, c.x)), fminf(a.z, fminf(b.z, c.z)), fmaxf(a.x, fmaxf(b.x, c.x)), fmaxf(a.z, fmaxf(b.z, c.z)));
    }

    void AddBox(V3 c, V3 h, V3 ax, V3 ay, V3 az)
    {
        V3 p[8];
        for (int i = 0; i < 8; ++i)
            p[i] = c + ax * ((i & 1) ? h.x : -h.x) + ay * ((i & 2) ? h.y : -h.y) + az * ((i & 4) ? h.z : -h.z);
        static const int f[6][4] = { {0,1,3,2}, {4,6,7,5}, {0,4,5,1}, {2,3,7,6}, {0,2,6,4}, {1,5,7,3} };
        for (int i = 0; i < 6; ++i) { AddTri(p[f[i][0]], p[f[i][1]], p[f[i][2]]); AddTri(p[f[i][0]], p[f[i][2]], p[f[i][3]]); }
        AddSolidBox(c, h, ax, ay, az);
    }

    void AddSolidBox(V3 c, V3 h, V3 ax, V3 ay, V3 az)
    {
        box.push_back({ c, h, ax, ay, az }); boxObj.push_back(curObj);
        const float ex = fabsf(ax.x) * h.x + fabsf(ay.x) * h.y + fabsf(az.x) * h.z;
        const float ez = fabsf(ax.z) * h.x + fabsf(ay.z) * h.y + fabsf(az.z) * h.z;
        Insert(0xC0000000u | (uint32_t)(box.size() - 1), c.x - ex, c.z - ez, c.x + ex, c.z + ez);
    }
    void AddSolidBoxCorners(const V3 T[4], const V3 B[4])
    {
        V3 c(0, 0, 0); for (int k = 0; k < 4; ++k) c = c + T[k] + B[k]; c = c * 0.125f;
        const V3 ex = T[1] - T[0], ez = T[3] - T[0], ey = T[0] - B[0];
        const float lx = Len(ex), ly = Len(ey), lz = Len(ez);
        if (lx < 1e-4f || ly < 1e-4f || lz < 1e-4f) return;
        AddSolidBox(c, V3(lx * 0.5f, ly * 0.5f, lz * 0.5f), ex * (1.0f / lx), ey * (1.0f / ly), ez * (1.0f / lz));
    }
    void AddCyl(V3 c, float r, float halfH)
    {
        cyl.push_back({ c, r, halfH });
        Insert(0x40000000u | (uint32_t)(cyl.size() - 1), c.x - r, c.z - r, c.x + r, c.z + r);
    }
    void AddSph(V3 c, float r)
    {
        sph.push_back({ c, r });
        Insert(0x80000000u | (uint32_t)(sph.size() - 1), c.x - r, c.z - r, c.x + r, c.z + r);
    }
};

typedef bool (*TerrainFn)(void* user, float x, float z, float& y, V3& n);

struct Contact { V3 n; float depth; };
static inline bool& CeilingHit() { static bool s = false; return s; }
static inline float& CeilingDepth() { static float s = 0.0f; return s; }
static inline uint32_t& DeepestId() { static uint32_t s = 0; return s; }
static inline int& BoxInsideHits() { static int s = 0; return s; }
static inline int& BoxInsideDown() { static int s = 0; return s; }

static inline V3 ClosestOnTri(V3 p, V3 a, V3 b, V3 c)
{
    const V3 ab = b - a, ac = c - a, ap = p - a;
    const float d1 = Dot(ab, ap), d2 = Dot(ac, ap);
    if (d1 <= 0 && d2 <= 0) return a;
    const V3 bp = p - b; const float d3 = Dot(ab, bp), d4 = Dot(ac, bp);
    if (d3 >= 0 && d4 <= d3) return b;
    const float vc = d1 * d4 - d3 * d2;
    if (vc <= 0 && d1 >= 0 && d3 <= 0) return a + ab * (d1 / (d1 - d3));
    const V3 cp = p - c; const float d5 = Dot(ab, cp), d6 = Dot(ac, cp);
    if (d6 >= 0 && d5 <= d6) return c;
    const float vb = d5 * d2 - d1 * d6;
    if (vb <= 0 && d2 >= 0 && d6 <= 0) return a + ac * (d2 / (d2 - d6));
    const float va = d3 * d6 - d5 * d4;
    if (va <= 0 && (d4 - d3) >= 0 && (d5 - d6) >= 0) return b + (c - b) * ((d4 - d3) / ((d4 - d3) + (d5 - d6)));
    const float den = 1.0f / (va + vb + vc);
    return a + ab * (vb * den) + ac * (vc * den);
}

static bool DeepestContact(const World& w, V3 s, float r, TerrainFn tf, void* tu, Contact& out, const std::vector<uint32_t>** cache = nullptr)
{
    bool any = false; float best = 0.0f; uint32_t curId = 0xffffffffu;

    auto consider = [&](V3 n, float depth)
    {
        if (n.y < 0.0f && n.x * n.x + n.z * n.z < 1e-6f) { if (depth > 0) { CeilingHit() = true; if (depth > CeilingDepth()) CeilingDepth() = depth; } return; }
        if (depth > best) { best = depth; out.n = n; out.depth = depth; any = true; DeepestId() = curId; }
    };
    const int ax = (int)floorf((s.x - r) / w.cell), bx = (int)floorf((s.x + r) / w.cell);
    const int az = (int)floorf((s.z - r) / w.cell), bz = (int)floorf((s.z + r) / w.cell);
    for (int ix = ax; ix <= bx; ++ix) for (int iz = az; iz <= bz; ++iz)
    {
        const CellList cellIds = w.grid.Cell(ix, iz);
        for (uint32_t id : cellIds)
        {
            curId = id;
            if ((id & 0xC0000000u) == 0xC0000000u)
            {
                if (w.Off(w.boxObj[id & 0x3fffffffu])) continue;

                const Box& q = w.box[id & 0x3fffffffu];
                const V3 d = s - q.c;
                const float lx = Dot(d, q.ax), ly = Dot(d, q.ay), lz = Dot(d, q.az);
                const float px = q.h.x - fabsf(lx), py = q.h.y - fabsf(ly), pz = q.h.z - fabsf(lz);
                if (px > 0 && py > 0 && pz > 0)
                {
                    V3 n; float pen;
                    if (py <= px && py <= pz) { n = q.ay * (ly >= 0 ? 1.0f : -1.0f); pen = py; }
                    else if (px <= pz)        { n = q.ax * (lx >= 0 ? 1.0f : -1.0f); pen = px; }
                    else                      { n = q.az * (lz >= 0 ? 1.0f : -1.0f); pen = pz; }
                    ++BoxInsideHits(); if (n.y < -0.5f) ++BoxInsideDown();
                    consider(n, pen + r);
                }
                continue;
            }
            if (id & 0x80000000u)
            {
                const Sph& q = w.sph[id & 0x3fffffffu];
                const V3 d = s - q.c; const float l = Len(d), rr = r + q.r;
                if (l < rr && l > 1e-6f) consider(d * (1.0f / l), rr - l);
            }
            else if (id & 0x40000000u)
            {
                const Cyl& q = w.cyl[id & 0x3fffffffu];
                const float cy = fmaxf(q.c.y - q.halfH, fminf(q.c.y + q.halfH, s.y));
                float dx = s.x - q.c.x, dz = s.z - q.c.z; const float hl = sqrtf(dx * dx + dz * dz);
                V3 cp;
                if (hl <= q.r) cp = V3(s.x, cy, s.z);
                else cp = V3(q.c.x + dx / hl * q.r, cy, q.c.z + dz / hl * q.r);
                V3 d = s - cp; float l = Len(d);
                if (l < 1e-6f) { d = (s.y >= q.c.y) ? V3(0, 1, 0) : V3(0, -1, 0); l = 0.0f; } else d = d * (1.0f / l);
                if (l < r) consider(d, r - l);
            }
            else
            {
                if (!w.objOff.empty() && w.Off(w.triObj[id])) continue;
                const V3 a = w.tri[id * 3], b = w.tri[id * 3 + 1], c = w.tri[id * 3 + 2];
                if (s.y + r < fminf(a.y, fminf(b.y, c.y)) || s.y - r > fmaxf(a.y, fmaxf(b.y, c.y))) continue;
                const V3 cp = ClosestOnTri(s, a, b, c);
                const V3 d = s - cp; const float l = Len(d);
                if (l >= r) continue;
                V3 n;
                if (l > 1e-5f) n = d * (1.0f / l);
                else { n = Norm(Cross(b - a, c - a)); if (n.y < 0) n = n * -1.0f; }
                consider(n, r - l);
            }
        }
    }
    if (tf)
    {
        float ty; V3 tn;
        if (tf(tu, s.x, s.z, ty, tn))
        {

            const float dist = (s.y - ty) * tn.y;
            if (dist < r) consider(tn, r - dist);
        }
    }
    (void)cache;
    return any;
}

struct Body
{
    V3 pos;
    V3 vel;
    V3 groundN = V3(0, 1, 0);
    bool grounded = false;
    float airTime = 0.0f;

    V3 wallPush = V3(0, 0, 0);
};

inline bool& LastGroundUpperOnly() { static bool v = false; return v; }
inline bool& UpperNeverGrounds() { static bool v = false; return v; }

static int ResolveContacts(const World& w, Body& b, float facingX, float facingZ, float scale, TerrainFn tf, void* tu, int iters = 4, bool crouch = false)
{
    const float up = UpperY(scale, crouch);
    int hits = 0;
    bool ground = false, groundLower = false; V3 bestN(0, 1, 0);
    CeilingHit() = false; CeilingDepth() = 0.0f;
    const bool rising = b.vel.y > 0.0f;
    for (int it = 0; it < iters; ++it)
    {
        const V3 fo(facingX * kForward, 0, facingZ * kForward);
        Contact cl, cu; cl.depth = cu.depth = 0;
        const bool hl = DeepestContact(w, b.pos + fo + V3(0, kLowerY, 0), kRadius, tf, tu, cl);
        const bool hu = DeepestContact(w, b.pos + fo + V3(0, up, 0), kRadius, tf, tu, cu);
        if (!hl && !hu) break;
        bool useLower = hl && (!hu || cl.depth >= cu.depth);

        if (!useLower && cu.n.y > kGroundNy && UpperNeverGrounds()) { if (hl) useLower = true; else break; }
        const Contact c = useLower ? cl : cu;
        ++hits;
        V3 n = c.n; float depth = c.depth;
        if (depth > kMaxPush) break;
        if (n.y > kGroundNy)
        {
            if (!ground || n.y > bestN.y) bestN = n;
            ground = true; if (useLower) groundLower = true;

            const float lift = depth / n.y - kSinkIn;
            if (lift <= 0.0f) break;
            b.pos.y += lift;
            const float vn = Dot(b.vel, n);
            if (vn < 0) b.vel = b.vel - n * vn;
            continue;
        }

        const float h = sqrtf(n.x * n.x + n.z * n.z);
        if (h < 1e-3f)
        {

            if (b.vel.y > 0) b.vel.y = 0;
            break;
        }
        const bool wallFace = fabsf(c.n.y) < 0.3f;
        depth /= h; n = V3(n.x / h, 0, n.z / h);
        if (depth > kMaxPush) break;
        if (wallFace) b.wallPush = b.wallPush + n;
        b.pos = b.pos + n * depth;
        const float vn = Dot(b.vel, n);
        if (vn < 0) b.vel = b.vel - n * vn;
    }
    if (!ground && b.vel.y <= 0.0f)
    {

        const V3 fo(facingX * kForward, kLowerY, facingZ * kForward);
        Contact c;
        if (DeepestContact(w, b.pos + fo, kRadius + kGroundSep, tf, tu, c) && c.n.y > kGroundNy)
        { ground = true; groundLower = true; bestN = c.n; }
    }
    if (CeilingHit() && rising)
    {

        b.pos.y -= CeilingDepth();
        b.vel.y = 0;
    }
    b.grounded = ground;
    if (ground) b.groundN = bestN;
    LastGroundUpperOnly() = ground && !groundLower;
    return hits;
}

static bool LiftOutOfFloor(const World& w, Body& b, float fx, float fz, float scale, bool crouch, TerrainFn tf = nullptr, void* tu = nullptr)
{
    const Body before = b;
    b.pos.y += 1.3f; b.vel.y = 0.0f; b.grounded = false;
    for (int k = 0; k < 7 && !b.grounded; ++k) { b.pos.y -= 0.2f; ResolveContacts(w, b, fx, fz, scale, tf, tu, 4, crouch); }
    if (b.grounded && !LastGroundUpperOnly() && b.pos.y > before.pos.y + 0.2f && b.pos.y < before.pos.y + 1.35f) return true;
    b = before; LastGroundUpperOnly() = true;
    return false;
}

static bool FollowGround(const World& w, Body& b, float travelH, float facingX, float facingZ, float scale, TerrainFn tf, void* tu, bool crouch = false)
{

    if (b.grounded || b.vel.y > 0.5f || b.airTime > 0.3f) return false;
    const float maxDrop = travelH * 1.3333f + 0.1f;
    const V3 fo(facingX * kForward, kLowerY, facingZ * kForward);
    for (float dy = 0.05f; dy <= maxDrop + 1e-4f; dy += 0.05f)
    {
        Contact c;
        if (DeepestContact(w, b.pos + fo - V3(0, dy, 0), kRadius, tf, tu, c) && c.n.y > kGroundNy)
        {
            b.pos.y -= dy;
            ResolveContacts(w, b, facingX, facingZ, scale, tf, tu, 4, crouch);
            if (!b.grounded) return false;
            b.vel = b.vel - b.groundN * Dot(b.vel, b.groundN);
            return true;
        }
    }
    return false;
}

inline uint32_t& StepSubCount() { static uint32_t v = 0; return v; }
inline float& StepMaxLen() { static float v = 0.0f; return v; }

static bool StepMove(const World& w, Body& b, V3 start, float dx, float dz, float dt, float fx, float fz, float scale, bool crouch,
                     V3* outPre = nullptr, int* outRedo = nullptr, TerrainFn tf = nullptr, void* tu = nullptr)
{
    float travel = sqrtf(dx * dx + dz * dz);
    { const float vy0 = b.grounded ? 0.0f : fabsf(b.vel.y) * dt; const float l0 = sqrtf(travel * travel + vy0 * vy0); if (l0 > StepMaxLen()) StepMaxLen() = l0; }

    if (travel > 3.0f) { start.x += dx; start.z += dz; dx = dz = 0.0f; travel = 0.0f; }
    const float vyStep = b.grounded ? 0.0f : fabsf(b.vel.y - kGravity * dt) * dt;
    const float stepLen = sqrtf(travel * travel + vyStep * vyStep);
    const int nSub = (stepLen > 0.5f) ? (int)fminf(8.0f, ceilf(stepLen / 0.5f)) : 1;
    if (outRedo && nSub > 1) *outRedo += nSub - 1;
    StepSubCount() += (uint32_t)nSub;
    const float sdt = dt / (float)nSub, sdx = dx / (float)nSub, sdz = dz / (float)nSub;
    V3 pre = b.pos;
    bool upperOnly = false;
    b.pos = start;
    for (int s = 0; s < nSub; ++s)
    {
        b.pos.x += sdx; b.pos.z += sdz;
        if (b.grounded && b.groundN.y > 0.05f)
            b.pos.y += -(b.groundN.x * sdx + b.groundN.z * sdz) / b.groundN.y;
        else if (sdt > 0.0f)
        {
            b.vel.y -= kGravity * sdt;
            b.pos.y += b.vel.y * sdt;
        }
        if (dt > 0.0f) { b.vel.x = dx / dt; b.vel.z = dz / dt; }
        pre = b.pos;
        ResolveContacts(w, b, fx, fz, scale, tf, tu, 4, crouch);
        upperOnly = LastGroundUpperOnly();
        FollowGround(w, b, travel / (float)nSub, fx, fz, scale, tf, tu, crouch);
    }
    if (outPre) *outPre = pre;
    return upperOnly;
}

static void GroundMove(Body& b, float dt, float fwdX, float fwdZ, float wishX, float wishZ, float accel, bool applyGravity = true)
{
    const V3 n = b.grounded ? b.groundN : V3(0, 1, 0);
    if (b.grounded) { const float vn = Dot(b.vel, n); if (vn < 0) b.vel = b.vel - n * vn; }
    const V3 fwd(fwdX, 0, fwdZ), right(fwdZ, 0, -fwdX);
    float thrust = wishX * fwd.x + wishZ * fwd.z, strafe = wishX * right.x + wishZ * right.z;
    V3 tF = Norm(Cross(right, n)); if (Dot(tF, fwd) < 0) tF = tF * -1.0f;
    V3 tR = Norm(Cross(n, tF));    if (Dot(tR, right) < 0) tR = tR * -1.0f;
    float a = accel;
    if (b.grounded && n.y < kSteepNy) a *= 0.1f;
    const float maxDv = a * dt;
    float biasF = 0, biasR = 0;
    if (b.grounded)
    {
        const float g = 1.0f * dt * kGravity;
        biasF = -tF.y * g; biasR = -tR.y * g;
        const float upc = tF.y * thrust + tR.y * strafe;
        const float w2 = thrust * thrust + strafe * strafe;
        if (upc > 0 && w2 > 1e-6f) { const float k = 1.0f - (upc * upc) / w2; thrust *= k; strafe *= k; }
    }
    float dF = thrust - Dot(b.vel, tF); dF = fmaxf(biasF - maxDv, fminf(biasF + maxDv, dF));
    float dR = strafe - Dot(b.vel, tR); dR = fmaxf(biasR - maxDv, fminf(biasR + maxDv, dR));
    b.vel = b.vel + tF * dF + tR * dR;
    if (!b.grounded && applyGravity) b.vel.y -= kGravity * dt;
    b.pos = b.pos + b.vel * dt;
    b.airTime = b.grounded ? 0.0f : b.airTime + dt;
}

}
