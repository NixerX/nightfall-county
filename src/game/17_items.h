// Nightfall County: items.
// Not a standalone header: included once, in order, from src/main.cpp. Everything above it in that list is visible here.
#pragma once
// ============================================================ items
enum Cal {
    C_9MM,
    C_12GA,
    C_556,
    C_308,
    C_COUNT
};
static const char* CALN[4] = {"9mm", "12ga", "5.56", ".308"};
enum ItemId {
    IT_BEANS,
    IT_APPLE,
    IT_CHIPS,
    IT_PEACHES,
    IT_WATER,
    IT_SODA,
    IT_BANDAGE,
    IT_ANTIBIO,
    IT_PAINKILL,
    IT_MORPHINE,
    IT_BATTERY,
    IT_FLARE,
    IT_PIPEBOMB,
    IT_A9,
    IT_A12,
    IT_A556,
    IT_A308,
    IT_LOG,
    IT_KINDLING,
    IT_SPLINT,
    IT_MATCHES,
    IT_RAG,
    IT_TAPE,
    IT_SEWKIT,
    IT_LEATHER,
    IT_WAX,
    IT_DYE_BLACK,
    IT_DYE_GREEN,
    IT_DYE_BROWN,
    IT_ROPE,
    IT_GAS,
    IT_COUNT
};
struct ItemDef {
    const char* n;
    float food, water;
    int stack;
    Color c;
    const char* use;
};
static const ItemDef IDEF[IT_COUNT] = {{"Canned Beans", 38, -2, 4, rgba(170, 90, 60), "Eat"},
                                       {"Apple", 12, 6, 6, rgba(180, 40, 40), "Eat"},
                                       {"Potato Chips", 20, -6, 4, rgba(220, 190, 70), "Eat"},
                                       {"Canned Peaches", 26, 12, 4, rgba(230, 150, 80), "Eat"},
                                       {"Water Bottle", 0, 45, 4, rgba(120, 180, 220), "Drink"},
                                       {"Soda Can", 4, 26, 6, rgba(200, 50, 50), "Drink"},
                                       {"Bandage", 0, 0, 6, rgba(230, 226, 214), "Bandage"},
                                       {"Antibiotics", 0, 0, 4, rgba(240, 240, 240), "Take"},
                                       {"Painkillers", 0, 0, 6, rgba(220, 220, 255), "Take"},
                                       {"Morphine", 0, 0, 3, rgba(200, 220, 200), "Inject"},
                                       {"Battery", 0, 0, 4, rgba(110, 90, 180), "Swap"},
                                       {"Road Flare", 0, 0, 6, rgba(230, 60, 40), "Light"},
                                       {"Pipe Bomb", 0, 0, 4, rgba(120, 124, 130), "Throw"},
                                       {"9mm Rounds", 0, 0, 60, rgba(200, 170, 90), ""},
                                       {"12ga Shells", 0, 0, 30, rgba(200, 60, 50), ""},
                                       {"5.56 Rounds", 0, 0, 60, rgba(200, 170, 90), ""},
                                       {".308 Rounds", 0, 0, 30, rgba(200, 170, 90), ""},
                                       {"Firewood Log", 0, 0, 2, rgba(120, 84, 52), "Split"},
                                       {"Kindling", 0, 0, 12, rgba(190, 150, 100), "Build fire"},
                                       {"Splint", 0, 0, 2, rgba(200, 170, 120), "Apply"},
                                       {"Matches", 0, 0, 40, rgba(220, 120, 60), ""},
                                       {"Rag", 0, 0, 6, rgba(200, 196, 180), "Bandage"},
                                       {"Duct Tape", 0, 0, 1, rgba(160, 164, 170), "Repair"},
                                       {"Sewing Kit", 0, 0, 1, rgba(200, 80, 90), "Repair"},
                                       {"Leather Patch", 0, 0, 4, rgba(130, 84, 50), "Patch"},
                                       {"Wax Tin", 0, 0, 1, rgba(210, 190, 110), "Waterproof"},
                                       {"Black Dye", 0, 0, 1, rgba(30, 30, 34), "Dye"},
                                       {"Olive Dye", 0, 0, 1, rgba(90, 104, 60), "Dye"},
                                       {"Brown Dye", 0, 0, 1, rgba(110, 76, 46), "Dye"},
                                       {"Rope", 0, 0, 1, rgba(180, 160, 110), ""},
                                       {"Jerry Can", 0, 0, 1, rgba(170, 40, 30), ""}};
static const int ISZ[IT_COUNT][2] = {
    {1, 2}, {1, 1}, {1, 2}, {1, 2}, {1, 2}, {1, 1}, {1, 1}, {1, 1}, {1, 1}, {1, 1}, {1, 1},
    {1, 2}, {1, 2}, {1, 1}, {1, 1}, {1, 1}, {1, 1}, {2, 3}, {1, 2}, {1, 3}, {1, 1}, {1, 1},
    {1, 1}, {1, 2}, {1, 1}, {1, 1}, {1, 1}, {1, 1}, {1, 1}, {2, 2}, {2, 3}}; // grid footprint of each item
static bool isAmmo(int id) {
    return id >= IT_A9 && id <= IT_A308;
}
static int ammoItem(int cal) {
    return IT_A9 + cal;
}

enum WKey {
    W_M9,
    W_GLOCK,
    W_MP5,
    W_SHOTGUN,
    W_AR15,
    W_M4,
    W_HUNT,
    W_COUNT
};
struct WBase {
    const char* n;
    int slot, cal;
    float dmg, rate, spread;
    int pel;
    float spd, range;
    int mag;
    float rel;
    bool autoF;
    float kick;
    int snd;
    float len;
    int pierce;
    float dism, noise;
};
static const WBase WB[W_COUNT] = {
    {"M9 Pistol", 1, C_9MM, 30, .24f, .04f, 1, 230, 60, 15, 1.1f, false, 2.5f, 0, .26f, 0, .08f, 70},
    {"Glock 17", 1, C_9MM, 27, .17f, .05f, 1, 230, 55, 17, 1.0f, false, 2.f, 0, .24f, 0, .06f, 65},
    {"MP5", 0, C_9MM, 22, .075f, .075f, 1, 240, 60, 30, 1.6f, true, 1.4f, 1, .5f, 0, .06f, 75},
    {"Pump Shotgun", 0, C_12GA, 17, .85f, .2f, 8, 200, 24, 6, 2.4f, false, 8, 2, .75f, 0, .3f, 110},
    {"AR-15", 0, C_556, 40, .11f, .035f, 1, 320, 95, 30, 1.9f, true, 2.4f, 0, .78f, 0, .14f, 120},
    {"M4A1", 0, C_556, 42, .085f, .04f, 1, 330, 95, 30, 1.8f, true, 2.6f, 0, .8f, 0, .14f, 130},
    {"Hunting Rifle", 0, C_308, 140, 1.0f, .008f, 1, 420, 150, 5, 2.6f, false, 10, 3, 1.0f, 2, .65f, 150}};
struct Weapon {
    int key = -1, rar = 0, cal = 0, slot = 0;
    float dmg, rate, spread, spd, range, rel, kick, len, dism, crit = 0, leech = 0, noise;
    int pel, mag, snd, pierce, clip = 0;
    bool autoF, burn = false;
    std::vector<int> aff;
    std::string name;
};
struct Affix {
    const char* pre;
    const char* suf;
    const char* desc;
};
static const Affix AFF[10] = {{"Vicious", nullptr, "+35% damage"},
                              {"Rapid", nullptr, "+30% fire rate"},
                              {nullptr, "of Plenty", "+60% magazine"},
                              {nullptr, "of Piercing", "Pierces 2 more targets"},
                              {nullptr, "of Embers", "Sets the dead on fire"},
                              {nullptr, "of the Leech", "Heals on hit"},
                              {"Steady", nullptr, "Half the spread"},
                              {"Quick", nullptr, "40% faster reload"},
                              {"Brutal", nullptr, "+12% headshot chance"},
                              {"Butcher's", nullptr, "Rips limbs off more often"}};
static const std::vector<std::string> LEG = {"Hellbender",   "Last Rites", "Widowmaker",   "Gravedigger", "Sunrise",
                                             "Bad Neighbor", "Mercy",      "Old Faithful", "Pale Rider",  "Cascadia"};
static Weapon makeWeapon(int key, int rar) {
    const WBase& b = WB[key];
    Weapon w;
    w.key = key;
    w.rar = rar;
    w.cal = b.cal;
    w.slot = b.slot;
    w.dmg = b.dmg;
    w.rate = b.rate;
    w.spread = b.spread;
    w.pel = b.pel;
    w.spd = b.spd;
    w.range = b.range;
    w.mag = b.mag;
    w.rel = b.rel;
    w.autoF = b.autoF;
    w.kick = b.kick;
    w.snd = b.snd;
    w.len = b.len;
    w.pierce = b.pierce;
    w.dism = b.dism;
    w.noise = b.noise;
    static const float rm[4] = {1, 1.08f, 1.18f, 1.32f};
    w.dmg *= rm[rar];
    std::vector<int> pool;
    for (int i = 0; i < 10; i++)
        pool.push_back(i);
    for (int i = 0; i < rar; i++) {
        int j = irand(0, (int)pool.size() - 1), id = pool[j];
        pool.erase(pool.begin() + j);
        w.aff.push_back(id);
        switch (id) {
        case 0:
            w.dmg *= 1.35f;
            break;
        case 1:
            w.rate /= 1.3f;
            break;
        case 2:
            w.mag = (int)roundf(w.mag * 1.6f);
            break;
        case 3:
            w.pierce += 2;
            break;
        case 4:
            w.burn = true;
            break;
        case 5:
            w.leech += .5f;
            break;
        case 6:
            w.spread *= .5f;
            break;
        case 7:
            w.rel *= .6f;
            break;
        case 8:
            w.crit += .12f;
            break;
        case 9:
            w.dism = std::min(.9f, w.dism * 2.5f + .15f);
            break;
        }
    }
    if (rar == 3)
        w.name = "\"" + pick(LEG) + "\" " + b.n;
    else {
        const char* pre = nullptr;
        const char* suf = nullptr;
        for (int id : w.aff) {
            if (!pre && AFF[id].pre)
                pre = AFF[id].pre;
            if (!suf && AFF[id].suf)
                suf = AFF[id].suf;
        }
        w.name = std::string(pre ? std::string(pre) + " " : "") + b.n + (suf ? std::string(" ") + suf : "");
    }
    w.clip = irand(0, w.mag / 2);
    return w;
}
enum MKey {
    MK_FISTS,
    MK_KNIFE,
    MK_MACHETE,
    MK_AXE,
    MK_BAT,
    MK_CROWBAR,
    MK_HATCHET,
    MK_WRENCH,
    MK_PIPE,
    MK_COUNT
};
struct MBase {
    const char* n;
    float dmg, rate, range, arc, dism, len, sta;
    Color c;
};
static const MBase MB[MK_COUNT] = {{"Fists", 10, .45f, 1.3f, .7f, 0, 0, 6, BONE},
                                   {"Kitchen Knife", 26, .38f, 1.5f, .7f, .15f, .35f, 7, rgba(200, 205, 210)},
                                   {"Machete", 55, .5f, 1.9f, 1.0f, .5f, .7f, 10, rgba(200, 205, 210)},
                                   {"Fire Axe", 85, .85f, 2.0f, .9f, .75f, .85f, 16, rgba(190, 40, 30)},
                                   {"Baseball Bat", 42, .55f, 2.0f, 1.1f, .1f, .85f, 11, rgba(160, 120, 80)},
                                   {"Crowbar", 48, .6f, 1.8f, .9f, .1f, .75f, 11, rgba(150, 40, 36)},
                                   {"Hatchet", 50, .55f, 1.6f, .8f, .45f, .45f, 10, rgba(170, 140, 100)},
                                   {"Pipe Wrench", 44, .62f, 1.7f, .9f, .08f, .45f, 11, rgba(90, 100, 120)},
                                   {"Lead Pipe", 38, .55f, 1.9f, 1.f, .05f, .8f, 10, rgba(120, 124, 128)}};
static bool canChop(int mk) {
    return mk == MK_AXE || mk == MK_HATCHET;
}
