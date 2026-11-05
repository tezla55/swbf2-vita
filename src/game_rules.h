#pragma once

#include <math.h>
#include <string.h>

static inline float AngleDiff(float a, float b)
{
    float d = fmodf(b - a, 2.0f * 3.14159265f);
    if (d > 3.14159265f) d -= 2.0f * 3.14159265f;
    if (d < -3.14159265f) d += 2.0f * 3.14159265f;
    return d;
}

static inline float WrapAngle(float a) { return AngleDiff(0.0f, a); }

enum AnimScope { AS_UPPER = 0, AS_LOWER, AS_FULL };

enum AnimPosture { AP_STAND = 0, AP_CROUCH, AP_PRONE,
                   AP_STANDALERT, AP_CROUCHALERT, AP_PRONEALERT, AP_POSTURE_COUNT };

enum AnimCategory { ACAT_WEAPON = 0, ACAT_MOVEMENT, ACAT_ACTION };

static inline float AnimRealBlendTime(AnimCategory cat, const char* anim, AnimPosture posture)
{
    if (cat == ACAT_WEAPON)
    {

        return (strncmp(anim, "shoot", 5) == 0) ? 0.02f : 0.5f;
    }
    if (cat == ACAT_MOVEMENT)
    {

        return (posture == AP_PRONE || posture == AP_PRONEALERT) ? 0.4f : 0.15f;
    }

    if (strcmp(anim, "fall") == 0)            return 0.8f;
    if (strcmp(anim, "thrown_flail") == 0)    return 0.5f;
    if (strncmp(anim, "jump", 4) == 0)        return 0.4f;
    if (strncmp(anim, "thrown_land", 11) == 0) return 0.4f;
    if (strncmp(anim, "thrown_tumble", 13) == 0) return 0.1f;
    if (strncmp(anim, "thrown_bounce", 13) == 0) return 0.1f;
    if (strcmp(anim, "landsoft") == 0 || strcmp(anim, "landhard") == 0) return 0.05f;
    return 0.15f;
}

static const float SOLDIER_HEAD_MULT_PC = 3.0f;
static const float SOLDIER_HEAD_MULT = 1.0f;
static const float HEADSHOT_DAMAGE_MULT = SOLDIER_HEAD_MULT;

enum { AI_RULES_PS2 = 0, AI_RULES_PC = 1 };
inline int& AiRulesMode() { static int m = AI_RULES_PS2; return m; }

inline int& AiDifficultyLevel() { static int l = 2; return l; }

static inline float HitLocationMult(bool isHeadshot, bool attackerIsHuman, int level = AiDifficultyLevel())
{
    float m = isHeadshot ? (AiRulesMode() == AI_RULES_PC ? SOLDIER_HEAD_MULT_PC : SOLDIER_HEAD_MULT) : 1.0f;
    if (m > 1.0f && level < 3 && !attackerIsHuman) m = 1.0f;
    return m;
}

static inline float ApplyHitLocation(float damage, bool isHeadshot, bool attackerIsHuman)
{
    return damage * HitLocationMult(isHeadshot, attackerIsHuman);
}

static const float HIT_HEAD_MIN_Y = 1.5f;

static inline float ExplosionDamage(float explDamage, float falloff, float healthTypeScale)
{
    return explDamage * falloff * healthTypeScale;
}

static const float FALL_DAMAGE_THRESHOLD = 10.0f;
static const float FALL_DAMAGE_THRESHOLD_UP = 3.0f;
static const float FALL_DAMAGE_SCALE = 1.0f;

static inline float FallDamageFromImpact(float impactSpeed, float normalUp = 1.0f)
{
    const float thr = FALL_DAMAGE_THRESHOLD + FALL_DAMAGE_THRESHOLD_UP * normalUp;
    if (impactSpeed <= thr) return 0.0f;
    const float over = impactSpeed - thr;
    return FALL_DAMAGE_SCALE * over * over;
}

static const float THROW_MIN_UP_SPEED = 3.0f;
static const float THROW_MIN_SPEED = 6.0f;
static inline bool PushThrows(float vx, float vy, float vz)
{
    return vy >= THROW_MIN_UP_SPEED && (vx * vx + vy * vy + vz * vz) >= THROW_MIN_SPEED * THROW_MIN_SPEED;
}

static const float HUD_HEALTH_BAR_X = 0.082181f;
static const float HUD_HEALTH_BAR_Y = 0.830954f;
static const float HUD_ENERGY_BAR_X = 0.082305f;
static const float HUD_ENERGY_BAR_Y = 0.854368f;

static const float HUD_BAR_GROUP_ALPHA = 0.600000f;

static const float HUD_HEALTH_BACKDROP_DX = -0.004992f;
static const float HUD_HEALTH_BACKDROP_W  = 0.211292f;

static inline bool HitMarkerIsFriendly(int targetTeam, int playerTeam)
{
    return targetTeam == playerTeam;
}

static inline float ZeroScrollRate(unsigned rawParam)
{
    return (float)(unsigned char)rawParam * -0.003921569f;
}

static inline bool SkyTextureTiles(double edgePerRow, double innerPerRow)
{
    return edgePerRow <= 2.0 * innerPerRow + 3.0;
}

static const float kSkyConsoleGrade[3] = { 1.00f, 0.82f, 0.57f };

static inline int SkyObjectLiveCount(const char* geometryLower, int numObjects)
{
    if (geometryLower != 0 && strcmp(geometryLower, "cis_fly_droidfighter_dome") == 0 && numObjects > 8) return 8;
    return numObjects;
}
