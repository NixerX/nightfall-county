// Nightfall County: traders and barter.
// Not a standalone header: included once, in order, from src/main.cpp. Everything above it in that list is visible here.
// Lower files reach this through forward declarations (interaction, update, render, map, HUD, save, new game).
#pragma once
// ============================================================ traders
// Kinds: 0 town trader (a stall in most towns), 1 wandering trader (walks town to town),
// 2 survivor (reserved: rescued survivors will open the same barter screen via openTrade once they exist).
// Barter by value: their goods count at full value, yours at their buy rate (they need a cut). No currency.

// ---- values (per unit). Tune here.
static const float IVAL[IT_COUNT] = {6, 3, 4, 5,  6,    3, 6,  30, 10, 25, 8, 6, 22, 1, 2, 2,
                                     4, 2, 1, 10, .25f, 2, 10, 14, 6,  8,  4, 4, 4,  6, 15};
static const float GVAL[W_COUNT] = {40, 45, 90, 70, 110, 130, 100};
static const float GRAR[4] = {1, 1.7f, 2.8f, 5};
static const float MVAL[MK_COUNT] = {0, 8, 30, 40, 15, 18, 22, 14, 10};
constexpr float BUY_TOWN = .65f, BUY_WANDER = .75f, BUY_FRIEND = .15f; // what they pay for your stuff
constexpr float RAID_RANGE = 110, RAID_ABANDON = 140;

struct TOffer {
    int kind = 0; // 0 item, 1 gun, 2 melee
    int id = 0, n = 1;
    Weapon w;
};
struct Trader {
    int kind = 0;
    std::string name;
    int town = -1, look = -1;
    Vector2 p{}, stall{}, to{};
    float a = 0, hp = 160, rest = 0, walk = 0, shotCd = 0, hurtT = 0, stuckT = 0, flash = 0;
    Vector2 stuckP{};
    bool alive = true, friendly = false, raid = false, moving = false;
    int stockDay = 0, raidDay = 0;
    std::vector<TOffer> stock;
    std::vector<int> raidIds;
    std::map<int, float> biteT;
    PoseState ps;
};
static std::vector<Trader> traders;
static int tradeWith = -1;

static float offerVal(const TOffer& o) {
    if (o.kind == 0)
        return IVAL[o.id];
    if (o.kind == 1)
        return GVAL[o.w.key] * GRAR[o.w.rar];
    return MVAL[o.id];
}
static std::string offerName(const TOffer& o) {
    if (o.kind == 0)
        return IDEF[o.id].n;
    if (o.kind == 1)
        return o.w.name;
    return MB[o.id].n;
}
static float buyRate(const Trader& t) {
    return (t.kind == 1 ? BUY_WANDER : BUY_TOWN) + (t.friendly ? BUY_FRIEND : 0);
}
static void stockAdd(Trader& t, int id, int n) {
    for (auto& o : t.stock)
        if (o.kind == 0 && o.id == id) {
            o.n += n;
            return;
        }
    TOffer o;
    o.kind = 0;
    o.id = id;
    o.n = n;
    t.stock.push_back(o);
}
static void restock(Trader& t) {
    t.stock.clear();
    struct S {
        int id;
        float ch;
        int lo, hi;
    };
    static const S town[] = {
        {IT_BEANS, .9f, 2, 6},    {IT_PEACHES, .6f, 1, 4}, {IT_CHIPS, .5f, 1, 4},     {IT_APPLE, .5f, 2, 6},
        {IT_WATER, .9f, 2, 6},    {IT_SODA, .5f, 1, 5},    {IT_BANDAGE, .9f, 2, 6},   {IT_RAG, .7f, 2, 6},
        {IT_PAINKILL, .7f, 1, 4}, {IT_ANTIBIO, .4f, 1, 2}, {IT_MORPHINE, .15f, 1, 1}, {IT_SPLINT, .5f, 1, 2},
        {IT_A9, .8f, 15, 45},     {IT_A12, .7f, 6, 18},    {IT_A556, .4f, 15, 40},    {IT_A308, .35f, 5, 15},
        {IT_BATTERY, .7f, 1, 3},  {IT_FLARE, .5f, 1, 3},   {IT_MATCHES, .7f, 10, 30}, {IT_TAPE, .4f, 1, 1},
        {IT_SEWKIT, .3f, 1, 1},   {IT_LEATHER, .4f, 1, 3}, {IT_ROPE, .3f, 1, 1},      {IT_GAS, .25f, 1, 1},
        {IT_PIPEBOMB, .2f, 1, 2}};
    static const S wander[] = {{IT_BEANS, .5f, 1, 3},      {IT_WATER, .5f, 1, 3},      {IT_BANDAGE, .6f, 1, 4},
                               {IT_ANTIBIO, .7f, 1, 3},    {IT_MORPHINE, .5f, 1, 2},   {IT_PAINKILL, .6f, 1, 3},
                               {IT_A556, .6f, 20, 60},     {IT_A308, .6f, 10, 25},     {IT_A12, .5f, 8, 20},
                               {IT_PIPEBOMB, .5f, 1, 3},   {IT_WAX, .4f, 1, 1},        {IT_DYE_BLACK, .25f, 1, 1},
                               {IT_DYE_GREEN, .25f, 1, 1}, {IT_DYE_BROWN, .25f, 1, 1}, {IT_SEWKIT, .4f, 1, 1},
                               {IT_TAPE, .5f, 1, 1},       {IT_GAS, .4f, 1, 1},        {IT_BATTERY, .6f, 1, 3}};
    bool w = t.kind == 1;
    float more = !w && t.town >= 0 && TOWNS[t.town].kind == TK_CITY ? 1.3f : 1;
    const S* tb = w ? wander : town;
    int tn = w ? (int)(sizeof wander / sizeof *wander) : (int)(sizeof town / sizeof *town);
    for (int i = 0; i < tn; i++)
        if (chance(std::min(1.f, tb[i].ch * more)))
            stockAdd(t, tb[i].id, irand(tb[i].lo, tb[i].hi));
    int nm = w ? irand(0, 1) : irand(1, 2);
    for (int i = 0; i < nm; i++) {
        TOffer o;
        o.kind = 2;
        o.id = irand(MK_KNIFE, MK_COUNT - 1);
        t.stock.push_back(o);
    }
    int ng = w ? irand(1, 2) : (chance(.5f * more) ? 1 : 0);
    for (int i = 0; i < ng; i++) {
        static const int townG[] = {W_M9, W_GLOCK, W_SHOTGUN, W_SHOTGUN, W_HUNT, W_MP5};
        static const int wandG[] = {W_MP5, W_AR15, W_M4, W_HUNT, W_SHOTGUN, W_GLOCK};
        int key = w ? wandG[irand(0, 5)] : townG[irand(0, 5)];
        float r = frand(0, 1);
        int rar = w ? (r < .1f ? 3 : r < .4f ? 2 : 1) : (r < .05f ? 2 : r < .3f ? 1 : 0);
        TOffer o;
        o.kind = 1;
        o.id = key;
        o.w = makeWeapon(key, rar);
        o.w.clip = 0;
        t.stock.push_back(o);
    }
    t.stockDay = dayN();
}

static const std::vector<std::string> TRADER_NAMES = {"Mags", "Hank", "Dolores", "Earl",   "Rosie", "Lyle",
                                                      "Bev",  "Otis", "June",    "Curtis", "Darla", "Wade"};
static const std::vector<std::string> DRIFTER_NAMES = {"Drifter Ray", "Old Sal", "Packrat Jim", "Tinker Lou"};
static int survivorLook() {
    if (!lookPool[6].empty())
        return lookPool[6][irand(0, (int)lookPool[6].size() - 1)];
    return playerLooks[0];
}
static Vector2 townSpot(int ti, float rk) {
    const Town& T = TOWNS[ti];
    for (int k = 0; k < 120; k++) {
        Vector2 q = Vector2Add(T.c, polar(frand(0, 2 * PI), T.r * rk * sqrtf(frand(0, 1))));
        if (freeAt(q.x, q.y, 2.4f) && buildingAt(q) < 0 && !inBuilding(q, 2))
            return q;
    }
    return T.c;
}
static void genTraders() { // called at the end of newGame: every life gets its own traders
    traders.clear();
    tradeWith = -1;
    std::vector<int> cand;
    for (int i = 0; i < 6; i++)
        if (TOWNS[i].kind != TK_MILITARY)
            cand.push_back(i);
    std::vector<std::string> names = TRADER_NAMES;
    int placed = 0;
    for (int ci = 0; ci < (int)cand.size(); ci++) {
        int ti = cand[ci], after = (int)cand.size() - ci - 1;
        if (!chance(.8f) && placed + after >= 3)
            continue; // most towns have one; never fewer than three
        Trader t;
        t.kind = 0;
        t.town = ti;
        int ni = irand(0, (int)names.size() - 1);
        t.name = names[ni];
        names.erase(names.begin() + ni);
        t.stall = townSpot(ti, .45f);
        t.a = atan2f(TOWNS[ti].c.y - t.stall.y, TOWNS[ti].c.x - t.stall.x);
        t.p = Vector2Add(t.stall, polar(t.a, 1.1f)); // stands in front of the stall
        {                                            // the table blocks you (box around the turned table)
            float c = fabsf(cosf(t.a)), sn = fabsf(sinf(t.a)), hx = c * .4f + sn * .9f, hz = sn * .4f + c * .9f;
            addSolid(t.stall.x - hx, t.stall.y - hz, t.stall.x + hx, t.stall.y + hz, .9f, SK_CAR);
        }
        t.look = survivorLook();
        restock(t);
        traders.push_back(t);
        placed++;
    }
    { // one wanderer per life
        Trader t;
        t.kind = 1;
        t.name = pick(DRIFTER_NAMES);
        int ti = pick(cand);
        t.p = townSpot(ti, .8f);
        t.to = t.p;
        t.rest = frand(20, 60);
        t.look = survivorLook();
        t.hp = 220;
        restock(t);
        traders.push_back(t);
    }
}
static int nearTraderIdx() {
    int best = -1;
    float bd = 2.8f;
    for (int i = 0; i < (int)traders.size(); i++) {
        const Trader& t = traders[i];
        if (!t.alive || t.raid)
            continue;
        float d = Vector2Distance(t.p, P.p);
        if (d < bd && !wallBetween(P.p, t.p)) {
            bd = d;
            best = i;
        }
    }
    return best;
}
static std::string traderLabel(int i) {
    return "Trade with " + traders[i].name;
}
static std::string traderWhere(const Trader& t) {
    if (t.kind == 1)
        return "Wandering trader";
    if (t.kind == 2)
        return "Survivor";
    return std::string("Trader, ") + (t.town >= 0 ? TOWNS[t.town].name : "the county");
}

// ---- barter screen state
static std::vector<int> theirQ; // per stock entry
static int yItemQ[IT_COUNT], yGunQ[4];
static bool yMeleeQ = false;
static float scrollL = 0, scrollR = 0;
static void openTrade(int i) {
    if (i < 0 || i >= (int)traders.size() || !traders[i].alive || traders[i].raid)
        return;
    Trader& t = traders[i];
    if (t.kind == 0 && t.stockDay < dayN())
        restock(t); // town stalls restock every morning
    tradeWith = i;
    theirQ.assign(t.stock.size(), 0);
    memset(yItemQ, 0, sizeof yItemQ);
    memset(yGunQ, 0, sizeof yGunQ);
    yMeleeQ = false;
    scrollL = scrollR = 0;
    P.autorun = false;
    state = ST_TRADE;
    sfx(SFX_PICK, .6f);
}
static void closeTrade() {
    tradeWith = -1;
    if (state == ST_TRADE)
        state = ST_PLAY;
}
static void dropGunAt(Vector2 p, const Weapon& w) {
    Loot l;
    l.kind = LK_GUN;
    l.id = w.key;
    l.count = 1;
    l.w = w;
    l.p = xz3(p, .05f);
    l.rot = frand(0, 6);
    loot.push_back(l);
}
static float tradeGive(const Trader& t) { // what your side is worth to them
    float v = 0;
    for (int id = 0; id < IT_COUNT; id++)
        v += IVAL[id] * yItemQ[id];
    for (int s : {0, 1, 3})
        if (yGunQ[s] && P.hasGun[s])
            v += GVAL[P.gun[s].key] * GRAR[P.gun[s].rar];
    if (yMeleeQ && P.melee != MK_FISTS)
        v += MVAL[P.melee];
    return v * buyRate(t);
}
static float tradeGet(const Trader& t) {
    float v = 0;
    for (int i = 0; i < (int)t.stock.size() && i < (int)theirQ.size(); i++)
        v += offerVal(t.stock[i]) * theirQ[i];
    return v;
}
static void doTrade() {
    Trader& t = traders[tradeWith];
    Vector2 feet = P.p;
    // your side goes to them
    for (int id = 0; id < IT_COUNT; id++)
        if (yItemQ[id] > 0) {
            int got = invTake(id, yItemQ[id]);
            if (got > 0)
                stockAdd(t, id, got);
        }
    for (int s : {0, 1, 3})
        if (yGunQ[s] && P.hasGun[s]) {
            TOffer o;
            o.kind = 1;
            o.id = P.gun[s].key;
            o.w = P.gun[s];
            t.stock.push_back(o);
            P.hasGun[s] = false;
            if (P.active == s) {
                P.active = 2;
                P.reload = 0;
            }
        }
    if (yMeleeQ && P.melee != MK_FISTS) {
        TOffer o;
        o.kind = 2;
        o.id = P.melee;
        t.stock.push_back(o);
        P.melee = MK_FISTS;
    }
    // their side comes to you (whatever doesn't fit lands at your feet)
    bool dropped = false;
    for (int i = 0; i < (int)theirQ.size() && i < (int)t.stock.size(); i++) {
        int q = theirQ[i];
        if (q <= 0)
            continue;
        TOffer& o = t.stock[i];
        if (o.kind == 0) {
            int left = invAdd(o.id, q);
            if (left > 0) {
                spawnItem(xz3(feet, .05f), o.id, left);
                dropped = true;
            }
            o.n -= q;
        } else if (o.kind == 1) {
            int s = o.w.slot;
            if (s == 0)
                s = !P.hasGun[0] ? 0 : !P.hasGun[3] ? 3 : -1;
            else if (P.hasGun[s])
                s = -1;
            if (s < 0) {
                dropGunAt(feet, o.w);
                dropped = true;
            } else {
                P.gun[s] = o.w;
                P.hasGun[s] = true;
            }
            o.n = 0;
        } else {
            if (P.melee != MK_FISTS) { // the one in your hand goes to your feet (sold ones are already fists)
                spawnMelee(xz3(feet, .05f), P.melee);
                dropped = true;
            }
            P.melee = o.id;
            o.n = 0;
        }
    }
    t.stock.erase(std::remove_if(t.stock.begin(), t.stock.end(),
                                 [](const TOffer& o) {
                                     return o.n <= 0;
                                 }),
                  t.stock.end());
    theirQ.assign(t.stock.size(), 0);
    memset(yItemQ, 0, sizeof yItemQ);
    memset(yGunQ, 0, sizeof yGunQ);
    yMeleeQ = false;
    sfx(SFX_LOOT, .8f);
    msg("Deal with " + t.name + "." + (dropped ? " No room for some of it: it's at your feet." : ""), OKC);
}

// ---- raids: town traders sometimes get overrun and need a hand
static Zombie* zById(int id) {
    for (auto& z : zombies)
        if (z.id == id && !z.dead && !z.gone)
            return &z;
    return nullptr;
}
static void startRaid(Trader& t) {
    float dk = cfg.diff == 0 ? .75f : cfg.diff >= 2 ? 1.2f : 1;
    int n = std::min(26, (int)((8 + 2 * dayN()) * dk));
    bool brute = (cfg.diff >= 2 && dayN() >= 3) || (cfg.diff == 1 && dayN() >= 4 && chance(.3f));
    float a0 = frand(0, 2 * PI);
    t.raidIds.clear();
    t.biteT.clear();
    for (int i = 0; i < n; i++) {
        float ang = a0 + (i % 2 ? PI * frand(.6f, 1.2f) : 0) + frand(-.6f, .6f); // from one or two sides
        for (int k = 0; k < 20; k++) {
            Vector2 q = Vector2Add(t.p, polar(ang, frand(22, 36)));
            if (freeAt(q.x, q.y, .5f) && buildingAt(q) < 0 && Vector2Distance(q, P.p) > 12) {
                int type = brute && i == 0 ? Z_BRUTE : chance(.25f) ? Z_RUNNER : Z_WALKER;
                Zombie& z = spawnZ(type, q);
                z.aggro = 0;
                z.np = t.p;
                z.noiseT = 60;
                t.raidIds.push_back(z.id);
                break;
            }
        }
    }
    if (t.raidIds.empty())
        return;
    t.raid = true;
    if (tradeWith >= 0 && &traders[tradeWith] == &t)
        closeTrade();
    showBanner("TRADER UNDER ATTACK", t.name + " is being overrun in " + TOWNS[t.town].name + ". Help hold them off.",
               rgba(255, 90, 60));
    sfx(SFX_STING, 1, 0);
}
static void endRaid(Trader& t, bool won) {
    t.raid = false;
    t.raidIds.clear();
    t.biteT.clear();
    if (!won)
        return;
    t.friendly = true;
    Vector2 rp = Vector2Add(t.p, polar(t.a + PI / 2, 1.3f));
    static const int keys[] = {W_SHOTGUN, W_HUNT, W_AR15, W_MP5, W_GLOCK};
    Weapon w = makeWeapon(keys[irand(0, 4)], std::max(1, rollRarity()));
    w.clip = w.mag;
    dropGunAt(rp, w);
    spawnItem(xz3(Vector2Add(rp, V2(.4f, .3f)), .05f), IT_ANTIBIO, 1);
    spawnItem(xz3(Vector2Add(rp, V2(-.3f, .4f)), .05f), IT_BANDAGE, 3);
    spawnItem(xz3(Vector2Add(rp, V2(.2f, -.4f)), .05f), ammoItem(w.cal), w.cal == C_12GA || w.cal == C_308 ? 12 : 30);
    float xp = 120 + 20.f * dayN();
    gainXP(xp);
    showBanner("TRADER SAVED",
               t.name + " owes you: better prices from now on, and left you something. +" + std::to_string((int)xp) +
                   " XP",
               OKC);
    sfx(SFX_LEVEL);
}
static void traderDies(Trader& t) {
    t.alive = false;
    t.raid = false;
    t.raidIds.clear();
    Corpse c{};
    c.p = t.p;
    c.yaw = t.a;
    c.yaw0 = t.a;
    c.type = Z_WALKER;
    c.armL = c.armR = c.head = true;
    c.look = t.look;
    c.fall = 0;
    c.fallKind = irand(0, 2);
    corpses.push_back(c);
    for (int k = 0; k < 4; k++)
        decal(Vector2Add(t.p, V2(frand(-.6f, .6f), frand(-.6f, .6f))), frand(.3f, .6f), BLOOD_FRESH);
    int n = 0; // what they had is up for grabs now
    for (auto& o : t.stock) {
        if (n++ >= 16)
            break;
        Vector2 q = Vector2Add(t.stall, V2(frand(-1.2f, 1.2f), frand(-1.2f, 1.2f)));
        if (o.kind == 0)
            spawnItem(xz3(q, .05f), o.id, std::min(o.n, IDEF[o.id].stack));
        else if (o.kind == 1)
            dropGunAt(q, o.w);
        else
            spawnMelee(xz3(q, .05f), o.id);
    }
    t.stock.clear();
    if (tradeWith >= 0 && &traders[tradeWith] == &t)
        closeTrade();
    showBanner("TRADER DOWN", t.name + " didn't make it. Their stall's up for grabs.", BLOODC);
}
static void traderShoot(Trader& t, float range) {
    Zombie* tg = nullptr;
    float bd = range;
    for (auto& z : zombies) {
        if (z.dead || z.gone)
            continue;
        float d = Vector2Distance(z.p, t.p);
        if (d < bd && losClear(t.p, z.p)) {
            bd = d;
            tg = &z;
        }
    }
    if (!tg)
        return;
    t.shotCd = frand(1.2f, 1.7f);
    t.a = atan2f(tg->p.y - t.p.y, tg->p.x - t.p.x);
    t.flash = .08f;
    hurtZ(*tg, frand(22, 34) * (bd < 6 ? 1.3f : 1), chance(.15f), t.a, false, .3f); // TUNE: trader shot
    sfxAt(SFX_SHOTGUN, t.p, .85f);
    noiseAt(t.p, t.raid ? 45.f : 15.f);
    lights.push_back({xz3(Vector2Add(t.p, polar(t.a, .9f)), 1.3f), 10, .06f, .06f, rgba(255, 200, 120)});
}
static void pickWanderTarget(Trader& t) {
    std::vector<int> c;
    for (int i = 0; i < 6; i++)
        if (TOWNS[i].kind != TK_MILITARY && (t.town < 0 || i != t.town))
            c.push_back(i);
    int ti = pick(c);
    t.town = ti;
    t.to = townSpot(ti, .7f);
    t.stuckT = 0;
    t.stuckP = t.p;
}
static void updateTraders(float dt) {
    for (auto& t : traders) {
        if (!t.alive)
            continue;
        float d = Vector2Distance(t.p, P.p);
        t.flash = std::max(0.f, t.flash - dt);
        t.hurtT = std::max(0.f, t.hurtT - dt);
        t.shotCd -= dt;
        t.moving = false;
        bool trading = tradeWith >= 0 && &traders[tradeWith] == &t;
        // wanderer: walk town to town, rest a few hours at each
        if (t.kind == 1 && !trading && d > 3) {
            if (t.rest > 0) {
                t.rest -= dt;
                if (t.rest <= 0)
                    pickWanderTarget(t);
            } else {
                Vector2 dv = Vector2Subtract(t.to, t.p);
                float dl = Vector2Length(dv);
                if (dl < 2) {
                    t.rest = frand(90, 200); // 3-7 in-game hours
                    restock(t);
                } else {
                    t.a = atan2f(dv.y, dv.x);
                    Vector2 was = t.p;
                    t.p = Vector2Add(t.p, Vector2Scale(dv, 1.5f * dt / dl));
                    collide(t.p, .35f);
                    t.walk += Vector2Distance(was, t.p);
                    t.moving = true;
                    t.stuckT += dt;
                    if (t.stuckT > 4) { // barely moved in 4 s: go somewhere else
                        if (Vector2Distance(t.stuckP, t.p) < 1.5f)
                            pickWanderTarget(t);
                        t.stuckT = 0;
                        t.stuckP = t.p;
                    }
                }
            }
        }
        if (!t.moving && !t.raid && d < 8)
            t.a += angDiff(atan2f(P.p.y - t.p.y, P.p.x - t.p.x), t.a) * std::min(1.f, dt * 4); // looks at you
        // they look after themselves: anything that gets too close takes a shotgun blast
        if (t.shotCd <= 0 && d < 90)
            traderShoot(t, t.raid ? 16.f : 6.f);
        // raids
        if (t.kind == 0 && !t.raid && t.raidDay < dayN() && d < RAID_RANGE && d > 25) {
            t.raidDay = dayN(); // one roll a day, when you come by
            if (chance(dayN() == 1 ? .15f : .3f))
                startRaid(t);
        }
        if (t.raid) {
            if (d > RAID_ABANDON) {
                msg("You left " + t.name + " to it.", STEEL);
                endRaid(t, false);
                continue;
            }
            for (int i = (int)t.raidIds.size() - 1; i >= 0; i--) {
                Zombie* z = zById(t.raidIds[i]);
                if (!z) {
                    t.raidIds.erase(t.raidIds.begin() + i);
                    continue;
                }
                if (z->aggro <= 0 && !z->sees) { // not after you: after them
                    z->np = Vector2Add(t.p, polar(z->seed * 6.28f, .8f));
                    z->noiseT = std::max(z->noiseT, 3.f);
                }
                if (Vector2Distance(z->p, t.p) < 1.6f) {
                    float& bt = t.biteT[z->id];
                    bt -= dt;
                    if (bt <= 0) {
                        bt = frand(.9f, 1.3f);
                        z->a = atan2f(t.p.y - z->p.y, t.p.x - z->p.x);
                        t.hp -= z->dmg * .6f;
                        t.hurtT = .25f;
                        sfxAt(SFX_HIT, t.p, .7f);
                        if (chance(.5f))
                            decal(Vector2Add(t.p, V2(frand(-.5f, .5f), frand(-.5f, .5f))), frand(.15f, .3f),
                                  BLOOD_FRESH);
                    }
                }
            }
            if (t.hp <= 0) {
                traderDies(t);
                continue;
            }
            if (t.raidIds.empty())
                endRaid(t, true);
        } else
            t.hp = std::min(t.kind == 1 ? 220.f : 160.f, t.hp + 2 * dt);
    }
    if (tradeWith >= 0 && (!traders[tradeWith].alive || traders[tradeWith].raid ||
                           Vector2Distance(traders[tradeWith].p, P.p) > 4 || P.hurt > 0))
        closeTrade(); // walked off, or something's biting you
}

// ---- drawing (3D, inside renderWorld3D)
static void drawTraders() {
    for (auto& t : traders) {
        if (Vector2Distance(t.p, camPos) > 80)
            continue;
        if (t.kind == 0) { // the stall: table, crates, a tarp on four posts
            float y = t.a;
            Vector2 s = t.stall;
            drawTexMesh(M_CUBE, TRS(V3(s.x, .82f, s.y), V3(.7f, .06f, 1.7f), y), rgba(150, 120, 90), TX_PLYWOOD);
            for (int k = 0; k < 4; k++) {
                Vector2 lp =
                    Vector2Add(s, Vector2Add(polar(y, k < 2 ? .3f : -.3f), polar(y + PI / 2, k % 2 ? .78f : -.78f)));
                drawMeshM(M_CUBE, TRS(V3(lp.x, .4f, lp.y), V3(.06f, .8f, .06f), y), rgba(70, 54, 40));
            }
            for (int k = 0; k < 4; k++) {
                Vector2 pp =
                    Vector2Add(s, Vector2Add(polar(y, k < 2 ? 1.2f : -.9f), polar(y + PI / 2, k % 2 ? 1.2f : -1.2f)));
                drawMeshM(M_CUBE, TRS(V3(pp.x, 1.15f, pp.y), V3(.08f, 2.3f, .08f), y), rgba(80, 66, 50));
            }
            drawMeshM(M_CUBE, TRS(V3(s.x + cosf(y) * .15f, 2.32f, s.y + sinf(y) * .15f), V3(2.4f, .04f, 2.7f), y),
                      rgba(64, 88, 62));
            for (int k = 0; k < 3; k++) {
                Vector2 cp = Vector2Add(s, Vector2Add(polar(y, -.75f), polar(y + PI / 2, -.8f + k * .8f)));
                drawTexMesh(M_CUBE, TRS(V3(cp.x, .25f + (k == 1) * .5f, cp.y), V3(.5f, .5f, .5f), y + k * .3f),
                            rgba(140, 110, 70), TX_PLANK);
            }
            setTexMode(0);
            Vector2 lp = Vector2Add(s, polar(y + PI / 2, .6f)); // lantern on the table
            drawMeshM(M_CUBE, TRS(V3(lp.x, .95f, lp.y), V3(.12f, .2f, .12f), y),
                      darkK() > .3f ? rgba(255, 210, 120) : rgba(120, 100, 60));
        }
        if (!t.alive || t.look < 0 || !haveChars)
            continue;
        PoseIn pin;
        pin.kind = PK_PLAYER;
        pin.phase = t.walk * 1.85f;
        pin.move = t.moving ? .9f : 0;
        pin.time = (float)GetTime() + t.stall.x;
        pin.hold = t.raid || t.flash > 0 ? 2 : 0;
        pin.recoil = t.flash > 0 ? 1 : 0;
        charY = 0;
        drawCharacter(t.look, pin, t.p, t.a, rgba(255, 60, 60), t.hurtT > 0 ? .35f : 0, 0, nullptr, 1, &t.ps);
    }
}
// ---- map markers
static void drawTradersMap(float mx, float my, float sc, Vector2 me) {
    for (auto& t : traders) {
        if (!t.alive || (t.kind == 1 && Vector2Distance(t.p, me) > 250))
            continue;
        Vector2 p = V2(mx + t.p.x * sc, my + t.p.y * sc);
        if (t.raid) {
            float pulse = 9 + 2 * sinf((float)GetTime() * 6);
            DrawRing(p, pulse, pulse + 3, 0, 360, 24, rgba(230, 40, 30, 220));
        }
        DrawCircleV(p, 7, t.friendly ? rgba(90, 200, 120) : rgba(230, 190, 80));
        DrawCircleLines((int)p.x, (int)p.y, 7, rgba(30, 26, 20));
        text("$", p.x, p.y - 7, 13, rgba(30, 26, 20), 1, false);
        text(t.kind == 1 ? t.name : t.name + " (trader)", p.x, p.y + 9, 12, rgba(40, 34, 26), 1, false);
    }
}
// ---- HUD during a raid
static void drawTraderHUD() {
    float y = 96;
    for (auto& t : traders) {
        if (!t.alive || !t.raid)
            continue;
        Vector2 s = GetWorldToScreen(xz3(t.p, 2.5f), cam3);
        float mhp = t.kind == 1 ? 220.f : 160.f;
        bar(s.x - 30, s.y, 60, 6, t.hp / mhp, rgba(90, 200, 120));
        text(t.name, s.x, s.y - 18, 13, BONE, 1);
        text("DEFEND " + t.name + " (" + TOWNS[t.town].name + "): " + std::to_string(t.raidIds.size()) + " left",
             W / 2.f, y, 18, rgba(255, 120, 90), 1);
        y += 22;
    }
}

// ---- the barter screen
static void drawTrade() {
    if (tradeWith < 0)
        return;
    Trader& t = traders[tradeWith];
    if (theirQ.size() != t.stock.size())
        theirQ.resize(t.stock.size(), 0);
    dim(.72f);
    Vector2 m = GetMousePosition();
    bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    int step = ctrl ? 9999 : shift ? 10 : 1;
    bool lmb = IsMouseButtonPressed(MOUSE_BUTTON_LEFT), rmb = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
    float wheel = GetMouseWheelMove();
    float pw = std::min(580.f, (W - 100) / 2.f), gap = 24, x0 = W / 2.f - pw - gap / 2, x1 = W / 2.f + gap / 2;
    float top = 120, rowH = 30, ph = H - top - 150;
    text(t.name, W / 2.f, 34, 30, SODIUM, 1);
    char rb[128];
    snprintf(rb, sizeof rb, "%s   They pay %d%% of value for your stuff%s", traderWhere(t).c_str(),
             (int)roundf(buyRate(t) * 100), t.friendly ? " (you helped them out)" : "");
    text(rb, W / 2.f, 70, 15, BONE, 1);
    text("Click: +1   Shift: +10   Ctrl: all   Right click: take back", W / 2.f, 90, 13, STEEL, 1);

    struct Row {
        int kind, id, have, *q;
        bool* b;
        float val;
        std::string name;
        Color c;
        const char* ic;
    };
    auto drawList = [&](float x, const char* title, std::vector<Row>& rows, float& scroll) {
        Rectangle pr = {x, top, pw, ph};
        DrawRectangleRec(pr, rgba(12, 14, 18, 230));
        DrawRectangleLinesEx(pr, 1, rgba(60, 68, 80));
        text(title, x + 10, top - 26, 18, BONE);
        float maxS = std::max(0.f, rows.size() * rowH - ph + 8);
        if (wheel != 0 && CheckCollisionPointRec(m, pr))
            scroll = Clamp(scroll - wheel * rowH * 2, 0, maxS);
        scroll = Clamp(scroll, 0, maxS);
        BeginScissorMode((int)x, (int)top, (int)pw, (int)ph);
        float y = top + 4 - scroll;
        for (auto& r : rows) {
            Rectangle rr = {x + 4, y, pw - 8, rowH - 2};
            int q = r.b ? (*r.b ? 1 : 0) : *r.q;
            bool hov = CheckCollisionPointRec(m, rr) && m.y > top && m.y < top + ph;
            DrawRectangleRec(rr, q > 0 ? rgba(70, 58, 30, 220) : hov ? rgba(36, 42, 52) : rgba(20, 24, 30));
            if (r.ic)
                drawIcon(r.ic, {rr.x + 2, rr.y, rowH - 2, rowH - 2}, iconTint(r.c));
            text(r.name, rr.x + rowH + 6, rr.y + 6, 15, r.c);
            char b[48];
            if (r.have > 1)
                snprintf(b, sizeof b, "x%d", r.have);
            else
                b[0] = 0;
            text(b, rr.x + pw - 190, rr.y + 6, 14, STEEL);
            snprintf(b, sizeof b, r.val < 1 ? "%.2f ea" : "%.0f ea", r.val);
            text(b, rr.x + pw - 120, rr.y + 6, 14, STEEL);
            if (q > 0) {
                snprintf(b, sizeof b, "%d", q);
                text(b, rr.x + pw - 22, rr.y + 5, 16, SODIUM, 2);
            }
            if (hov && (lmb || rmb)) {
                if (r.b)
                    *r.b = lmb;
                else
                    *r.q = Clamp(*r.q + (lmb ? step : -step), 0, r.have);
                sfx(SFX_BEEP, .25f);
            }
            y += rowH;
        }
        EndScissorMode();
        if (rows.empty())
            text("Nothing.", x + pw / 2, top + 20, 16, STEEL, 1);
    };
    std::vector<Row> L, R;
    for (int i = 0; i < (int)t.stock.size(); i++) {
        TOffer& o = t.stock[i];
        Row r{o.kind, o.id, o.n, &theirQ[i], nullptr, offerVal(o), offerName(o), BONE, nullptr};
        if (o.kind == 0) {
            r.c = BONE;
            r.ic = ITEM_ICON[o.id];
        } else if (o.kind == 1) {
            r.c = RARC[o.w.rar];
            r.ic = GUN_ICON[o.w.key];
        } else
            r.ic = MELEE_ICON[o.id];
        L.push_back(r);
    }
    float rate = buyRate(t);
    for (int id = 0; id < IT_COUNT; id++) {
        int have = invCount(id);
        yItemQ[id] = std::min(yItemQ[id], have);
        if (have > 0)
            R.push_back({0, id, have, &yItemQ[id], nullptr, IVAL[id] * rate, IDEF[id].n, BONE, ITEM_ICON[id]});
    }
    for (int s : {1, 0, 3})
        if (P.hasGun[s]) {
            Row r{1,
                  s,
                  1,
                  &yGunQ[s],
                  nullptr,
                  GVAL[P.gun[s].key] * GRAR[P.gun[s].rar] * rate,
                  P.gun[s].name,
                  RARC[P.gun[s].rar],
                  GUN_ICON[P.gun[s].key]};
            R.push_back(r);
        } else
            yGunQ[s] = 0;
    if (P.melee != MK_FISTS)
        R.push_back({2, P.melee, 1, nullptr, &yMeleeQ, MVAL[P.melee] * rate, MB[P.melee].n, BONE, MELEE_ICON[P.melee]});
    else
        yMeleeQ = false;
    drawList(x0, "THEIR GOODS", L, scrollL);
    drawList(x1, "YOUR STUFF (what they'd give for it)", R, scrollR);

    float give = tradeGive(t), get = tradeGet(t);
    float by = top + ph + 18;
    char b[160];
    snprintf(b, sizeof b, "You give: %.0f", give);
    text(b, x1, by, 20, give >= get && get > 0 ? OKC : BONE);
    snprintf(b, sizeof b, "You get: %.0f", get);
    text(b, x0, by, 20, BONE);
    bool ok = get > 0 && give + .01f >= get;
    if (get > 0 && !ok)
        snprintf(b, sizeof b, "Short by %.0f. Add more from your stuff.", ceilf(get - give));
    else if (ok && give - get >= 1)
        snprintf(b, sizeof b, "Fair. They'll keep the extra %.0f.", floorf(give - get));
    else if (ok)
        snprintf(b, sizeof b, "Fair trade.");
    else
        snprintf(b, sizeof b, "Pick what you want from their goods.");
    text(b, W / 2.f, by + 30, 16, ok ? OKC : STEEL, 1);
    float bw = 150, bx = W / 2.f - bw * 1.5f - 12;
    if (button(ok ? "TRADE" : "-", bx, by + 58, bw, 38, ok) && ok)
        doTrade();
    if (button("CLEAR", bx + bw + 12, by + 58, bw, 38, false)) {
        theirQ.assign(t.stock.size(), 0);
        memset(yItemQ, 0, sizeof yItemQ);
        memset(yGunQ, 0, sizeof yGunQ);
        yMeleeQ = false;
    }
    if (button("LEAVE  [E]", bx + 2 * (bw + 12), by + 58, bw, 38, false))
        closeTrade();
    drawCursor();
}

// ---- save / load (an optional "TR" block, so older saves still load and just get fresh traders)
static void saveTraders(std::ostream& o) {
    o << "TR " << traders.size() << '\n';
    for (auto& t : traders) {
        o << t.kind << ' ' << t.town << ' ' << t.look << ' ' << t.alive << ' ' << t.friendly << ' ' << t.stockDay << ' '
          << t.raidDay << ' ' << t.hp << ' ' << t.p.x << ' ' << t.p.y << ' ' << t.stall.x << ' ' << t.stall.y << ' '
          << t.to.x << ' ' << t.to.y << ' ' << t.a << ' ' << t.rest << ' ' << std::quoted(t.name) << ' '
          << t.stock.size() << '\n';
        for (auto& s : t.stock) {
            o << s.kind << ' ' << s.id << ' ' << s.n << '\n';
            if (s.kind == 1)
                wGun(o, s.w);
        }
    }
}
static void loadTraders(std::istream& in) {
    size_t n;
    in >> n;
    traders.clear();
    tradeWith = -1;
    for (size_t i = 0; i < n && in; i++) {
        Trader t;
        size_t sn;
        in >> t.kind >> t.town >> t.look >> t.alive >> t.friendly >> t.stockDay >> t.raidDay >> t.hp >> t.p.x >>
            t.p.y >> t.stall.x >> t.stall.y >> t.to.x >> t.to.y >> t.a >> t.rest >> std::quoted(t.name) >> sn;
        if (t.look < 0 || t.look >= (int)looks.size())
            t.look = survivorLook();
        if (t.town >= 6)
            t.town = -1;
        for (size_t k = 0; k < sn && in; k++) {
            TOffer o;
            in >> o.kind >> o.id >> o.n;
            if (o.kind == 1)
                o.w = rGun(in);
            bool okId = o.kind == 0   ? o.id >= 0 && o.id < IT_COUNT
                        : o.kind == 1 ? o.w.key >= 0 && o.w.key < W_COUNT
                                      : o.id >= 0 && o.id < MK_COUNT;
            if (okId)
                t.stock.push_back(o);
        }
        if (t.kind == 0 && t.town < 0)
            continue;
        traders.push_back(t);
    }
}
