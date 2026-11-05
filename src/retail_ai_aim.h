#pragma once
#include <cmath>

namespace retail_ai {

inline float Lerp(float a, float b, float f) { return (b - a) * f + a; }
inline float SkillF(int skill) { if (skill < 1) skill = 1; if (skill > 20) skill = 20; return (float)(skill - 1) * 0.05263158f; }

inline float BubbleK(float d, float divider, float clampK, bool targetCarriesHuman, bool shooterWithHuman)
{
    if (shooterWithHuman) return 0.0f;
    float k = d / divider + 0.5f;
    if (divider * 0.5f <= d) { if (k < 1.0f) k = 1.0f; if (k > clampK) k = clampK; }
    return targetCarriesHuman ? k : k * 0.3f;
}

inline float SpreadMult(float skillF) { return Lerp(3.0f, 1.0f, skillF); }
inline int   BurstCount(int bmin, int bmax, float u01, float skillF)
{
    const int span = bmax - bmin;
    int n = (int)((float)bmin + u01 * (float)(span > 0 ? span : 0) * Lerp(0.0f, 1.0f, skillF));
    return n < 2 ? 2 : n;
}
inline float BurstPause(float barrageDelay, float skillF) { return barrageDelay * (1.6f - Lerp(0.1f, 0.9f, skillF)); }
inline float SingleRearm(float u01, float skillF)
{
    const float v = Lerp(0.1f, 0.9f, skillF), r = 1.0f - v;
    return (0.2f + 1.8f * u01) * (r > 0.0f ? r : 0.0f);
}

inline float TrackAccel(float skillF) { return Lerp(0.8f, 14.0f, skillF * skillF * skillF); }
inline float TrackDamp(float skillF) { return Lerp(0.8f, 2.4f, skillF); }
inline bool  IsWeaponCannonLabel(const char* l)
{
    const char* k[] = { "cannon", "launcher", "grapplinghook", "towcable" };
    for (const char* s : k) { const char* a = l; const char* b = s; while (*a && *b && ((*a | 0x20) == *b)) { ++a; ++b; } if (!*a && !*b) return true; }
    return false;
}

struct V3 { float x, y, z; };
inline float Dot(V3 a, V3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline V3 Norm(V3 a) { const float l = std::sqrt(Dot(a, a)); return l > 1e-12f ? V3{ a.x / l, a.y / l, a.z / l } : a; }

inline void TrackStep(V3& cur, V3& vel, V3 want, float dt, float A, float D)
{
    const float c30 = 0.8660254f, s30 = 0.5f;
    const float d = Dot(cur, want);
    if (d < c30)
    {
        V3 p{ cur.x - want.x * d, cur.y - want.y * d, cur.z - want.z * d };
        if (Dot(p, p) < 1e-12f) p = std::fabs(want.y) < 0.9f ? V3{ -want.z, 0.0f, want.x } : V3{ 1.0f, 0.0f, 0.0f };
        p = Norm(p);
        cur = V3{ want.x * c30 + p.x * s30, want.y * c30 + p.y * s30, want.z * c30 + p.z * s30 };
        vel = V3{ 0.0f, 0.0f, 0.0f };
    }
    if (dt > 0.0f)
    {
        const float r = 1.0f / dt;
        V3 a{ ((want.x - cur.x) * r - vel.x) * r, ((want.y - cur.y) * r - vel.y) * r, ((want.z - cur.z) * r - vel.z) * r };
        const float al = std::sqrt(Dot(a, a));
        if (A <= al) { const float k = A / al; a = V3{ a.x * k, a.y * k, a.z * k }; }
        vel = V3{ vel.x + (a.x - D * vel.x) * dt, vel.y + (a.y - D * vel.y) * dt, vel.z + (a.z - D * vel.z) * dt };
    }
    cur = Norm(V3{ cur.x + vel.x * dt, cur.y + vel.y * dt, cur.z + vel.z * dt });
}

}
