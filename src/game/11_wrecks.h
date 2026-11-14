// Nightfall County: wrecks.
// Not a standalone header: included once, in order, from src/main.cpp. Everything above it in that list is visible here.
#pragma once
// wrecks: real vehicle models picked by place, burned-out ones charred, some with parts scattered around
static void car(float x, float z, float yaw, float burnP = .25f, int kind = VK_CIVIL) {
    if (kind == VK_CIVIL) {
        float r = frand(0, 1);
        kind = r < .03f ? VK_POLICE : r < .05f ? VK_DELIVERY : r < .14f ? VK_TRUCK : VK_CIVIL;
    }
    const char* key;
    float L;
    switch (kind) {
    case VK_POLICE:
        key = "vehicles/police";
        L = 4.8f;
        break;
    case VK_AMBULANCE:
        key = "vehicles/ambulance";
        L = 6.2f;
        break;
    case VK_FIRE:
        key = "vehicles/firetruck";
        L = 9.f;
        break;
    case VK_DELIVERY:
        key = chance(.5f) ? "vehicles/delivery" : "vehicles/delivery-flat";
        L = 6.6f;
        break;
    case VK_GARBAGE:
        key = "vehicles/garbage-truck";
        L = 8.5f;
        break;
    case VK_TRACTOR:
        key = chance(.7f) ? "vehicles/tractor" : "vehicles/tractor-shovel";
        L = 4.2f;
        break;
    case VK_MILITARY:
        key = chance(.6f) ? "vehicles/truck" : "vehicles/truck-flat";
        L = 6.4f;
        break;
    case VK_TRUCK:
        key = chance(.75f) ? "vehicles/truck" : "vehicles/van";
        L = 5.4f;
        break;
    default: {
        static const char* civ[] = {
            "vehicles/sedan", "vehicles/sedan", "vehicles/sedan-sports", "vehicles/hatchback-sports",
            "vehicles/suv",   "vehicles/suv",   "vehicles/suv-luxury",   "vehicles/van",
            "vehicles/taxi"};
        key = civ[irand(0, 8)];
        L = 4.6f;
    }
    }
    int ai = AS(key);
    bool burn = chance(burnP);
    if (ai < 0) { // fallback box car
        float Wd = 1.9f;
        L = 4.4f;
        Vector2 f = polar(yaw, 1);
        float hx = fabsf(f.x) * L / 2 + fabsf(f.y) * Wd / 2, hz = fabsf(f.y) * L / 2 + fabsf(f.x) * Wd / 2;
        if (!boxFree(x - hx, z - hz, x + hx, z + hz))
            return;
        Color c = burn ? rgba(46, 40, 38) : pick(CAR_C);
        addProp(M_CUBE, V3(x, .55f, z), V3(L, .8f, Wd), yaw, c);
        addProp(M_CUBE, V3(x - f.x * .3f, 1.2f, z - f.y * .3f), V3(2.2f, .6f, Wd * .9f), yaw,
                burn ? rgba(24, 22, 20) : rgba(30, 36, 44));
        addSolid(x - hx, z - hz, x + hx, z + hz, 1.6f, SK_CAR);
        carObjs.push_back({V2(x, z), yaw, burn});
        return;
    }
    const Asset& A = assets[ai];
    bool alongZ = A.size.z >= A.size.x;
    float sc = L / std::max(.1f, alongZ ? A.size.z : A.size.x), my = alongZ ? yaw - PI / 2 : yaw;
    Vector2 fp = aSize(ai, sc, my);
    float hx = fp.x / 2, hz = fp.y / 2;
    if (!boxFree(x - hx, z - hz, x + hx, z + hz))
        return;
    Color tint = WHITE;
    if (kind == VK_MILITARY)
        tint = rgba(150, 165, 110);
    if (burn)
        tint = rgba(70, 58, 50);
    else if (chance(.35f))
        tint = lerpC(tint, rgba(150, 120, 95), frand(.15f, .35f)); // rust and grime
    addAProp(ai, V3(x, 0, z), sc, my, -1, tint);
    addSolid(x - hx, z - hz, x + hx, z + hz, std::min(2.6f, A.size.y * sc), SK_CAR);
    aoRect(x - hx, z - hz, x + hx, z + hz, 1.2f, .45f);
    carObjs.push_back({V2(x, z), yaw, burn});
    if (burn || chance(.25f)) {
        static const char* deb[] = {"vehicles/debris-door", "vehicles/debris-tire", "vehicles/debris-bumper",
                                    "vehicles/debris-plate-a", "vehicles/debris-drivetrain-axle"};
        for (int k = 0; k < irand(1, 3); k++) {
            int di = AS(deb[irand(0, 4)]);
            if (di < 0)
                continue;
            Vector2 dp = V2(x + frand(-hx - 2, hx + 2), z + frand(-hz - 2, hz + 2));
            if (!freeAt(dp.x, dp.y, .5f))
                continue;
            addAProp(di, V3(dp.x, 0, dp.y), sc * frand(.9f, 1.1f), frand(0, 6), -1, burn ? rgba(80, 70, 62) : WHITE);
        }
    }
    if (burn) {
        paintCircle(V2(x, z), 3.5f, rgba(26, 24, 22), .55f);
        aoCircle(V2(x, z), 3.5f, .6f);
    }
    if (burn && chance(.5f))
        staticLights.push_back({V3(x, 1.2f, z), rgba(255, 110, 30), 7, 1});
}
static void serviceCar(int bi, int type) { // a matching vehicle out front of police stations, clinics and stores
    const Building& b = blds[bi];
    float x = (b.x0 + b.x1) / 2 + frand(-2, 2), z = b.z1 + 3.2f;
    int k = type == B_POLICE ? VK_POLICE : type == B_CLINIC ? VK_AMBULANCE : chance(.5f) ? VK_DELIVERY : VK_CIVIL;
    if (type != B_STORE || chance(.6f))
        car(x, z, chance(.5f) ? 0 : PI, .15f, k);
}
static void addTree(Vector2 p, int kind) {
    if (!freeAt(p.x, p.y, 1.2f) || hotAt(p, .85f) >= 0 || inPlot(p, 1.5f))
        return;
    for (int cx = gcl(p.x - 3); cx <= gcl(p.x + 3); cx++)
        for (int cz = gcl(p.y - 3); cz <= gcl(p.y + 3); cz++)
            for (int bi : bgrid[cx][cz]) { // never inside (or hard against) a building
                const Building& b = blds[bi];
                if (p.x > b.x0 - 2 && p.x < b.x1 + 2 && p.y > b.z0 - 2 && p.y < b.z1 + 2)
                    return;
            }
    Tree t;
    t.p = p;
    t.kind = kind;
    t.h = kind == 0 ? frand(8, 14) : frand(6, 10);
    t.r = kind == 0 ? frand(1.8f, 2.8f) : frand(2.4f, 3.8f);
    {
        static const char* pines[] = {"nature/PineTree_1", "nature/PineTree_2", "nature/PineTree_3",
                                      "nature/PineTree_4", "nature/PineTree_5"};
        static const char* broad[] = {"nature/CommonTree_1", "nature/CommonTree_2", "nature/CommonTree_3",
                                      "nature/CommonTree_4", "nature/CommonTree_5", "nature/BirchTree_1",
                                      "nature/BirchTree_2",  "nature/BirchTree_3"};
        const char* key = kind == 0
                              ? (chance(.06f) ? "nature/PineTree_Autumn_1" : pines[irand(0, 4)])
                              : (chance(.07f) ? (chance(.5f) ? "nature/CommonTree_Dead_1" : "nature/CommonTree_Dead_2")
                                 : chance(.08f) ? "nature/CommonTree_Autumn_2"
                                                : broad[irand(0, 7)]);
        int ai = AS(key);
        if (ai >= 0) {
            t.asset = ai;
            t.h = kind == 0 ? frand(9, 16) : frand(7, 12);
            t.sc = t.h / std::max(.1f, assets[ai].size.y);
            t.yaw = frand(0, 2 * PI);
            t.r = std::max(assets[ai].size.x, assets[ai].size.z) * t.sc * .42f;
        }
    }
    t.c = kind == 0 ? rgba(48 + irand(-8, 8), 80 + irand(-10, 10), 50 + irand(-8, 8))
                    : rgba(56 + irand(-10, 10), 84 + irand(-10, 10), 42 + irand(-8, 8));
    trees.push_back(t);
    tgrid[gcl(p.x)][gcl(p.y)].push_back((int)trees.size() - 1);
    addSolid(p.x - .3f, p.y - .3f, p.x + .3f, p.y + .3f, 3, SK_TRUNK);
}
static void addRoad(Vector2 a, Vector2 b, float w, bool town, int tier = 0) {
    roads.push_back({a, b, w, town, tier});
    if (tier == 0) { // paved: a wide, soft shoulder so the grass fades into the asphalt instead of stopping at a line
        curLayer = 5;
        paintSeg(a, b, w / 2 + 4.6f, rgba(104, 96, 78), .3f);
        paintSeg(a, b, w / 2 + 3.0f, rgba(100, 92, 74), .45f);
        curLayer = town ? 9 : 7;
        paintSeg(a, b, w / 2 + 2.2f, rgba(92, 84, 68), .6f);
        curLayer = -1;
    } else if (tier == 1) {
        curLayer = 5;
        paintSeg(a, b, w / 2 + 1.6f, rgba(110, 100, 80), .5f);
        curLayer = 7;
        paintSeg(a, b, w / 2 + .4f, rgba(120, 114, 100), 1.f);
        curLayer = -1;
    } else {
        curLayer = 1;
        paintSeg(a, b, w / 2 + 1.6f, rgba(80, 96, 56), .5f);
        curLayer = 5;
        paintSeg(a, b, w / 2 + .5f, rgba(104, 86, 62), .95f);
        curLayer = 8;
        paintSeg(a, b, w / 2 - .4f, rgba(84, 70, 52), .45f);
        curLayer = -1;
    }
}
static Vector2 nearestRoadPt(Vector2 p, float* dist = nullptr, int maxTier = 2) {
    Vector2 best = p;
    float bd = 1e9f;
    for (auto& r : roads) {
        if (r.tier > maxTier)
            continue;
        Vector2 ab = Vector2Subtract(r.b, r.a);
        float t =
            Clamp(Vector2DotProduct(Vector2Subtract(p, r.a), ab) / std::max(1e-4f, Vector2DotProduct(ab, ab)), 0, 1);
        Vector2 q = Vector2Add(r.a, Vector2Scale(ab, t));
        float d = Vector2Distance(p, q);
        if (d < bd) {
            bd = d;
            best = q;
        }
    }
    if (dist)
        *dist = bd;
    return best;
}
static void
track(Vector2 from, int tier = 2,
      float maxLen = 450) { // dirt/gravel lane out to the nearest road, with a bend so it doesn't look ruled
    float d;
    Vector2 q = nearestRoadPt(from, &d);
    if (d < 3 || d > maxLen)
        return;
    float w = tier == 2 ? 3.4f : 4.2f;
    auto hitsBld = [&](Vector2 a, Vector2 b) {
        float L = Vector2Distance(a, b);
        for (float t = 0; t <= L; t += 1.f) {
            Vector2 p = Vector2Lerp(a, b, t / std::max(.01f, L));
            if (Vector2Distance(p, from) < 2.5f)
                continue;
            for (int bi : bgrid[gcl(p.x)][gcl(p.y)]) {
                const Building& B = blds[bi];
                float m = w / 2 + .4f;
                if (p.x > B.x0 - m && p.x < B.x1 + m && p.y > B.z0 - m && p.y < B.z1 + m)
                    return true;
            }
        }
        return false;
    };
    for (int tryN = 0; tryN < 8; tryN++) {
        float bend = tryN == 0 ? frand(-.12f, .12f) : (tryN % 2 ? 1 : -1) * (.12f + tryN * .06f);
        Vector2 m = Vector2Add(Vector2Lerp(from, q, .5f), Vector2Scale(V2(-(q.y - from.y), q.x - from.x), bend));
        if (hitsBld(from, m) || hitsBld(m, q))
            continue;
        addRoad(from, m, w, false, tier);
        addRoad(m, q, w, false, tier);
        return;
    }
}
static void fence(float x0, float z0, float x1, float z1, Color c, float h = 1.1f) {
    bool horiz = fabsf(z1 - z0) < .01f;
    float len = horiz ? fabsf(x1 - x0) : fabsf(z1 - z0);
    { // split around roads and buildings: build only the runs that stay clear of both
        int n = std::max(1, (int)(len / .5f));
        bool any = false, all = true;
        std::vector<bool> ok(n + 1);
        for (int i = 0; i <= n; i++) {
            float t = (float)i / n;
            Vector2 p = V2(x0 + (x1 - x0) * t, z0 + (z1 - z0) * t);
            ok[i] = roadDist(p) > .5f && !inBuilding(p, .4f); // skip roads and buildings
            any |= !ok[i];
            all &= ok[i];
        }
        if (any) {
            if (!all) {
                int st = -1;
                for (int i = 0; i <= n + 1; i++) {
                    bool g = i <= n && ok[i];
                    if (g && st < 0)
                        st = i;
                    if (!g && st >= 0) {
                        if (i - 1 - st >= 3) {
                            float ta = (float)st / n, tb = (float)(i - 1) / n;
                            fence(x0 + (x1 - x0) * ta, z0 + (z1 - z0) * ta, x0 + (x1 - x0) * tb, z0 + (z1 - z0) * tb, c,
                                  h);
                        }
                        st = -1;
                    }
                }
            }
            return;
        }
    }
    if (h > 2)
        addProp(M_CUBE, V3((x0 + x1) / 2, h / 2, (z0 + z1) / 2), horiz ? V3(len, h, .04f) : V3(.04f, h, len), 0,
                rgba(170, 170, 170), -1, L_WALLDET, TX_CHAIN); // chain-link
    else
        addProp(M_CUBE, V3((x0 + x1) / 2, h / 2, (z0 + z1) / 2), horiz ? V3(len, h, .08f) : V3(.08f, h, len), 0, c);
    for (float s = 0; s <= len; s += 2.5f)
        addProp(M_CUBE, V3(horiz ? std::min(x0, x1) + s : x0, h / 2 + .1f, horiz ? z0 : std::min(z0, z1) + s),
                V3(.14f, h + .2f, .14f), 0, mulC(c, .8f));
    addSolid(horiz ? std::min(x0, x1) : x0 - .06f, horiz ? z0 - .06f : std::min(z0, z1),
             horiz ? std::max(x0, x1) : x0 + .06f, horiz ? z0 + .06f : std::max(z0, z1), h, SK_FENCE);
}
