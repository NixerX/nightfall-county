// Nightfall County: player.
// Not a standalone header: included once, in order, from src/main.cpp. Everything above it in that list is visible here.
#pragma once
constexpr float STA_K = .7f; // stamina cost multiplier: sprint, swings, dodge, kick, shove, vault, block
// ============================================================ player
struct Perks {
    float dmg = 1, rate = 1, crit = .06f, metab = 1, sta = 1, luck = 1, blast = 1, cold = 1, melee = 1, noise = 1,
          medic = 1;
    float regen = 0;
    float maxhp = 100, reload = 1, armor = 0, speed = 1, pistol = 1, shotgun = 1, rifle = 1, stealth = 0, cdr = 0,
          ammo = 1, batt = 1, bleedres = 0, block = 0, kick = 1;
};
struct InvSlot {
    int id, n;
};
struct Player {
    Vector2 p{0, 0}, k{}, rv{};
    float a = 0, hp = 100, food = 80, water = 80, temp = 37.f, sta = 100, staD = 0, roll = 0, rollCd = 0, iframe = 0,
          hurt = 0;
    int bleeds = 0;
    float sick = 0, pain = 0, shiver = 0;
    bool broken = false;
    Weapon gun[2];
    bool hasGun[2] = {false, false};
    int melee = MK_FISTS;
    int active = 2;
    std::shared_ptr<Cloth> eq[CS_COUNT];
    float wet = 0;
    float batt = 60, xp = 0, next = 0;
    int lvl = 1;
    bool light = false, uv = false, sprint = false, crouch = false, fired = false;
    float reload = 0, cd = 0, step = 0, swing = 0, swingCd = 0, swingA = 0, walked = 0, alive = 0;
    Perks perks;
    int cls = 0;
    int rank[3][4] = {};
    int points = 0;
    float actCd = 0, actT = 0;
    bool splinted = false;
    float splintT = 0;
    int loft = -1;
    float moveA = 0;
    bool moving = false;
    float hideK = 1, firedT = 0;
    bool blocking = false, autorun = false;
    float shieldHP = 0, kickCd = 0, kickT = 0, autoA = 0, blockFx = 0;
    Vector2 vel{};
    bool back = false;
    float rmbT = 0, shoveCd = 0, vaultT = 0, vaultH = 0;
    Vector2 vaultA{}, vaultB{};
    float animA = 0;
};
static Player P;
