// Nightfall County: building parts.
// Not a standalone header: included once, in order, from src/main.cpp. Everything above it in that list is visible here.
#pragma once
// ------------------------------------------------------------ building construction
static const std::vector<Color> HOUSE_WALL = {rgba(176, 168, 150), rgba(150, 160, 168), rgba(190, 180, 158),
                                              rgba(140, 150, 130), rgba(170, 120, 100), rgba(200, 196, 184)};
static const std::vector<Color> ROOF_C = {rgba(70, 60, 58), rgba(60, 66, 74), rgba(92, 58, 48), rgba(56, 60, 56),
                                          rgba(80, 78, 76)};
static const std::vector<Color> CAR_C = {rgba(120, 36, 36), rgba(40, 64, 100), rgba(90, 94, 100),  rgba(130, 118, 84),
                                         rgba(46, 86, 66),  rgba(38, 40, 44),  rgba(170, 172, 176)};
enum LootTable {
    LT_HOUSE,
    LT_KITCHEN,
    LT_STORE,
    LT_POLICE,
    LT_MED,
    LT_GARAGE,
    LT_BARN,
    LT_MIL,
    LT_OFFICE,
    LT_CACHE,
    LT_WOODPILE,
    LT_CAR,
    LT_WARDROBE,
    LT_SHED,
    LT_FACTORY,
    LT_FIRE,
    LT_SCHOOL,
    LT_GAS,
    LT_TRAILER,
    LT_HUNT,
    LT_CLOTHES,
    LT_BASEMENT
};

static int curWallTex = 0, curCond = 1;
static Color houseColor() { // paint: whites, creams, pastels, earth tones, with jitter
    static const Color P[] = {rgba(222, 218, 206), rgba(232, 222, 190), rgba(186, 204, 214), rgba(170, 190, 160),
                              rgba(226, 206, 140), rgba(160, 160, 158), rgba(176, 96, 80),   rgba(140, 110, 84),
                              rgba(176, 210, 192), rgba(206, 170, 150), rgba(110, 126, 140), rgba(150, 70, 60)};
    Color c = P[irand(0, 11)];
    int j = 14;
    return rgba(c.r + irand(-j, j), c.g + irand(-j, j), c.b + irand(-j, j));
}
struct FurnBounds {
    bool on = false;
    float x0, z0, x1, z1;
} furnB;
static void wallSeg(float ax, float az, float bx, float bz, float H, Color c, int bi, bool exterior, bool windows,
                    float outN) {
    const float t = .22f;
    bool horiz = fabsf(az - bz) < .01f;
    float len = horiz ? fabsf(bx - ax) : fabsf(bz - az);
    if (len < .05f)
        return;
    float cx = (ax + bx) / 2, cz = (az + bz) / 2;
    addProp(M_CUBE, V3(cx, H / 2, cz), horiz ? V3(len + t, H, t) : V3(t, H, len + t), 0, c, bi, L_WALL, curWallTex);
    if (horiz)
        addSolid(std::min(ax, bx) - t / 2, az - t / 2, std::max(ax, bx) + t / 2, az + t / 2, H, SK_WALL, bi);
    else
        addSolid(ax - t / 2, std::min(az, bz) - t / 2, ax + t / 2, std::max(az, bz) + t / 2, H, SK_WALL, bi);
    if (exterior && windows && len > 2.6f) {
        int nw = (int)(len / 3.2f);
        for (int i = 0; i < nw; i++) {
            float f = (i + .5f) / nw;
            float wx = horiz ? std::min(ax, bx) + f * len : ax, wz = horiz ? az : std::min(az, bz) + f * len;
            static const float bp[3] = {.02f, .18f, .55f}, brk[3] = {0, .15f, .3f};
            bool board = chance(bp[curCond]), broken = !board && chance(brk[curCond]);
            Color wc = board ? rgba(110, 86, 60) : broken ? rgba(12, 12, 14) : rgba(58, 74, 92);
            Vector3 p = horiz ? V3(wx, 1.75f, wz + outN * (t / 2 + .02f)) : V3(wx + outN * (t / 2 + .02f), 1.75f, wz);
            Vector3 fr = horiz ? V3(wx, 1.75f, wz + outN * (t / 2 + .01f)) : V3(wx + outN * (t / 2 + .01f), 1.75f, wz);
            addProp(M_CUBE, fr, horiz ? V3(1.26f, 1.16f, .04f) : V3(.04f, 1.16f, 1.26f), 0, rgba(214, 210, 200), bi,
                    L_WALLDET);
            addProp(M_CUBE, p, horiz ? V3(1.1f, 1.0f, .05f) : V3(.05f, 1.0f, 1.1f), 0, wc, bi, L_WALLDET,
                    board ? TX_PLYWOOD : 0);
            Vector3 sill =
                horiz ? V3(wx, 1.22f, wz + outN * (t / 2 + .06f)) : V3(wx + outN * (t / 2 + .06f), 1.22f, wz);
            addProp(M_CUBE, sill, horiz ? V3(1.3f, .08f, .12f) : V3(.12f, .08f, 1.3f), 0, mulC(c, .8f), bi, L_WALLDET);
        }
    }
}
// model furniture: facing 0=+Z(south) 1=-X(west) 2=-Z(north) 3=+X(east); keeps footprint inside building
static bool furnA(int bi, const char* key, float x, float z, int facing, int table, float sc = .5f, bool solid = true) {
    int ai = AS(key);
    if (ai < 0)
        return false;
    const Building& B = blds[bi];
    static const float TH[4] = {0, -PI / 2, PI, PI / 2};
    float yaw = -TH[facing & 3];
    if (curCond == 2 && chance(.3f))
        return true; // looted, burned, hauled off
    if (curCond == 2 && chance(.3f))
        yaw += frand(-.8f, .8f); // shoved around
    Vector2 fp = aSize(ai, sc, yaw);
    float bx0 = furnB.on ? furnB.x0 : B.x0, bz0 = furnB.on ? furnB.z0 : B.z0, bx1 = furnB.on ? furnB.x1 : B.x1,
          bz1 = furnB.on ? furnB.z1 : B.z1;
    x = Clamp(x, bx0 + fp.x / 2 + .14f, std::max(bx0 + fp.x / 2 + .14f, bx1 - fp.x / 2 - .14f));
    z = Clamp(z, bz0 + fp.y / 2 + .14f, std::max(bz0 + fp.y / 2 + .14f, bz1 - fp.y / 2 - .14f));
    float h = assets[ai].size.y * sc;
    addAProp(ai, V3(x, .06f, z), sc, yaw, bi);
    if (solid)
        addSolid(x - fp.x / 2, z - fp.y / 2, x + fp.x / 2, z + fp.y / 2, h, SK_FURN, bi);
    if (table >= 0)
        blds[bi].spots.push_back({V3(x, std::min(h, 1.0f) + .08f, z), table});
    return true;
}
static void furn(int bi, float x, float z, float w, float d, float h, Color c, int table, bool solid = true) {
    addProp(M_CUBE, V3(x, h / 2 + .06f, z), V3(w, h, d), 0, c, bi, L_ALWAYS);
    if (solid)
        addSolid(x - w / 2, z - d / 2, x + w / 2, z + d / 2, h, SK_FURN, bi);
    if (table >= 0)
        blds[bi].spots.push_back({V3(x, h + .1f, z), table});
}
static int makeDoor(Vector2 c, bool horiz, float w, int bi) {
    int di = (int)doorObjs.size();
    DoorObj d;
    d.c = c;
    d.horiz = horiz;
    d.w = w;
    d.bld = bi;
    int ty = blds[bi].type, cnd = blds[bi].cond;
    d.metal =
        (ty == B_POLICE || ty == B_STORE || ty == B_CLINIC || ty == B_GARAGE || ty == B_HANGAR || ty == B_BARRACKS ||
         ty == B_OFFICE || ty == B_FACTORY || ty == B_FIRE || ty == B_SCHOOL || ty == B_GAS || ty == B_TRAILER)
            ? chance(.7f)
        : ty == B_SHED ? chance(.4f)
                       : chance(.1f);
    d.cond = Clamp(frand(.35f, 1.f) - cnd * .25f, .1f, 1);
    d.hpMax = d.hp = (d.metal ? frand(260, 420) : frand(90, 170)) * (.4f + .6f * d.cond) * (w > 3 ? 1.6f : 1);
    d.pitch = frand(.85f, 1.15f);
    static const float openP[3] = {.15f, .45f, .45f};
    d.open = chance(openP[cnd]);
    if (cnd == 2 && chance(.35f)) {
        d.broken = true;
        d.hp = 0;
    }
    float t = .14f;
    if (horiz)
        addSolid(c.x - w / 2, c.y - t, c.x + w / 2, c.y + t, 2.6f, SK_WALL, bi);
    else
        addSolid(c.x - t, c.y - w / 2, c.x + t, c.y + w / 2, 2.6f, SK_WALL, bi);
    d.solid = (int)solids.size() - 1;
    solids.back().door = di;
    d.anim = d.open ? 1 : 0;
    doorObjs.push_back(d);
    return di;
}
static void woodpile(Vector2 p, int bi) { // stacked logs; the yard spot rolls logs, kindling, matches, often an axe
    if (!boxFree(p.x - 1, p.y - .5f, p.x + 1, p.y + .5f) || rectOnRoad(p.x - 1, p.y - .5f, p.x + 1, p.y + .5f))
        return;
    for (int k = 0; k < 3; k++)
        addProp(M_CUBE, V3(p.x, .2f + k * .32f, p.y), V3(1.8f - k * .3f, .3f, .7f), 0,
                mulC(rgba(110, 76, 48), .85f + k * .08f), bi);
    addSolid(p.x - .9f, p.y - .35f, p.x + .9f, p.y + .35f, 1, SK_CRATE);
    yardSpots.push_back({V3(p.x + 1.3f, .1f, p.y), LT_WOODPILE});
}
