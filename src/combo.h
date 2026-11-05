#pragma once
#include <LibSWBF2/Wrappers/Level.h>
#include <LibSWBF2/Wrappers/Config.h>
#include <LibSWBF2/Hashing.h>
#include <string>
#include <vector>
#include <set>
#include <cstring>
#include <strings.h>

namespace combo {

struct Time
{
    float v = 0.0f;
    bool set = false, fromAnim = false, fromEnd = false;
    float At(float clipLen, float stateLen) const
    {
        const float s = fromAnim ? v * clipLen : v;
        return fromEnd ? stateLen - s : s;
    }
};

enum Button { B_NONE = -1, B_FIRE, B_FIRE2, B_FIREBOTH, B_JUMP, B_CROUCH, B_SPRINT, B_RELOAD, B_COUNT };
enum ButtonMode { BM_PRESS, BM_RELEASE, BM_DOWN, BM_UP, BM_TAP, BM_DOUBLETAP, BM_HOLD };

enum PostureBit { P_STAND = 1, P_CROUCH = 2, P_PRONE = 4, P_SPRINT = 8, P_JUMP = 16, P_JET = 32, P_ROLL = 64,
                  P_TUMBLE = 128, P_ALL = 255 };

struct Cond
{
    Time tStart, tEnd, brk;
    bool hasBreak = false;
    int button = B_NONE, mode = BM_PRESS;
    unsigned postures = 0;
    char energyOp = 0; float energy = 0.0f;
    char thrustOp = 0; float thrust = 0.0f;
    bool hasAngle = false; float angMin = 0.0f, angMax = 0.0f;
    char tipOp = 0; float timeInPosture = 0.0f;
    char velYOp = 0; float velY = 0.0f;
    bool anyButton = false;
};
struct Transition
{
    std::string target;
    int to = -1;
    float energyCost = 0.0f;
    std::vector<Cond> groups;
};
struct Attack
{
    Time t0, t1;
    float length = 0.0f; bool lengthFromEdge = false;
    float width = 0.0f, damage = 0.0f, push = 0.0f, distance = 0.0f;
    int edge = -1;
    std::string id;
    bool playExplosion = false;
};
struct DeflectAnim { std::string anim, dir; };
struct Deflect
{
    bool on = false;
    float angMin = -180.0f, angMax = 180.0f, energyCost = 0.0f;
    std::vector<DeflectAnim> anims;
};
struct State
{
    std::string name;
    unsigned posture = 0;
    std::string anim; bool loop = false, loopFinal = false, restart = false;

    std::string animUpper, animLower;
    std::string aimType;
    float blendIn = 0.15f, blendOut = 0.15f;
    std::string sound; Time soundAt;
    Time duration;
    float inputLockSec = -1.0f;
    std::vector<std::string> inputLock;
    bool alignedToThrust = false, mustShowOneFrame = false, turnOffSaber = false;
    float turnFactor = -1.0f, strafeFactor = -1.0f, thrustFactor = -1.0f;
    bool hasEnergyRestore = false, energyFromSoldier = false; float energyRestore = 0.0f;

    bool animatedMove = false;
    float velX = 0.0f, velZ = 0.0f, velFromThrust = 0.0f, velFromStrafe = 0.0f;
    bool hasVelY = false; float velY = 0.0f;
    bool hasGravity = false; float gravity = 1.0f;
    bool hasGravityTarget = false; float gravityTarget = 0.0f;
    bool until = false;
    Cond untilCond;
    bool playExplosion = false;
    std::vector<Attack> attacks;
    Deflect deflect;
    std::vector<Transition> transitions;
};
struct Combo
{
    std::string name;
    std::vector<State> states;
    int idle = -1;
    std::string throwAnim;
    std::vector<std::pair<std::string, std::string>> offhand;
    std::set<std::string> unknown;
    int Find(const std::string& n) const
    {
        for (size_t i = 0; i < states.size(); ++i) if (strcasecmp(states[i].name.c_str(), n.c_str()) == 0) return (int)i;
        return -1;
    }
};

using LibSWBF2::Wrappers::Field;
using LibSWBF2::Wrappers::Config;
using LibSWBF2::List;

inline uint32_t H(const char* n) { return LibSWBF2::FNV::Hash(LibSWBF2::Types::String(n)); }
inline bool IsStr(const Field& f, uint8_t i)
{
    if (i >= f.GetNumValues()) return false;
    const LibSWBF2::Types::String s = f.GetString(i);
    return s.Length() > 0 && s.Buffer()[0] >= 32 && s.Buffer()[0] < 127;
}
inline std::string Str(const Field& f, uint8_t i) { return IsStr(f, i) ? std::string(f.GetString(i).Buffer()) : std::string(); }
inline float Num(const Field& f, uint8_t i)
{
    if (i >= f.GetNumValues()) return 0.0f;
    if (IsStr(f, i)) return (float)atof(f.GetString(i).Buffer());
    return f.GetFloat(i);
}

inline Time ReadTime(const Field& f, uint8_t first = 0)
{
    Time t; t.set = f.GetNumValues() > first; t.v = Num(f, first);
    for (uint8_t i = first + 1; i < f.GetNumValues(); ++i)
    {
        const std::string u = Str(f, i);
        if (strcasecmp(u.c_str(), "frames") == 0) t.v *= 0.033333335f;
        else if (strcasecmp(u.c_str(), "fromanim") == 0) t.fromAnim = true;
        else if (strcasecmp(u.c_str(), "fromend") == 0) t.fromEnd = true;
    }
    return t;
}
inline unsigned PostureBits(const std::string& p)
{
    static const char* const n[] = { "Stand", "Crouch", "Prone", "Sprint", "Jump", "Jet", "Roll", "Tumble" };
    if (strcasecmp(p.c_str(), "Any") == 0 || strcasecmp(p.c_str(), "All") == 0) return P_ALL;
    for (int i = 0; i < 8; ++i) if (strcasecmp(p.c_str(), n[i]) == 0) return 1u << i;
    return 0;
}

inline unsigned ReadPostures(const Field& f)
{
    unsigned m = 0;
    for (uint8_t i = 0; i < f.GetNumValues(); ++i)
    {
        const std::string p = Str(f, i);
        if (!p.empty() && p[0] == '!') m &= ~PostureBits(p.substr(1));
        else m |= PostureBits(p);
    }
    return m;
}
inline char ReadOp(const std::string& s)
{
    if (s == ">=") return 'g';
    if (s == "<=") return 'l';
    return s.empty() ? 0 : s[0];
}
inline int ButtonOf(const std::string& b)
{
    static const char* const n[] = { "Fire", "FireSecondary", "FireBoth", "Jump", "Crouch", "Sprint", "Reload" };
    for (int i = 0; i < B_COUNT; ++i) if (strcasecmp(b.c_str(), n[i]) == 0) return i;
    return B_NONE;
}
inline int ModeOf(const std::string& m)
{
    static const char* const n[] = { "Press", "Release", "Down", "Up", "Tap", "DoubleTap", "Hold" };
    for (int i = 0; i < 7; ++i) if (strcasecmp(m.c_str(), n[i]) == 0) return i;
    return BM_PRESS;
}

inline void ReadCond(const List<const Field*>& fields, Cond& c, Combo& out)
{
    for (size_t i = 0; i < fields.Size(); ++i)
    {
        const Field& f = *fields[i];
        const uint32_t h = f.GetNameHash();
        if (h == H("Break")) { c.hasBreak = true; c.brk = ReadTime(f); c.brk.set = true; }
        else if (h == H("TimeStart")) c.tStart = ReadTime(f);
        else if (h == H("TimeEnd")) c.tEnd = ReadTime(f);
        else if (h == H("Button")) { c.button = ButtonOf(Str(f, 0)); c.mode = ModeOf(Str(f, 1)); if (c.button == B_NONE) out.unknown.insert("Button:" + Str(f, 0)); }
        else if (h == H("Posture")) c.postures = ReadPostures(f);
        else if (h == H("Energy")) { c.energyOp = ReadOp(Str(f, 0)); c.energy = Num(f, 1); }
        else if (h == H("Thrust")) { c.thrustOp = ReadOp(Str(f, 0)); c.thrust = Num(f, 1); }
        else if (h == H("ThrustAngle")) { c.hasAngle = true; c.angMin = Num(f, 0); c.angMax = Num(f, 1); }
        else if (h == H("TimeInPosture")) { c.tipOp = ReadOp(Str(f, 0)); c.timeInPosture = Num(f, 1); }
        else if (h == H("ButtonsPressed")) c.anyButton = true;
        else if (h == H("VelocityY")) { c.velYOp = ReadOp(Str(f, 0)); c.velY = Num(f, 1); }
        else out.unknown.insert(std::string("cond:") + f.GetName().Buffer());
    }
}

inline void ReadState(const Field& sf, State& st, Combo& out)
{
    st.name = Str(sf, 0);
    const List<const Field*> body = sf.m_Scope.GetFields();
    for (size_t k = 0; k < body.Size(); ++k)
    {
        const Field& f = *body[k];
        const uint32_t h = f.GetNameHash();
        if (h == H("Posture")) st.posture = ReadPostures(f);
        else if (h == H("Animation"))
        {
            const std::string clip = Str(f, 0);
            auto endsWith = [](const std::string& s, const char* suf) {
                const size_t n = strlen(suf);
                return s.size() > n && s.compare(s.size() - n, n, suf) == 0;
            };
            if (endsWith(clip, "_upper")) st.animUpper = clip;
            else if (endsWith(clip, "_lower")) { st.animLower = clip; if (st.anim.empty()) st.anim = clip; }
            else st.anim = clip;
            const List<const Field*> af = f.m_Scope.GetFields();
            for (size_t j = 0; j < af.Size(); ++j)
            {
                const Field& g = *af[j];
                const uint32_t gh = g.GetNameHash();
                if (gh == H("Loop")) { st.loop = true; st.loopFinal = strcasecmp(Str(g, 0).c_str(), "FinalFrame") == 0; }
                else if (gh == H("AimType")) st.aimType = Str(g, 0);
                else if (gh == H("BlendInTime") || gh == H("BlendTimeIn")) st.blendIn = Num(g, 0);
                else if (gh == H("BlendOutTime") || gh == H("BlendTimeOut")) st.blendOut = Num(g, 0);
                else if (gh == H("BlendTimeTo") || gh == H("LowResPose") || gh == H("SyncType")) {}
                else out.unknown.insert(std::string("anim:") + g.GetName().Buffer());
            }
        }
        else if (h == H("RestartAnimation")) st.restart = true;
        else if (h == H("Sound")) { st.sound = Str(f, 0); if (f.GetNumValues() > 1) st.soundAt = ReadTime(f, 1); }
        else if (h == H("Duration")) st.duration = ReadTime(f);
        else if (h == H("InputLock"))
        {
            uint8_t i = 0;
            st.inputLockSec = 0.0f;
            if (!IsStr(f, 0))
            {
                st.inputLockSec = Num(f, 0); i = 1;
                if (strcasecmp(Str(f, 1).c_str(), "frames") == 0) { st.inputLockSec *= 0.033333335f; i = 2; }
            }
            for (; i < f.GetNumValues(); ++i) st.inputLock.push_back(Str(f, i));
        }
        else if (h == H("AlignedToThrust")) st.alignedToThrust = true;
        else if (h == H("MustShowOneFrame")) st.mustShowOneFrame = true;
        else if (h == H("TurnOffLightsaber")) st.turnOffSaber = true;
        else if (h == H("PlayExplosion")) st.playExplosion = true;
        else if (h == H("TurnFactor")) st.turnFactor = Num(f, 0);
        else if (h == H("StrafeFactor")) st.strafeFactor = Num(f, 0);
        else if (h == H("ThrustFactor")) st.thrustFactor = Num(f, 0);
        else if (h == H("EnergyRestoreRate"))
        {
            st.hasEnergyRestore = true;
            st.energyFromSoldier = f.GetNumValues() == 0;
            st.energyRestore = Num(f, 0);
        }
        else if (h == H("AnimatedMove"))
        {
            st.animatedMove = true;
            const List<const Field*> mf = f.m_Scope.GetFields();
            for (size_t j = 0; j < mf.Size(); ++j)
            {
                const Field& g = *mf[j];
                const uint32_t gh = g.GetNameHash();
                if (gh == H("VelocityX")) st.velX = Num(g, 0);
                else if (gh == H("VelocityZ")) st.velZ = Num(g, 0);
                else if (gh == H("VelocityY")) { st.hasVelY = true; st.velY = Num(g, IsStr(g, 0) ? 1 : 0); }
                else if (gh == H("VelocityFromThrust")) st.velFromThrust = Num(g, 0);
                else if (gh == H("VelocityFromStrafe")) st.velFromStrafe = Num(g, 0);
                else if (gh == H("Gravity")) { st.hasGravity = true; st.gravity = Num(g, 0); }
                else if (gh == H("GravityVelocityTarget")) { st.hasGravityTarget = true; st.gravityTarget = Num(g, 0); }
                else if (gh == H("Until") || gh == H("If"))
                {
                    st.until = true;
                    ReadCond(g.m_Scope.GetFields(), st.untilCond, out);
                }
                else out.unknown.insert(std::string("move:") + g.GetName().Buffer());
            }
        }
        else if (h == H("Gravity")) { st.hasGravity = true; st.gravity = Num(f, 0); }
        else if (h == H("GravityVelocityTarget")) { st.hasGravityTarget = true; st.gravityTarget = Num(f, 0); }
        else if (h == H("Attack"))
        {
            Attack a;
            const List<const Field*> af = f.m_Scope.GetFields();
            for (size_t j = 0; j < af.Size(); ++j)
            {
                const Field& g = *af[j];
                const uint32_t gh = g.GetNameHash();
                if (gh == H("DamageTime")) { a.t0 = ReadTime(g, 0); a.t1.v = Num(g, 1); a.t1.set = true;
                    for (uint8_t i = 2; i < g.GetNumValues(); ++i)
                    {
                        const std::string u = Str(g, i);
                        if (strcasecmp(u.c_str(), "frames") == 0) { a.t0.v = Num(g, 0) * 0.033333335f; a.t1.v *= 0.033333335f; }
                        else if (strcasecmp(u.c_str(), "fromanim") == 0) a.t0.fromAnim = a.t1.fromAnim = true;
                    } }
                else if (gh == H("DamageLength")) { a.length = Num(g, 0); a.lengthFromEdge = strcasecmp(Str(g, 1).c_str(), "FromEdge") == 0; }
                else if (gh == H("DamageWidth")) a.width = Num(g, 0);
                else if (gh == H("Damage")) a.damage = Num(g, 0);
                else if (gh == H("Push")) a.push = Num(g, 0);
                else if (gh == H("DamageDistance")) a.distance = Num(g, 0);
                else if (gh == H("Edge")) a.edge = (int)Num(g, 0);
                else if (gh == H("AttackID")) a.id = Str(g, 0);
                else if (gh == H("PlayExplosion")) a.playExplosion = true;
                else out.unknown.insert(std::string("attack:") + g.GetName().Buffer());
            }
            st.attacks.push_back(a);
        }
        else if (h == H("Deflect"))
        {
            st.deflect.on = true;
            const List<const Field*> df = f.m_Scope.GetFields();
            for (size_t j = 0; j < df.Size(); ++j)
            {
                const Field& g = *df[j];
                const uint32_t gh = g.GetNameHash();
                if (gh == H("DeflectAngle")) { st.deflect.angMin = Num(g, 0); st.deflect.angMax = Num(g, 1); }
                else if (gh == H("EnergyCost")) st.deflect.energyCost = Num(g, 0);
                else if (gh == H("DeflectAnimation")) st.deflect.anims.push_back({ Str(g, 0), Str(g, 1) });
                else out.unknown.insert(std::string("deflect:") + g.GetName().Buffer());
            }
        }
        else if (h == H("Transition"))
        {
            Transition t; t.target = Str(f, 0);
            const List<const Field*> tf = f.m_Scope.GetFields();
            for (size_t j = 0; j < tf.Size(); ++j)
            {
                const Field& g = *tf[j];
                const uint32_t gh = g.GetNameHash();
                if (gh == H("If") || gh == H("Or")) { Cond c; ReadCond(g.m_Scope.GetFields(), c, out); t.groups.push_back(c); }
                else if (gh == H("EnergyCost")) t.energyCost = Num(g, 0);
                else out.unknown.insert(std::string("transition:") + g.GetName().Buffer());
            }
            st.transitions.push_back(t);
        }
        else out.unknown.insert(std::string("state:") + f.GetName().Buffer());
    }
}

inline void ParseConfig(const Config& cfg, Combo& out)
{
    const List<const Field*> top = cfg.GetFields();
    for (size_t i = 0; i < top.Size(); ++i)
    {
        const Field& f = *top[i];
        const uint32_t h = f.GetNameHash();
        if (h == H("State")) { State st; ReadState(f, st, out); out.states.push_back(st); }
        else if (h == H("ThrowAnimation")) out.throwAnim = Str(f, 0);
        else if (h == H("OffhandAnimation")) out.offhand.push_back({ Str(f, 0), Str(f, 1) });
        else if (h == H("Animation")) {}
        else out.unknown.insert(std::string("top:") + f.GetName().Buffer());
    }
    out.idle = out.Find("IDLE");
    for (State& st : out.states)
        for (Transition& t : st.transitions) t.to = out.Find(t.target);
}

inline bool Parse(LibSWBF2::Wrappers::Level* lvl, const std::string& name, Combo& out)
{
    out = Combo(); out.name = name;
    if (lvl == nullptr || name.empty()) return false;
    const uint32_t want = H(name.c_str());
    const List<const Config*> cfgs = lvl->GetConfigs(LibSWBF2::EConfigType::Combo);
    for (size_t i = 0; i < cfgs.Size(); ++i)
        if (cfgs[i]->m_Name == want) { ParseConfig(*cfgs[i], out); return true; }
    return false;
}

}
