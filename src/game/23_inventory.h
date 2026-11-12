// Nightfall County: inventory.
// Not a standalone header: included once, in order, from src/main.cpp. Everything above it in that list is visible here.
#pragma once
// ============================================================ inventory
// ---- container grids. Only clothes with pockets hold stuff; lose the jacket and you lose what's in it
static const int CONT_ORDER[4] = {CS_TORSO, CS_LEGS, CS_OUTER, CS_BACK};
static void itemDims(const InvItem& it, int& w, int& h) {
    if (it.kind == 1) {
        w = CD[it.c->def].iw;
        h = CD[it.c->def].ih;
    } else {
        w = ISZ[it.id][0];
        h = ISZ[it.id][1];
    }
    if (it.rot)
        std::swap(w, h);
}
static bool fits(const Cloth& c, int x, int y, int w, int h, const InvItem* ignore = nullptr) {
    if (x < 0 || y < 0 || x + w > gridW(c) || y + h > gridH(c))
        return false;
    for (auto& o : c.items) {
        if (&o == ignore)
            continue;
        int ow, oh;
        itemDims(o, ow, oh);
        if (x < o.x + ow && o.x < x + w && y < o.y + oh && o.y < y + h)
            return false;
    }
    return true;
}
static bool findSpot(const Cloth& c, InvItem& it) {
    for (int r = 0; r < 2; r++) {
        it.rot = r;
        int w, h;
        itemDims(it, w, h);
        for (int y = 0; y + h <= gridH(c); y++)
            for (int x = 0; x + w <= gridW(c); x++)
                if (fits(c, x, y, w, h)) {
                    it.x = x;
                    it.y = y;
                    return true;
                }
    }
    it.rot = false;
    return false;
}
static bool placeAnywhere(const InvItem& src) {
    InvItem it = src;
    for (int s : CONT_ORDER) {
        auto& c = P.eq[s];
        if (c && gridH(*c) > 0 && findSpot(*c, it)) {
            c->items.push_back(it);
            return true;
        }
    }
    return false;
}
template <class F> static void forInv(F f) {
    for (int s : CONT_ORDER)
        if (P.eq[s])
            for (auto& it : P.eq[s]->items)
                f(it);
}
static int invCount(int id) {
    int n = 0;
    forInv([&](InvItem& it) {
        if (it.kind == 0 && it.id == id)
            n += it.n;
    });
    return n;
}
static int invAdd(int id, int n) { // returns leftover
    forInv([&](InvItem& it) {
        if (n > 0 && it.kind == 0 && it.id == id && it.n < IDEF[id].stack) {
            int k = std::min(n, IDEF[id].stack - it.n);
            it.n += k;
            n -= k;
        }
    });
    while (n > 0) {
        InvItem it;
        it.kind = 0;
        it.id = id;
        it.n = std::min(n, IDEF[id].stack);
        if (!placeAnywhere(it))
            break;
        n -= it.n;
    }
    return n;
}
static void invPrune() {
    for (int s : CONT_ORDER)
        if (P.eq[s]) {
            auto& v = P.eq[s]->items;
            v.erase(std::remove_if(v.begin(), v.end(),
                                   [](const InvItem& i) {
                                       return i.kind == 0 && i.n <= 0;
                                   }),
                    v.end());
        }
}
static int invTake(int id, int n) {
    int took = 0;
    for (int s : CONT_ORDER)
        if (P.eq[s])
            for (int i = (int)P.eq[s]->items.size() - 1; i >= 0 && n > 0; i--) {
                auto& it = P.eq[s]->items[i];
                if (it.kind == 0 && it.id == id) {
                    int k = std::min(n, it.n);
                    it.n -= k;
                    n -= k;
                    took += k;
                }
            }
    invPrune();
    return took;
}
static InvItem* invFind(int id) {
    InvItem* r = nullptr;
    forInv([&](InvItem& it) {
        if (!r && it.kind == 0 && it.id == id && it.n > 0)
            r = &it;
    });
    return r;
}
static int setCount(int si) {
    int n = 0;
    for (int s = 0; s < CS_COUNT; s++)
        if (P.eq[s] && P.eq[s]->set == si && P.eq[s]->cond > 0)
            n++;
    return n;
}
struct Gear {
    float warm = 0, water = 0, armor = 0, noise = 1, vis = 1;
    int slots = 0, used = 0;
};
static Gear gear() {
    Gear g;
    float wsum = 0;
    for (int s = 0; s < CS_COUNT; s++) {
        auto& c = P.eq[s];
        if (!c)
            continue;
        const CDef& d = CD[c->def];
        g.warm += clothWarm(*c);
        g.armor += clothArmor(*c);
        float cov = s == CS_OUTER   ? .45f
                    : s == CS_TORSO ? .2f
                    : s == CS_LEGS  ? .2f
                    : s == CS_FEET  ? .1f
                    : s == CS_HEAD  ? .05f
                                    : 0;
        g.water += clothWater(*c) * cov;
        wsum += cov;
        g.noise *= std::max(.6f, d.noise - c->bQuiet);
        g.vis *= d.vis * c->bVis;
        if (gridH(*c)) {
            g.slots += gridW(*c) * gridH(*c);
            for (auto& it : c->items) {
                int w, h;
                itemDims(it, w, h);
                g.used += w * h;
            }
        }
    }
    g.water = std::min(.95f, g.water / .95f);
    return g;
}
static void wearDamage(float dmg) { // a bite eats the outermost layer first
    static const int order[] = {CS_OUTER, CS_TORSO, CS_LEGS, CS_HANDS, CS_HEAD, CS_BACK};
    for (int k = 0; k < 2; k++) {
        int s = order[irand(0, 5)];
        auto& c = P.eq[s];
        if (!c || c->cond <= 0)
            continue;
        float before = c->cond;
        c->cond = std::max(0.f, c->cond - dmg * frand(.004f, .012f) * (1 - std::min(.6f, clothArmor(*c))));
        if (condTier(before) != condTier(c->cond))
            msg("Your " + c->name + " is " + CONDN[condTier(c->cond)] + ".", condTier(c->cond) >= 3 ? BLOODC : STEEL);
        if (c->cond <= 0 && gridH(*c) == 0) {
        }
    }
}
static void dressStart() { // fists, rags, and a bad day
    auto put = [](const char* n, float cmax) {
        auto c = makeCloth(cdIndex(n), 0, cmax);
        c->cond = frand(.3f, cmax);
        c->pattern = 3;
        c->col = lerpC(c->col, rgba(150, 146, 136), .4f);
        c->name = std::string("Ragged ") + CD[c->def].n;
        c->decor.clear();
        P.eq[CD[c->def].slot] = c;
    };
    put("T-Shirt", .6f);
    put(chance(.6f) ? "Jeans" : "Sweatpants", .6f);
    put("Sneakers", .55f);
    put("Cotton Socks", .6f);
    invAdd(IT_RAG, 2);
    invAdd(IT_APPLE, 1);
}
