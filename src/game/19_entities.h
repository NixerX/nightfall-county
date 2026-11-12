// Nightfall County: entities.
// Not a standalone header: included once, in order, from src/main.cpp. Everything above it in that list is visible here.
#pragma once
// ============================================================ entities
enum ZType {
    Z_WALKER,
    Z_RUNNER,
    Z_BRUTE,
    Z_BLOATER,
    Z_VOLATILE,
    Z_SOLDIER
};
struct ZDef {
    float hp, spd, r, dmg, h;
    Color skin, shirt, pants;
    float xp;
};
static const ZDef ZT[6] = {
    {55, 1.5f, .34f, 9, 1.75f, rgba(150, 160, 130), rgba(90, 96, 110), rgba(60, 64, 76), 10},
    {40, 5.0f, .3f, 7, 1.7f, rgba(170, 150, 128), rgba(120, 50, 50), rgba(70, 60, 54), 14},
    {480, 2.0f, .55f, 26, 2.25f, rgba(130, 110, 120), rgba(60, 64, 72), rgba(50, 50, 56), 70},
    {80, 1.1f, .5f, 6, 1.8f, rgba(160, 190, 90), rgba(120, 140, 60), rgba(80, 90, 50), 22},
    {170, 7.2f, .32f, 20, 1.85f, rgba(214, 220, 226), rgba(150, 156, 164), rgba(120, 126, 134), 50},
    {110, 1.8f, .34f, 12, 1.8f, rgba(140, 150, 124), rgba(84, 96, 70), rgba(76, 86, 64), 24}};
enum ZJob {
    J_NONE,
    J_MEDIC,
    J_COP,
    J_FIRE,
    J_HUNTER,
    J_MECH,
    J_HIKER,
    J_COUNT
}; // what they were before. Shows in their clothes, and in their pockets
static const Color JOBC[J_COUNT] = {{0, 0, 0, 0},        {225, 232, 240, 255}, {34, 44, 80, 255}, {200, 170, 60, 255},
                                    {230, 110, 30, 255}, {70, 96, 130, 255},   {170, 50, 40, 255}};
static float gExposure = 0; // highest detection meter among zombies near you (drives the HIDDEN/NOTICED colour)
struct Zombie {
    int job = 0;
    int id, type;
    Vector2 p, k{};
    float r, hp, mhp, spd, dmg, atk = .4f, seed, a, aggro = 0;
    int side;
    float burn = 0, slow = 0, flash = 0, chg = 0, chgT = 0, ca = 0, lureT = 0;
    Vector2 lp{}, np{};
    float noiseT = 0, wt = 0, wa, stun = 0, bleed = 0, losT = 0, walk = 0;
    bool sees = false;
    bool armL = true, armR = true, dead = false, gone = false;
    int via = 0;
    int look = -1;
    bool inSight = false;
    float notice = 0; // detection build-up: 0 unseen, 1 spotted
    bool hordeM = false;
    int hstate = 0;
    bool chasing = false;
    int pound = -1;
    float poundT = 0, ignoreT = 0; // horde marchers: 0 marching, 1 searching your last known spot
};
struct Corpse {
    Vector2 p;
    float yaw;
    int type;
    bool armL, armR, head;
    float t;
    int look = -1;
    float seed = 0;
};
enum PKind {
    P_BLOOD,
    P_SPARK,
    P_FIRE,
    P_SMOKE,
    P_MIST,
    P_DUST,
    P_ACID,
    P_CASING,
    P_RAIN
};
struct Particle {
    Vector3 p, v;
    float t, mt, s;
    Color c;
    int kind;
};
enum GibKind {
    G_LIMB,
    G_HEAD,
    G_CHUNK,
    G_RIB,
    G_GUTS
};
struct Gib {
    Vector3 p, v, axis;
    float rot, rotv;
    int kind;
    Color c;
    Vector3 s;
    float t, trail;
    bool acid, rest;
};
struct Bullet {
    Vector2 p, v;
    float dmg, life;
    int pierce;
    int hits[8];
    int nh = 0;
    bool burn, shotgun;
    float leech, crit, dism;
};
struct Bomb {
    Vector3 p, v;
    float t, beep = 0, blink = 0;
    bool flare;
};
struct AcidPool {
    Vector2 p;
    float r, t;
};
struct Decal {
    Vector2 p;
    float r, rot, a;
    int v;
    Color c;
    float grow, y;
};
struct Floater {
    Vector3 p;
    std::string txt;
    Color c;
    float size, t, mt;
};
struct Light {
    Vector3 p;
    float r, t, mt;
    Color c;
};
struct ScreenDrop {
    Vector2 p;
    float r, t, mt;
};
static std::vector<Zombie> zombies;
static std::vector<Corpse> corpses;
static std::vector<Particle> parts;
static std::vector<Gib> gibs;
static std::vector<Bullet> bullets;
static std::vector<Bomb> bombs;
static std::vector<AcidPool> acid;
static std::vector<Decal> decals;
static int decalHead = 0;
static std::vector<Floater> floaters;
static std::vector<Light> lights;
static std::vector<ScreenDrop> sdrops;
