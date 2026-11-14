// Nightfall County: world data.
// Not a standalone header: included once, in order, from src/main.cpp. Everything above it in that list is visible here.
#pragma once
// ============================================================ world data
constexpr float MAP = 3200.f, GCELL = 32.f, CELLX = MAP + 40.f, WORLDX = MAP + 330.f;
constexpr int GN = (int)(WORLDX / GCELL) + 1; // interiors (basements, upstairs) live in a strip east of the map
constexpr int PPM = 1;
constexpr int GTEX = (int)MAP * PPM;
constexpr float AREA = (MAP / 1600.f) * (MAP / 1600.f); // content scales with area
enum SolidKind {
    SK_WALL,
    SK_FURN,
    SK_CAR,
    SK_TRUNK,
    SK_FENCE,
    SK_ROCK,
    SK_BARREL,
    SK_TENT,
    SK_CRATE,
    SK_TOWER
};
struct Solid {
    float x0, z0, x1, z1, h;
    int kind;
    int bld;
    int q = 0;
    int door = -1;
};
enum Layer {
    L_ALWAYS,
    L_ROOF,
    L_WALL,
    L_WALLDET,
    L_FLOOR,
    L_LOFT
};
struct Prop {
    int mesh;
    Vector3 p, s;
    float yaw;
    Color c;
    int bld;
    int layer;
    int asset = -1;
    int tex = 0;
};
enum BType {
    B_HOUSE,
    B_STORE,
    B_POLICE,
    B_CLINIC,
    B_APART,
    B_GARAGE,
    B_BARN,
    B_BARRACKS,
    B_CHURCH,
    B_CABIN,
    B_OFFICE,
    B_HANGAR,
    B_SHED,
    B_FACTORY,
    B_FIRE,
    B_SCHOOL,
    B_TRAILER,
    B_GAS
};
struct LootSpot {
    Vector3 p;
    int table;
};
struct Door {
    Vector2 out, in;
    int obj = -1;
};
// real doors: closable, block sight and movement, can be beaten down
struct DoorObj {
    Vector2 c;
    bool horiz;
    float w;
    bool open, broken = false;
    float hp = 140, bang = 0;
    int bld;
    int solid;
    float anim = 0;
    bool metal = false;
    float hpMax = 140, pitch = 1, cond = 1;
    Vector2 outN{0, 1};
};
static std::vector<DoorObj> doorObjs;
struct Loft {
    int bld;
    Vector2 base, top;
    float y;
    float x0 = 0, z0 = 0, x1 = 0, z1 = 0;
};
static std::vector<Loft> lofts; // barn haylofts
struct Cover {
    Vector2 p;
    float r;
};
static std::vector<Cover> covers; // tall grass & bushes you can hide in
struct CarObj {
    Vector2 p;
    float yaw;
    bool burned, searched = false;
    int prop = -1; // its model in props (-1: fallback box car)
    int live = 0;  // 0 inert, 1 smouldering (can cook off), 2 about to blow
    float fuse = 0, smokeT = 0;
    bool blew = false; // cooked off this life (saved)
};
static std::vector<CarObj> carObjs;
struct Building {
    float x0, z0, x1, z1, h;
    int type;
    bool enter;
    std::vector<Door> doors;
    std::vector<LootSpot> spots;
    int town;
    float fade = 1;
    float cut = 1;
    int cond = 1;
    int cellKind = 0, parent = -1;
    bool visited = false;
    std::vector<Vector2> gaps;
}; // cond: 0 kept, 1 worn, 2 abandoned
struct Tree {
    Vector2 p;
    float r, h;
    int kind;
    Color c;
    float fade = 1;
    int asset = -1;
    float sc = 1, yaw = 0;
    int chops = 0, logs = 0;
    float shake = 0;
};
struct Road {
    Vector2 a, b;
    float w;
    bool town;
    int tier = 0;
}; // tier: 0 paved, 1 gravel, 2 dirt
enum TKind {
    TK_CITY,
    TK_TOWN,
    TK_VILLAGE,
    TK_MILITARY
};
struct Town {
    const char* name;
    Vector2 c;
    float r;
    int kind;
    unsigned seed;
};
static Town TOWNS[6] = { // positions are rerolled every life; layouts (seeds) never change
    {"Cedar Falls", {820, 760}, 150, TK_CITY, 101},        {"Harlow", {380, 1200}, 95, TK_TOWN, 202},
    {"Millbrook", {1250, 380}, 70, TK_VILLAGE, 303},       {"Pine Ridge", {300, 330}, 62, TK_VILLAGE, 404},
    {"Fort Calloway", {1280, 1260}, 58, TK_MILITARY, 505}, {"Dunmore", {1360, 830}, 55, TK_VILLAGE, 606}};
static std::vector<Solid> solids;
static std::vector<int> sgrid[GN][GN];
static std::vector<Prop> props;
static std::vector<int> pgrid[GN][GN];
static std::vector<Building> blds;
static std::vector<int> bgrid[GN][GN];
static std::vector<Tree> trees;
static std::vector<int> tgrid[GN][GN];
static std::vector<Road> roads;
static std::vector<Vector2> spawnPts;
struct LootSpot;
static std::vector<LootSpot> yardSpots;
struct Lamp {
    Vector3 p;
    Color c;
    float r;
    int kind;
}; // static lights: fires, flares, lamps
static std::vector<Lamp> staticLights;
static Image groundImg;
static Color* gpx = nullptr;
static Mesh chunkMesh[GN][GN];
static bool chunkHas[GN][GN];
static int qid = 0;
static std::vector<int> qbuf;

static int gcl(float v) {
    return std::max(0, std::min(GN - 1, (int)(v / GCELL)));
}
static void addSolid(float x0, float z0, float x1, float z1, float h, int kind, int bld = -1) {
    if (x0 > x1)
        std::swap(x0, x1);
    if (z0 > z1)
        std::swap(z0, z1);
    int idx = (int)solids.size();
    solids.push_back({x0, z0, x1, z1, h, kind, bld});
    for (int cx = gcl(x0); cx <= gcl(x1); cx++)
        for (int cz = gcl(z0); cz <= gcl(z1); cz++)
            sgrid[cx][cz].push_back(idx);
}
static void addProp(int mesh, Vector3 p, Vector3 s, float yaw, Color c, int bld = -1, int layer = L_ALWAYS,
                    int tex = 0) {
    int idx = (int)props.size();
    props.push_back({mesh, p, s, yaw, c, bld, layer});
    props.back().tex = tex;
    pgrid[gcl(p.x)][gcl(p.z)].push_back(idx);
}
static void addAProp(int ai, Vector3 p, float sc, float yaw, int bld = -1, Color tint = WHITE) {
    if (ai < 0)
        return;
    int idx = (int)props.size();
    props.push_back({-1, p, V3(sc, sc, sc), yaw, tint, bld, L_ALWAYS});
    props.back().asset = ai;
    pgrid[gcl(p.x)][gcl(p.z)].push_back(idx);
}
static Vector2 aSize(int ai, float sc, float yaw) {
    Vector3 z = Vector3Scale(assets[ai].size, sc);
    float c = fabsf(cosf(yaw)), s = fabsf(sinf(yaw));
    return V2(z.x * c + z.z * s, z.x * s + z.z * c);
}
static std::vector<int>& query(float x, float z, float r) {
    qid++;
    qbuf.clear();
    for (int cx = gcl(x - r); cx <= gcl(x + r); cx++)
        for (int cz = gcl(z - r); cz <= gcl(z + r); cz++)
            for (int i : sgrid[cx][cz])
                if (solids[i].q != qid) {
                    solids[i].q = qid;
                    qbuf.push_back(i);
                }
    return qbuf;
}
static bool solidOn(const Solid& s) {
    return s.door < 0 || !(doorObjs[s.door].open || doorObjs[s.door].broken);
}
static bool pointSolid(float x, float z, float minH = 0.f) {
    if (x < 0 || z < 0 || x >= MAP || z >= MAP)
        return true;
    for (int i : sgrid[gcl(x)][gcl(z)]) {
        const Solid& s = solids[i];
        if (solidOn(s) && s.h > minH && x >= s.x0 && x <= s.x1 && z >= s.z0 && z <= s.z1)
            return true;
    }
    return false;
}
static bool freeAt(float x, float z, float r) {
    if (x < r || z < r || x > WORLDX - r || z > MAP - r)
        return false;
    for (int i : query(x, z, r)) {
        const Solid& s = solids[i];
        if (!solidOn(s))
            continue;
        float cx = Clamp(x, s.x0, s.x1), cz = Clamp(z, s.z0, s.z1);
        if ((x - cx) * (x - cx) + (z - cz) * (z - cz) < r * r)
            return false;
    }
    return true;
}
static bool boxFree(float x0, float z0, float x1, float z1) {
    for (int i : query((x0 + x1) / 2, (z0 + z1) / 2, std::max(x1 - x0, z1 - z0))) {
        const Solid& s = solids[i];
        if (s.x0 < x1 && s.x1 > x0 && s.z0 < z1 && s.z1 > z0)
            return false;
    }
    return true;
}
static void collide(Vector2& o, float r) {
    for (int i : query(o.x, o.y, r)) {
        const Solid& s = solids[i];
        if (!solidOn(s))
            continue;
        float cx = Clamp(o.x, s.x0, s.x1), cz = Clamp(o.y, s.z0, s.z1), dx = o.x - cx, dz = o.y - cz,
              d2 = dx * dx + dz * dz;
        if (d2 < r * r) {
            if (d2 < 1e-8f) {
                float l = o.x - s.x0, rr = s.x1 - o.x, t = o.y - s.z0, b = s.z1 - o.y, m = std::min({l, rr, t, b});
                if (m == l)
                    o.x = s.x0 - r;
                else if (m == rr)
                    o.x = s.x1 + r;
                else if (m == t)
                    o.y = s.z0 - r;
                else
                    o.y = s.z1 + r;
            } else {
                float d = sqrtf(d2);
                o.x += dx / d * (r - d);
                o.y += dz / d * (r - d);
            }
        }
    }
    o.x = Clamp(o.x, r, (o.x > MAP + 10 ? WORLDX : MAP) - r);
    o.y = Clamp(o.y, r, MAP - r);
}
static bool losClear(Vector2 a, Vector2 b) {
    float d = Vector2Distance(a, b);
    int n = (int)(d / .18f) + 1; // walls and doors are ~.25 m thick: step finer than that
    for (int i = 1; i < n; i++) {
        Vector2 p = Vector2Lerp(a, b, (float)i / n);
        if (pointSolid(p.x, p.y, 1.4f))
            return false;
    }
    return true;
}
static bool wallBetween(Vector2 a, Vector2 b) { // a wall or closed door in the way (also works upstairs/basements)
    float d = Vector2Distance(a, b);
    int n = (int)(d / .1f) + 1;
    for (int i = 1; i < n; i++) {
        Vector2 p = Vector2Lerp(a, b, (float)i / n);
        for (int si : sgrid[gcl(p.x)][gcl(p.y)]) {
            const Solid& s = solids[si];
            if (s.kind == SK_WALL && solidOn(s) && p.x >= s.x0 && p.x <= s.x1 && p.y >= s.z0 && p.y <= s.z1)
                return true;
        }
    }
    return false;
}
static bool inBuilding(Vector2 p, float m) { // inside any building's footprint (plus margin m), enterable or not
    for (int bi : bgrid[gcl(p.x)][gcl(p.y)]) {
        const Building& b = blds[bi];
        if (p.x > b.x0 - m && p.x < b.x1 + m && p.y > b.z0 - m && p.y < b.z1 + m)
            return true;
    }
    return false;
}
static int buildingAt(Vector2 p) {
    if (p.x < 0 || p.y < 0 || p.x >= WORLDX || p.y >= MAP)
        return -1;
    for (int i : bgrid[gcl(p.x)][gcl(p.y)]) {
        const Building& b = blds[i];
        if (b.enter && p.x > b.x0 && p.x < b.x1 && p.y > b.z0 && p.y < b.z1)
            return i;
    }
    return -1;
}
// hot zones: rerolled each life, denser and nastier, better caches
enum HZKind {
    HZ_CHECKPOINT,
    HZ_MEDEVAC,
    HZ_CONVOY,
    HZ_GRAVE,
    HZ_EVAC,
    HZ_COUNT
};
static const char* HZN[HZ_COUNT] = {"Quarantine Checkpoint", "Crashed Medevac", "Burned Convoy", "Mass Grave",
                                    "Overrun Evac Point"};
static const char* HZD[HZ_COUNT] = {
    "The Guard held here for a day. Then it didn't.", "Somebody's meds are still in the wreck.",
    "A dozen trucks, no survivors, plenty of ammo.", "They ran out of lime before they ran out of bodies.",
    "The last helicopter left without them."};
struct HotZone {
    Vector2 c;
    float r;
    int kind;
    bool seen = false;
};
static std::vector<HotZone> hotZones;
static std::vector<Rectangle>
    plotRects; // countryside yards (farms, homesteads...): kept clear of trees, rocks, other lots
static bool inPlot(Vector2 p, float m) {
    for (auto& r : plotRects)
        if (p.x > r.x - m && p.x < r.x + r.width + m && p.y > r.y - m && p.y < r.y + r.height + m)
            return true;
    return false;
}
static int hotAt(Vector2 p, float k = 1.f) {
    for (int i = 0; i < (int)hotZones.size(); i++)
        if (Vector2Distance(p, hotZones[i].c) < hotZones[i].r * k)
            return i;
    return -1;
}
static int townAt(Vector2 p, float k = 1.15f) {
    for (int i = 0; i < 6; i++)
        if (Vector2Distance(p, TOWNS[i].c) < TOWNS[i].r * k)
            return i;
    return -1;
}
static float roadDist(Vector2 p) {
    float m = 1e9f;
    for (auto& r : roads)
        m = std::min(m, segDist(p, r.a, r.b) - r.w / 2);
    return m;
}
// true if any road (plus its shoulder) touches the rectangle: nothing gets built on the asphalt
static bool rectOnRoad(float x0, float z0, float x1, float z1, float pad = .8f) {
    for (auto& r : roads) {
        float hw = r.w / 2 + pad;
        if (std::max(r.a.x, r.b.x) + hw < x0 || std::min(r.a.x, r.b.x) - hw > x1 || std::max(r.a.y, r.b.y) + hw < z0 ||
            std::min(r.a.y, r.b.y) - hw > z1)
            continue;
        float len = Vector2Distance(r.a, r.b);
        int n = std::max(1, (int)(len / .75f));
        for (int i = 0; i <= n; i++) {
            Vector2 p = Vector2Lerp(r.a, r.b, (float)i / n);
            float cx = Clamp(p.x, x0, x1), cz = Clamp(p.y, z0, z1);
            if ((p.x - cx) * (p.x - cx) + (p.y - cz) * (p.y - cz) < hw * hw)
                return true;
        }
    }
    return false;
}
