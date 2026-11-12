// Nightfall County: clothing.
// Not a standalone header: included once, in order, from src/main.cpp. Everything above it in that list is visible here.
#pragma once
// ============================================================ clothing (DayZ style: your clothes are your storage)
enum CSlot {
    CS_HEAD,
    CS_EYES,
    CS_FACE,
    CS_TORSO,
    CS_OUTER,
    CS_HANDS,
    CS_LEGS,
    CS_SOCKS,
    CS_FEET,
    CS_BACK,
    CS_COUNT
};
static const char* CSN[CS_COUNT] = {"HEAD",  "EYES", "FACE",  "SHIRT", "JACKET",
                                    "HANDS", "LEGS", "SOCKS", "FEET",  "BACK"};
enum CFam {
    F_CIV = 1,
    F_WORK = 2,
    F_POLICE = 4,
    F_MIL = 8,
    F_FIRE = 16,
    F_FARM = 32,
    F_HUNT = 64,
    F_SCHOOL = 128,
    F_MED = 256,
    F_STORE = 512
};
enum CPal {
    PL_CASUAL,
    PL_WORK,
    PL_MIL,
    PL_POLICE,
    PL_HUNT,
    PL_NEUTRAL,
    PL_DENIM,
    PL_BRIGHT
};
struct CDef {
    const char* n;
    int slot;
    float warm, water, armor, noise, vis;
    int gw, gh, iw, ih, fam, pal;
    float rare;
};
static const CDef CD[] = {
    {"Beanie", CS_HEAD, .08f, .05f, 0, 1, 1, 0, 0, 1, 1, F_CIV | F_STORE | F_WORK, PL_CASUAL, 1},
    {"Baseball Cap", CS_HEAD, .02f, .1f, 0, 1, 1, 0, 0, 1, 1, F_CIV | F_STORE | F_SCHOOL, PL_BRIGHT, 1},
    {"Cowboy Hat", CS_HEAD, .03f, .25f, 0, 1, 1, 0, 0, 2, 2, F_FARM | F_CIV, PL_WORK, .5f},
    {"Hard Hat", CS_HEAD, 0, .2f, .12f, 1, 1.05f, 0, 0, 2, 2, F_WORK, PL_BRIGHT, .7f},
    {"Firefighter Helmet", CS_HEAD, .04f, .4f, .2f, 1, 1.05f, 0, 0, 2, 2, F_FIRE, PL_BRIGHT, .6f},
    {"Combat Helmet", CS_HEAD, .02f, .2f, .3f, 1, .95f, 0, 0, 2, 2, F_MIL, PL_MIL, .25f},
    {"Blaze Orange Cap", CS_HEAD, .04f, .1f, 0, 1, 1.2f, 0, 0, 1, 1, F_HUNT, PL_BRIGHT, .8f},
    {"Riot Helmet", CS_HEAD, .02f, .3f, .26f, 1, 1, 0, 0, 2, 2, F_POLICE, PL_POLICE, .3f},
    {"Sunglasses", CS_EYES, 0, 0, 0, 1, 1, 0, 0, 1, 1, F_CIV | F_STORE, PL_NEUTRAL, 1},
    {"Safety Glasses", CS_EYES, 0, 0, .02f, 1, 1, 0, 0, 1, 1, F_WORK | F_MED, PL_NEUTRAL, .8f},
    {"Ski Goggles", CS_EYES, .03f, .05f, .01f, 1, 1, 0, 0, 1, 1, F_STORE | F_CIV, PL_BRIGHT, .4f},
    {"Tactical Goggles", CS_EYES, .02f, .05f, .03f, 1, .97f, 0, 0, 1, 1, F_MIL | F_POLICE, PL_MIL, .3f},
    {"Bandana", CS_FACE, .02f, 0, 0, 1, 1, 0, 0, 1, 1, F_CIV | F_FARM | F_STORE, PL_BRIGHT, 1},
    {"Surgical Mask", CS_FACE, 0, 0, 0, 1, 1, 0, 0, 1, 1, F_MED, PL_NEUTRAL, 1},
    {"Balaclava", CS_FACE, .1f, .05f, .02f, 1, .95f, 0, 0, 1, 1, F_MIL | F_STORE, PL_NEUTRAL, .5f},
    {"Gas Mask", CS_FACE, 0, 0, .04f, 1.05f, 1, 0, 0, 2, 2, F_MIL | F_FIRE | F_POLICE, PL_MIL, .2f},
    {"T-Shirt", CS_TORSO, .04f, 0, 0, 1, 1, 2, 2, 2, 2, F_CIV | F_STORE | F_SCHOOL, PL_BRIGHT, 1},
    {"Flannel Shirt", CS_TORSO, .1f, .05f, .02f, 1, 1, 2, 2, 2, 2, F_CIV | F_FARM | F_WORK, PL_CASUAL, 1},
    {"Thermal Top", CS_TORSO, .16f, 0, 0, 1, 1, 2, 2, 2, 2, F_STORE | F_HUNT | F_CIV, PL_NEUTRAL, .6f},
    {"Work Shirt", CS_TORSO, .06f, .05f, .04f, 1, 1, 3, 2, 2, 2, F_WORK | F_FARM, PL_WORK, .8f},
    {"Police Shirt", CS_TORSO, .05f, 0, .02f, 1, 1, 3, 2, 2, 2, F_POLICE, PL_POLICE, .7f},
    {"Hoodie", CS_OUTER, .2f, .05f, .02f, 1, 1, 3, 2, 2, 3, F_CIV | F_SCHOOL | F_STORE, PL_CASUAL, 1},
    {"Wool Sweater", CS_OUTER, .26f, .08f, .02f, 1, 1, 2, 2, 2, 3, F_CIV | F_FARM, PL_CASUAL, .8f},
    {"Denim Jacket", CS_OUTER, .14f, .1f, .06f, 1, 1, 3, 2, 2, 3, F_CIV | F_STORE, PL_DENIM, .8f},
    {"Rain Jacket", CS_OUTER, .1f, .75f, .02f, 1.25f, 1.05f, 3, 3, 3, 3, F_STORE | F_CIV | F_FARM, PL_BRIGHT, .7f},
    {"Leather Jacket", CS_OUTER, .18f, .3f, .15f, 1.05f, 1, 3, 2, 3, 3, F_CIV | F_STORE, PL_NEUTRAL, .45f},
    {"Puffer Jacket", CS_OUTER, .42f, .3f, .04f, 1.15f, 1.05f, 3, 3, 3, 3, F_STORE | F_CIV, PL_BRIGHT, .4f},
    {"Hunting Coat", CS_OUTER, .32f, .35f, .06f, .95f, .82f, 4, 3, 3, 4, F_HUNT | F_FARM, PL_HUNT, .4f},
    {"Military Jacket", CS_OUTER, .24f, .3f, .12f, 1, .85f, 4, 3, 3, 4, F_MIL, PL_MIL, .45f},
    {"Firefighter Coat", CS_OUTER, .28f, .45f, .16f, 1.1f, 1.1f, 3, 3, 3, 4, F_FIRE, PL_BRIGHT, .4f},
    {"Police Vest", CS_OUTER, .05f, .05f, .3f, 1.05f, 1, 3, 2, 3, 3, F_POLICE, PL_POLICE, .25f},
    {"Plate Carrier", CS_OUTER, .05f, .05f, .4f, 1.1f, .95f, 4, 2, 3, 3, F_MIL, PL_MIL, .12f},
    {"Work Gloves", CS_HANDS, .05f, .05f, .05f, 1, 1, 0, 0, 1, 2, F_WORK | F_FARM | F_STORE, PL_WORK, 1},
    {"Wool Gloves", CS_HANDS, .1f, 0, 0, 1, 1, 0, 0, 1, 2, F_CIV | F_STORE, PL_CASUAL, .9f},
    {"Leather Gloves", CS_HANDS, .07f, .1f, .06f, 1, 1, 0, 0, 1, 2, F_CIV | F_POLICE, PL_NEUTRAL, .6f},
    {"Tactical Gloves", CS_HANDS, .05f, .1f, .08f, 1, .97f, 0, 0, 1, 2, F_MIL | F_POLICE, PL_MIL, .4f},
    {"Jeans", CS_LEGS, .07f, .05f, .05f, 1, 1, 2, 3, 2, 3, F_CIV | F_STORE | F_SCHOOL | F_FARM, PL_DENIM, 1},
    {"Sweatpants", CS_LEGS, .1f, 0, 0, .9f, 1, 2, 2, 2, 3, F_CIV | F_SCHOOL, PL_CASUAL, 1},
    {"Cargo Pants", CS_LEGS, .08f, .1f, .05f, 1, 1, 3, 3, 2, 3, F_CIV | F_STORE | F_WORK, PL_WORK, .7f},
    {"Work Pants", CS_LEGS, .08f, .1f, .08f, 1, 1, 2, 3, 2, 3, F_WORK | F_FARM, PL_WORK, .8f},
    {"Hunting Pants", CS_LEGS, .14f, .3f, .05f, .95f, .85f, 3, 3, 2, 3, F_HUNT, PL_HUNT, .5f},
    {"Military Pants", CS_LEGS, .1f, .2f, .1f, 1, .87f, 3, 3, 2, 3, F_MIL, PL_MIL, .45f},
    {"Firefighter Pants", CS_LEGS, .15f, .4f, .1f, 1.1f, 1.08f, 2, 3, 2, 3, F_FIRE, PL_BRIGHT, .45f},
    {"Cotton Socks", CS_SOCKS, .03f, 0, 0, 1, 1, 0, 0, 1, 1, F_CIV | F_STORE | F_SCHOOL, PL_NEUTRAL, 1},
    {"Wool Socks", CS_SOCKS, .08f, .05f, 0, 1, 1, 0, 0, 1, 1, F_CIV | F_STORE | F_HUNT | F_FARM, PL_CASUAL, .8f},
    {"Sneakers", CS_FEET, .03f, 0, .01f, .85f, 1, 0, 0, 2, 2, F_CIV | F_SCHOOL | F_STORE, PL_BRIGHT, 1},
    {"Work Boots", CS_FEET, .06f, .2f, .05f, 1.05f, 1, 0, 0, 2, 2, F_WORK | F_FARM, PL_WORK, .8f},
    {"Hiking Boots", CS_FEET, .08f, .35f, .04f, 1, 1, 0, 0, 2, 2, F_STORE | F_HUNT | F_CIV, PL_WORK, .6f},
    {"Rubber Boots", CS_FEET, .04f, .8f, .02f, 1.1f, 1, 0, 0, 2, 2, F_FARM | F_STORE, PL_BRIGHT, .6f},
    {"Combat Boots", CS_FEET, .07f, .35f, .08f, 1, 1, 0, 0, 2, 2, F_MIL | F_POLICE, PL_NEUTRAL, .45f},
    {"Dress Shoes", CS_FEET, .02f, .05f, 0, 1.2f, 1, 0, 0, 2, 2, F_CIV, PL_NEUTRAL, .6f},
    {"Drawstring Bag", CS_BACK, 0, 0, 0, 1, 1, 3, 3, 2, 2, F_CIV | F_SCHOOL | F_STORE, PL_BRIGHT, 1},
    {"School Backpack", CS_BACK, 0, .05f, 0, 1, 1, 4, 4, 3, 3, F_SCHOOL | F_CIV, PL_BRIGHT, .8f},
    {"Hiking Pack", CS_BACK, 0, .2f, 0, 1.05f, 1, 5, 5, 3, 4, F_STORE | F_HUNT, PL_BRIGHT, .4f},
    {"Hunting Pack", CS_BACK, 0, .2f, 0, .95f, .9f, 5, 4, 3, 4, F_HUNT, PL_HUNT, .35f},
    {"Military Pack", CS_BACK, 0, .25f, .02f, 1, .92f, 6, 6, 4, 4, F_MIL, PL_MIL, .15f}};
constexpr int CD_N = sizeof(CD) / sizeof(CD[0]);
static const char* PATN[4] = {"", "Plaid ", "Camo ", "Faded "};
static const char* CONDN[5] = {"Pristine", "Worn", "Damaged", "Badly Damaged", "Ruined"};
struct InvItem;
struct Cloth;
struct InvItem {
    int kind = 0, id = -1, n = 1, x = 0, y = 0;
    bool rot = false;
    std::shared_ptr<Cloth> c;
}; // kind 0 = item, 1 = clothing
struct Cloth {
    int def = 0;
    float cond = 1;
    Color col{};
    int pattern = 0, rar = 0, upg = 0, extraRows = 0, set = -1;
    float bWarm = 0, bWater = 0, bArmor = 0, bQuiet = 0, bVis = 1;
    std::string name, decor;
    std::vector<InvItem> items;
};
static int condTier(float c) {
    return c > .8f ? 0 : c > .55f ? 1 : c > .3f ? 2 : c > 0 ? 3 : 4;
}
static int gridW(const Cloth& c) {
    return CD[c.def].gw;
}
static int gridH(const Cloth& c) {
    return CD[c.def].gh ? CD[c.def].gh + c.extraRows : 0;
}
static Color palColor(int pal) {
    static const std::vector<Color> P[8] = {
        {rgba(150, 40, 40), rgba(40, 70, 120), rgba(60, 100, 70), rgba(120, 110, 90), rgba(90, 60, 110),
         rgba(170, 140, 60), rgba(70, 70, 76)},
        {rgba(130, 100, 60), rgba(110, 104, 90), rgba(200, 110, 40), rgba(90, 90, 92), rgba(150, 130, 90)},
        {rgba(84, 92, 58), rgba(140, 126, 90), rgba(70, 74, 66)},
        {rgba(28, 36, 64), rgba(30, 30, 34)},
        {rgba(84, 92, 58), rgba(110, 90, 60), rgba(220, 100, 20)},
        {rgba(30, 30, 32), rgba(120, 120, 124), rgba(220, 218, 210), rgba(80, 70, 60)},
        {rgba(50, 70, 110), rgba(70, 90, 130), rgba(40, 46, 60)},
        {rgba(210, 50, 40), rgba(240, 190, 40), rgba(40, 120, 200), rgba(50, 160, 80), rgba(230, 110, 30),
         rgba(240, 240, 236), rgba(200, 60, 140)}};
    const auto& v = P[pal];
    Color c = v[irand(0, (int)v.size() - 1)];
    int j = 10;
    return rgba(c.r + irand(-j, j), c.g + irand(-j, j), c.b + irand(-j, j));
}
static int rollRarity(int mn);
static int setOfDef(int def);
static void makeSetPiece(Cloth& c, int si);
static std::shared_ptr<Cloth> makeCloth(int def, int rar, float condMax = 1) {
    auto c = std::make_shared<Cloth>();
    const CDef& d = CD[def];
    c->def = def;
    c->rar = rar;
    c->cond = std::max(.08f, frand(.15f, condMax));
    c->col = palColor(d.pal);
    c->pattern = d.pal == PL_HUNT || d.pal == PL_MIL                          ? (chance(.6f) ? 2 : 0)
                 : (d.slot == CS_TORSO || d.slot == CS_OUTER) && chance(.25f) ? 1
                 : chance(.15f)                                               ? 3
                                                                              : 0;
    if (c->pattern == 3)
        c->col = lerpC(c->col, rgba(170, 166, 156), .35f);
    static const char* deco[] = {"Cedar Falls Lumber",
                                 "Harlow High Football",
                                 "Millbrook Feed & Seed",
                                 "Fort Calloway",
                                 "Pine Ridge Volunteer FD",
                                 "Dunmore Diner",
                                 "Linn County Fair '24",
                                 "Oregon Ducks",
                                 "Beavers",
                                 "Rogue Brewing"};
    if ((d.slot == CS_TORSO || d.slot == CS_OUTER || d.slot == CS_HEAD) && chance(.3f))
        c->decor = deco[irand(0, 9)];
    static const char* affN[5] = {"Insulated", "Reinforced", "Waterproofed", "Deep-Pocket", "Quiet"};
    std::string pre;
    for (int i = 0; i < rar; i++) {
        int a = irand(0, 4);
        if (a == 3 && !d.gh)
            a = 1;
        switch (a) {
        case 0:
            c->bWarm += .08f;
            break;
        case 1:
            c->bArmor += .06f;
            break;
        case 2:
            c->bWater += .25f;
            break;
        case 3:
            c->extraRows++;
            break;
        case 4:
            c->bQuiet += .1f;
            break;
        }
        if (i == 0)
            pre = std::string(affN[a]) + " ";
    }
    static const char* leg[] = {"Dad's", "The Sheriff's", "Lucky", "Old Glory", "Grandma's", "Bigfoot's"};
    c->name = rar == 3 ? std::string(leg[irand(0, 5)]) + " " + d.n : pre + PATN[c->pattern] + d.n;
    {
        int si = setOfDef(def);
        if (si >= 0 && chance(rar >= 2 ? .45f : rar == 1 ? .12f : .04f))
            makeSetPiece(*c, si);
    }
    return c;
}
static float clothWarm(const Cloth& c) {
    return (CD[c.def].warm + c.bWarm) * (c.cond <= 0 ? .2f : .4f + .6f * c.cond);
}
static float clothArmor(const Cloth& c) {
    return (CD[c.def].armor + c.bArmor) * (c.cond <= 0 ? 0 : .3f + .7f * c.cond);
}
static float clothWater(const Cloth& c) {
    return std::min(.95f, (CD[c.def].water + c.bWater) * (c.cond <= 0 ? .2f : .5f + .5f * c.cond));
}
static int clothFor(int fam, int slotMask = -1) { // weighted pick of a clothing def for a loot family
    float tot = 0;
    for (int i = 0; i < CD_N; i++)
        if ((CD[i].fam & fam) && (slotMask < 0 || (slotMask >> CD[i].slot & 1)))
            tot += CD[i].rare;
    if (tot <= 0)
        return 0;
    float r = frand(0, tot);
    for (int i = 0; i < CD_N; i++)
        if ((CD[i].fam & fam) && (slotMask < 0 || (slotMask >> CD[i].slot & 1))) {
            r -= CD[i].rare;
            if (r <= 0)
                return i;
        }
    return 0;
}
static int cdIndex(const char* n) {
    for (int i = 0; i < CD_N; i++)
        if (!strcmp(CD[i].n, n))
            return i;
    return 0;
}
// ---- class sets: loud colors, bonuses at 2 and 4 pieces, and extra if the set matches your class (set index == class index)
struct SetDef {
    const char* n;
    const char* pieces[5];
    Color c;
    const char* b2;
    const char* b4;
    const char* bc;
};
static const SetDef SETS[5] = {{"Thin Blue Line",
                                {"Riot Helmet", "Police Shirt", "Police Vest", "Tactical Gloves", "Combat Boots"},
                                {40, 110, 255, 255},
                                "+8% armor",
                                "+12% pistol and shotgun damage, +10% reload",
                                "Police: Adrenaline Dump recharges 25% faster"},
                               {"Golden Hour",
                                {"Surgical Mask", "Safety Glasses", "Thermal Top", "Rain Jacket", "Cargo Pants"},
                                {160, 255, 40, 255},
                                "Bandages heal 40% more",
                                "+15% bleed resistance, faster regen",
                                "Paramedic: Trauma Kit recharges 25% faster"},
                               {"Grease Monkey",
                                {"Hard Hat", "Work Shirt", "Work Gloves", "Work Pants", "Work Boots"},
                                {255, 214, 0, 255},
                                "+10% melee damage",
                                "+15% stamina, kicks 25% harder",
                                "Mechanic: Nail Bomb recharges 25% faster"},
                               {"Deep Woods",
                                {"Blaze Orange Cap", "Hunting Coat", "Hunting Pants", "Hiking Boots", "Hunting Pack"},
                                {60, 255, 120, 255},
                                "Spotted 8% later",
                                "+15% rifle damage, 12% quieter",
                                "Hunter: Deadeye recharges 25% faster"},
                               {"County Issue",
                                {"Beanie", "Hoodie", "Sweatpants", "Sneakers", "Bandana"},
                                {255, 110, 0, 255},
                                "Better loot odds",
                                "+12% melee damage, kicks 30% harder",
                                "Ex-Con: Bloodlust recharges 25% faster"}};
static int setOfDef(int def) {
    for (int i = 0; i < 5; i++)
        for (auto p : SETS[i].pieces)
            if (!strcmp(p, CD[def].n))
                return i;
    return -1;
}
static void makeSetPiece(Cloth& c, int si) {
    c.set = si;
    c.col = SETS[si].c;
    c.pattern = 0;
    c.decor.clear();
    c.rar = std::max(c.rar, 2);
    c.cond = std::max(c.cond, .6f);
    c.name = std::string(SETS[si].n) + " " + CD[c.def].n;
}

enum LootKind {
    LK_ITEM,
    LK_GUN,
    LK_MELEE,
    LK_CLOTH
};
struct Loot {
    int kind, id, count;
    Weapon w;
    Vector3 p, v;
    float rot, t = 0;
    bool settled = true;
    std::shared_ptr<Cloth> c;
};
static std::vector<Loot> loot;
static bool playerFemale = false;
static int gunAsset(int key) {
    static const char* k[W_COUNT] = {"items/Pistol_1",  "items/Pistol_2",  "items/Shotgun_ShortStock",
                                     "items/Shotgun_1", "items/Shotgun_1", "items/Shotgun_ShortStock",
                                     "items/Shotgun_2"};
    return AS(k[key]);
}
static float gunScale(int key, int ai) {
    static const float L[W_COUNT] = {.24f, .22f, .6f, 1.0f, .95f, .85f, 1.15f};
    return fitScale(ai, L[key], 0);
}
static int meleeAsset(int key) {
    if (key == MK_KNIFE || key == MK_MACHETE)
        return AS("items/Knife");
    if (key == MK_AXE || key == MK_HATCHET)
        return AS("items/Axe");
    return -1;
}
static float meleeScale(int key, int ai) {
    return fitScale(ai, MB[key].len);
}
static Matrix meleeAlign(int ai) {
    Vector3 s = assets[ai].size;
    return s.y >= s.x && s.y >= s.z ? MatrixRotateZ(-PI / 2) : (s.z > s.x ? MatrixRotateY(PI / 2) : MatrixIdentity());
}
static int itemAsset(int id) {
    switch (id) {
    case IT_BEANS:
        return AS("items/Can_Closed");
    case IT_PEACHES:
    case IT_SODA:
        return AS("items/Can_Red");
    case IT_WATER:
        return AS("items/WaterBottle_1");
    case IT_BANDAGE:
        return AS("items/Bandages");
    case IT_MORPHINE:
    case IT_ANTIBIO:
        return AS("items/FirstAidKit");
    case IT_BATTERY:
        return AS("items/Battery_Small");
    case IT_PIPEBOMB:
        return AS("items/PropaneTank");
    default:
        return -1;
    }
}
static float itemScale(int id, int ai) {
    float t = id == IT_WATER                          ? .26f
              : id == IT_MORPHINE || id == IT_ANTIBIO ? .3f
              : id == IT_PIPEBOMB                     ? .3f
              : id == IT_BANDAGE                      ? .14f
              : id == IT_BATTERY                      ? .09f
              : id == IT_SODA                         ? .12f
                                                      : .13f;
    return fitScale(ai, t);
}
