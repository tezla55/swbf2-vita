#pragma once
#include <cmath>
#include <functional>
#include <utility>
#include <vector>

namespace rbl
{

struct Box { float cx, cz, ux, uz, vx, vz, hx, hz; };

using ForEach = std::function<void(float, float, float, float, float, const std::function<void(const Box&)>&)>;

inline bool Inside(const Box& b, float x, float z, float r)
{
    const float px = x - b.cx, pz = z - b.cz;
    return std::fabs(px * b.ux + pz * b.uz) <= b.hx + r && std::fabs(px * b.vx + pz * b.vz) <= b.hz + r;
}

inline float SegmentEnters(const Box& b, float x0, float z0, float x1, float z1, float r)
{
    const float ax = (x0 - b.cx) * b.ux + (z0 - b.cz) * b.uz, az = (x0 - b.cx) * b.vx + (z0 - b.cz) * b.vz;
    const float bx = (x1 - b.cx) * b.ux + (z1 - b.cz) * b.uz, bz = (x1 - b.cx) * b.vx + (z1 - b.cz) * b.vz;
    const float h[2] = { b.hx + r, b.hz + r }, o[2] = { ax, az }, d[2] = { bx - ax, bz - az };
    if (std::fabs(ax) <= h[0] && std::fabs(az) <= h[1]) return -1.0f;
    float t0 = 0.0f, t1 = 1.0f;
    for (int k = 0; k < 2; ++k)
    {
        if (std::fabs(d[k]) < 1e-9f) { if (std::fabs(o[k]) > h[k]) return -1.0f; continue; }
        float ta = (-h[k] - o[k]) / d[k], tb = (h[k] - o[k]) / d[k];
        if (ta > tb) std::swap(ta, tb);
        t0 = std::fmax(t0, ta); t1 = std::fmin(t1, tb);
        if (t0 > t1) return -1.0f;
    }
    return t0;
}

inline bool NearestOnSegment(const ForEach& each, float x0, float z0, float x1, float z1, float r, Box& out)
{
    float best = 2.0f; bool hit = false;
    each(x0, z0, x1, z1, r + 1.0f, [&](const Box& b) {
        const float t = SegmentEnters(b, x0, z0, x1, z1, r);
        if (t >= 0.0f && t < best) { best = t; out = b; hit = true; }
    });
    return hit;
}

inline bool PointBlocked(const ForEach& each, float x, float z, float r)
{
    bool in = false;
    each(x, z, x, z, r + 1.0f, [&](const Box& b) { if (!in && Inside(b, x, z, r)) in = true; });
    return in;
}

inline int Silhouette(const Box& b, float e, float px, float pz, float qx, float qz, float out[4])
{
    float dx = qx - px, dz = qz - pz; const float dl = std::sqrt(dx * dx + dz * dz); if (dl < 1e-6f) return 0;
    dx /= dl; dz /= dl;
    float bestDot[2] = { 2.0f, 2.0f }, c[2][2] = {};
    for (int i = 0; i < 4; ++i)
    {
        const float sa = (i == 0 || i == 1) ? 1.0f : -1.0f, sb = (i == 0 || i == 3) ? 1.0f : -1.0f;
        const float kx = b.cx + b.ux * sa * (b.hx + e) + b.vx * sb * (b.hz + e);
        const float kz = b.cz + b.uz * sa * (b.hx + e) + b.vz * sb * (b.hz + e);
        float vx = kx - px, vz = kz - pz; const float vl = std::sqrt(vx * vx + vz * vz); if (vl < 1e-6f) continue;
        vx /= vl; vz /= vl;
        const int side = (dx * vz - dz * vx) < 0.0f ? 0 : 1;
        const float dot = dx * vx + dz * vz;
        if (dot < bestDot[side]) { bestDot[side] = dot; c[side][0] = kx; c[side][1] = kz; }
    }
    int n = 0;
    for (int s = 0; s < 2; ++s) if (bestDot[s] < 2.0f) { out[n * 2] = c[s][0]; out[n * 2 + 1] = c[s][1]; ++n; }
    return n;
}

enum Result { STRAIGHT = 0, CORNERS = 1, GOAL_BLOCKED = 2, NO_ROUTE = 3 };

inline Result PlanLeg(const ForEach& each, float sx, float sz, float gx, float gz, float r, std::vector<float>& corners)
{
    corners.clear();
    Box hitBox;
    if (!NearestOnSegment(each, sx, sz, gx, gz, r, hitBox)) return STRAIGHT;
    if (PointBlocked(each, gx, gz, r)) return GOAL_BLOCKED;
    struct Node { float x, z, g; int parent; bool closed; };
    std::vector<Node> nodes; nodes.reserve(64);
    nodes.push_back({ sx, sz, 0.0f, -1, false });
    float px = gz - sz, pz = -(gx - sx); { const float l = std::sqrt(px * px + pz * pz); px /= l; pz /= l; }
    const float band = std::fmax(25.0f, std::sqrt((gx - sx) * (gx - sx) + (gz - sz) * (gz - sz)));
    const float e = r + 0.5f;
    std::vector<std::pair<float, float>> visited;
    std::function<void(int, float, float, int)> expand = [&](int parent, float x, float z, int depth)
    {
        if (depth >= 32 || nodes.size() >= 1024) return;
        if (std::fabs(px * (x - gx) + pz * (z - gz)) > band) return;
        for (const auto& v : visited) if ((v.first - x) * (v.first - x) + (v.second - z) * (v.second - z) < 1e-8f) return;
        if (visited.size() < 100) visited.push_back({ x, z });
        const float ppx = nodes[(size_t)parent].x, ppz = nodes[(size_t)parent].z, pg = nodes[(size_t)parent].g;
        Box b;
        if (!NearestOnSegment(each, ppx, ppz, x, z, r, b))
        {
            const float g = pg + std::sqrt((x - ppx) * (x - ppx) + (z - ppz) * (z - ppz));
            for (Node& o : nodes)
                if ((o.x - x) * (o.x - x) + (o.z - z) * (o.z - z) < 1e-8f)
                {
                    if (!o.closed && g < o.g) { o.g = g; o.parent = parent; }
                    return;
                }
            nodes.push_back({ x, z, g, parent, false });
            return;
        }
        float c[4]; const int n = Silhouette(b, e, ppx, ppz, x, z, c);
        for (int k = 0; k < n; ++k) expand(parent, c[k * 2], c[k * 2 + 1], depth + 1);
    };
    for (int iter = 0; iter < 512; ++iter)
    {
        int best = -1; float bestF = 1e30f;
        for (size_t i = 0; i < nodes.size(); ++i)
        {
            if (nodes[i].closed) continue;
            const float f = nodes[i].g + std::sqrt((gx - nodes[i].x) * (gx - nodes[i].x) + (gz - nodes[i].z) * (gz - nodes[i].z));
            if (f < bestF) { bestF = f; best = (int)i; }
        }
        if (best < 0) break;
        const float bx = nodes[(size_t)best].x, bz = nodes[(size_t)best].z;
        if ((bx - gx) * (bx - gx) + (bz - gz) * (bz - gz) < 1e-8f)
        {
            std::vector<float> rev;
            for (int k = nodes[(size_t)best].parent; k > 0; k = nodes[(size_t)k].parent) { rev.push_back(nodes[(size_t)k].x); rev.push_back(nodes[(size_t)k].z); }
            for (size_t i = rev.size(); i >= 2; i -= 2) { corners.push_back(rev[i - 2]); corners.push_back(rev[i - 1]); }
            return CORNERS;
        }
        nodes[(size_t)best].closed = true;
        visited.clear();
        expand(best, gx, gz, 0);
    }
    return NO_ROUTE;
}
}
