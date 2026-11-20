// Nightfall County: combat.
// Not a standalone header: included once, in order, from src/main.cpp. Everything above it in that list is visible here.
#pragma once
// ============================================================ combat & gore
static void die();
static void throwItem(bool flareK, bool free = false);
static int playerLevel() {
    return P.lvl;
}
static void gainXP(float v) {
    P.xp += v;
    while (P.xp >= P.next) {
        P.xp -= P.next;
        P.lvl++;
        P.next = xpNeed(P.lvl);
        pendingLv++;
    }
}
static void noiseAt(Vector2 p, float r) {
    int pb = buildingAt(p);
    for (auto& z : zombies) {
        if (Vector2DistanceSqr(z.p, p) >= r * r)
            continue;                                   // out of range anyway: skip the building lookup
        float rr = buildingAt(z.p) != pb ? r * .4f : r; // walls muffle: inside vs outside hears 40% as far
        if (Vector2DistanceSqr(z.p, p) < rr * rr && z.aggro <= 0 && z.ignoreT <= 0 && z.pound < 0) {
            z.np = Vector2Add(p, V2(frand(-4, 4), frand(-4, 4)));
            z.noiseT = 12;
        }
    }
}
static bool bruteCharge = false; // the hit being dealt is a brute's charge (only that can break a leg)
static void hitPlayer(float dmg, Vector2 src, int srcType) {
    if (P.iframe > 0 || state != ST_PLAY)
        return;
    if (P.blocking && srcType >= 0 && srcType < 99 &&
        fabsf(angDiff(atan2f(src.y - P.p.y, src.x - P.p.x), P.a)) < 1.2f) { // blocked from the front
        float cost = (P.shieldHP > 0 ? 5.f : 11.f) * staK();
        if (P.sta >= cost) {
            P.sta -= cost;
            P.staD = .45f;
            float red = std::min(.95f, (P.shieldHP > 0 ? .88f : .6f) + P.perks.block);
            float taken = dmg * (1 - red);
            if (P.shieldHP > 0) {
                P.shieldHP -= dmg * (1 - P.perks.block * 2) * .8f;
                if (P.shieldHP <= 0) {
                    P.shieldHP = 0;
                    msg("Your shield splinters apart.", BLOODC);
                }
            }
            for (auto& z : zombies)
                if (Vector2Distance(z.p, src) < .3f) {
                    z.stun = .45f;
                    z.k = Vector2Add(z.k, polar(atan2f(z.p.y - P.p.y, z.p.x - P.p.x), 3));
                }
            sfx(SFX_BLOCK, .9f, .15f);
            P.blockFx = .15f;
            shake = std::min(.3f, shake + .1f);
            P.hp -= taken;
            if (P.hp <= 0)
                die();
            return;
        }
        msg("Guard broken!", BLOODC);
        P.blocking = false;
    }
    Gear gr = gear();
    dmg *= 1 - std::min(.7f, P.perks.armor + gr.armor * .5f);
    P.hp -= dmg;
    if (srcType >= 0 && srcType < 99)
        wearDamage(dmg);
    P.hurt = .35f;
    if (Vector2DistanceSqr(P.p, src) > 1e-4f)
        P.hurtA = atan2f(P.p.y - src.y, P.p.x - src.x);
    P.iframe = .15f;
    shake = std::min(.6f, shake + .25f);
    P.pain = std::max(P.pain, 20.f);
    spray(xz3(P.p, 1.2f), -999, 10, 0, 1, 4, BLOOD_FRESH);
    sfx(SFX_HURT);
    screenGore(dmg > 15 ? 4 : 2);
    if (srcType >= 0 && srcType < 99) {
        if (chance((srcType == Z_VOLATILE ? .6f : .35f) * (1 - P.perks.bleedres) * (1 - std::min(.75f, gr.armor)))) {
            P.bleeds++;
            msg("You're bleeding.", BLOODC);
        }
        if (chance(srcType == Z_BLOATER ? .5f : .06f) && P.sick <= 0) {
            P.sick = 1;
            msg("You feel sick. Find antibiotics.", rgba(170, 200, 90));
        }
        if (srcType == Z_BRUTE && bruteCharge && chance(.04f) && !P.broken) {
            P.broken = true;
            msg("Your leg is broken. Morphine will fix it.", BLOODC);
        }
        float a = atan2f(P.p.y - src.y, P.p.x - src.x);
        P.k = Vector2Add(P.k, polar(a, srcType == Z_BRUTE ? 10.f : 4.f));
    }
    if (P.hp <= 0)
        die();
}
// kind: 0 face first toward you, 1 onto its back away from you, 2 crumple (knees give first), -1 pick one
static void corpse(Zombie& z, float ang, bool head, int kind = -1) {
    if (corpses.size() > 260)
        corpses.erase(corpses.begin());
    Corpse c{z.p, ang, z.type, z.armL, z.armR, head, 0, z.look, frand(0, 1)};
    c.fallKind = kind >= 0 ? kind : !head ? 2 : crawler(z) ? 0 : chance(.45f) ? 1 : 0;
    c.fall = crawler(z) ? 1 : 0; // crawlers are already down
    c.yaw0 = z.a;
    float len = .88f * (z.look >= 0 && z.look < (int)looks.size() ? looks[z.look].scale.z : 1);
    if (crawler(z)) { // lies where it crawled, head first
        c.yaw = z.a + PI;
        c.fallKind = 0;
        len = 0;
    }
    Vector2 headDir = polar(c.fallKind == 1 ? c.yaw : c.yaw + PI, 1);
    c.p = Vector2Add(z.p, Vector2Scale(headDir, len)); // the body lies ahead of where its feet were
    for (int k = 0; k < 4; k++)
        c.cut[k] = z.cut[k];
    for (int k = 0; k < 3; k++)
        c.wnd[k] = z.wnd[k];
    c.ps = std::move(z.ps);
    corpses.push_back(std::move(c));
    decal(corpses.back().p, .3f, BLOOD_DRY, frand(.9f, 1.5f));
}
// ---- body-part gibs: the zombie's own arm, leg, head or upper body, everything else collapsed
enum PartKind {
    PG_ARM_L,
    PG_ARM_R,
    PG_LEG_L,
    PG_LEG_R,
    PG_HEAD,
    PG_UPPER
};
static int partRoot(const Rig& r, int part) {
    switch (part) {
    case PG_ARM_L:
        return r.ua[0];
    case PG_ARM_R:
        return r.ua[1];
    case PG_LEG_L:
        return r.ul[0];
    case PG_LEG_R:
        return r.ul[1];
    case PG_HEAD:
        return r.neck >= 0 ? r.neck : r.head;
    default:
        return r.abdomen;
    }
}
static bool inSubtree(const Rig& r, int b, int root) {
    for (int i = b; i >= 0; i = r.parent[i])
        if (i == root)
            return true;
    return false;
}
static Matrix zBodyM(const Zombie& z) {
    const Look& L = looks[z.look];
    return MatrixMultiply(MatrixMultiply(MatrixScale(L.scale.x, L.scale.y, L.scale.z), MatrixRotateY(PI / 2 - z.a)),
                          MatrixTranslate(z.p.x, 0, z.p.y));
}
// returns false when it couldn't (no character models, Gore set to Low): the caller throws box chunks instead
static bool addPartGib(const Zombie& z, int part, float ang, float spd, float up = 1) {
    if (cfg.gore == 0 || z.look < 0 || !haveChars)
        return false;
    const Look& L = looks[z.look];
    Asset& a = assets[L.asset];
    int root = partRoot(a.rig, part);
    if (root < 0)
        return false;
    PoseIn pin;
    pin.kind = PK_ZOMBIE;
    pin.seed = z.seed;
    static std::vector<Quaternion> D;
    static std::vector<float> S;
    static std::vector<Transform> tf;
    Vector3 ro = buildPose(a.rig, pin, D, S);
    tf.resize(a.rig.bind.size());
    solvePose(a.rig, D, S, ro, tf.data());
    Matrix M = zBodyM(z);
    Vector3 c = {0, 0, 0};
    int n = 0;
    for (int i = 0; i < (int)tf.size(); i++)
        if (inSubtree(a.rig, i, root)) {
            c = Vector3Add(c, Vector3Transform(tf[i].translation, M));
            n++;
        }
    if (!n)
        return false;
    c = Vector3Scale(c, 1.f / n);
    if (partGibs >= (cfg.gore == 2 ? 70 : 40)) // too many: the oldest part goes
        for (size_t i = 0; i < gibs.size(); i++)
            if (gibs[i].look >= 0) {
                gibs.erase(gibs.begin() + i);
                partGibs--;
                break;
            }
    Gib g{};
    g.p = c;
    Vector2 h = polar(ang, spd);
    g.v = V3(h.x, frand(2, 5) * up, h.y);
    float ha = frand(0, 2 * PI);
    g.axis = V3(cosf(ha), 0, sinf(ha)); // tumbles end over end, so it lands lying down
    g.rot = 0;
    g.rotv = frand(6, 14) * (chance(.5f) ? 1 : -1);
    g.kind = G_LIMB;
    g.c = WHITE;
    float th = part == PG_UPPER ? .3f : part == PG_HEAD ? .24f : .12f;
    g.s = V3(th, th, th);
    g.look = z.look;
    g.root = root;
    g.M0 = M;
    g.c0 = c;
    g.seed = z.seed;
    gibs.push_back(g);
    partGibs++;
    gibCount++;
    return true;
}
static float slowMo = 0; // a beat of slow motion on the odd spectacular kill
static void chunks(Vector3 c, const ZDef& t, float ang, int n, float smin, float smax, float spread = 1.4f) {
    for (int i = 0; i < n; i++)
        addGib(c, G_CHUNK,
               chance(.5f)   ? rgba(120, 20, 26)
               : chance(.5f) ? t.shirt
                             : t.skin,
               V3(frand(.06f, .2f), frand(.05f, .16f), frand(.06f, .2f)), ang + frand(-spread, spread),
               frand(smin, smax));
}
// a limb comes off: model part if we can, a box if not
static void throwLimb(const Zombie& z, int part, float ang, float spd) {
    if (addPartGib(z, part, ang, spd))
        return;
    const ZDef& t = ZT[z.type];
    bool leg = part == PG_LEG_L || part == PG_LEG_R;
    Vector3 c = xz3(z.p, part == PG_HEAD ? 1.6f : leg ? .5f : 1.f);
    if (part == PG_HEAD)
        addGib(c, G_HEAD, t.skin, V3(.24f, .26f, .24f), ang, spd);
    else
        addGib(c, G_LIMB, leg ? t.pants : t.skin, leg ? V3(.16f, .8f, .16f) : V3(.12f, .6f, .12f), ang, spd);
}
static int gibForce = -1; // test scenes: force a gib pattern
// explosive / overkill death. Every one rolls a pattern and its own counts, sizes and speeds
static void gibZombie(Zombie& z, float ang) {
    const ZDef& t = ZT[z.type];
    Vector3 c = xz3(z.p, 1.0f);
    float gk = goreK();
    bool big = chance(.05f);
    float bk = big ? 2.f : 1.f;
    float roll = frand(0, 1);
    int pat = roll < .45f ? 0 : roll < .65f ? 1 : roll < .87f ? 2 : 3; // shred, top half gone, split, red mist
    if (gibForce >= 0)
        pat = gibForce; // test scenes
    if (z.type == Z_BRUTE && pat == 3)
        pat = 0;
    if (pat == 0) { // full shred: whatever's still attached flies, in pieces
        if (z.cut[0] == 0 && chance(.75f))
            throwLimb(z, PG_ARM_L, ang + frand(-1.2f, 1.2f), frand(4, 8));
        if (z.cut[1] == 0 && chance(.75f))
            throwLimb(z, PG_ARM_R, ang + frand(-1.2f, 1.2f), frand(4, 8));
        if (z.cut[2] == 0 && chance(.6f))
            throwLimb(z, PG_LEG_L, ang + frand(-1, 1), frand(2, 6));
        if (z.cut[3] == 0 && chance(.6f))
            throwLimb(z, PG_LEG_R, ang + frand(-1, 1), frand(2, 6));
        if (chance(.6f))
            throwLimb(z, PG_HEAD, ang + frand(-.5f, .5f), frand(4, 9));
        else
            chunks(xz3(z.p, 1.6f), t, ang, irand(3, 6), 3, 8, .8f);
        chunks(c, t, ang, (int)(irand(4, 10) * gk * bk), 3, 10);
        for (int i = 0; i < irand(1, 4); i++)
            addGib(c, G_RIB, rgba(225, 215, 195), V3(.04f, .04f, frand(.22f, .38f)), ang + frand(-1, 1), frand(3, 7));
        for (int i = 0; i < irand(1, 2); i++)
            addGib(c, G_GUTS, rgba(150, 60, 72), V3(frand(.22f, .34f), .14f, frand(.22f, .34f)), ang + frand(-.5f, .5f),
                   frand(1, 4));
    } else if (pat == 1) { // top half blown away: the legs stand a beat, then fold
        throwLimb(z, PG_UPPER, ang + frand(-.3f, .3f), frand(3, 6));
        chunks(xz3(z.p, 1.1f), t, ang, (int)(irand(4, 8) * gk * bk), 2, 8);
        addGib(xz3(z.p, 1.f), G_GUTS, rgba(150, 60, 72), V3(.3f, .14f, .3f), ang + frand(-.5f, .5f), frand(.5f, 2));
        corpse(z, ang, true, 2);
        corpses.back().noUpper = true;
        corpses.back().fall = -.6f; // stays standing a moment before it folds
    } else if (pat == 2) {          // one side blown off: arm and leg go, the rest falls
        int sd = chance(.5f) ? 0 : 1;
        if (z.cut[sd] < 2)
            throwLimb(z, sd ? PG_ARM_R : PG_ARM_L, ang + frand(-.6f, .6f), frand(4, 8));
        if (z.cut[2 + sd] < 2)
            throwLimb(z, sd ? PG_LEG_R : PG_LEG_L, ang + frand(-.6f, .6f), frand(3, 6));
        z.cut[sd] = 2;
        z.cut[2 + sd] = 2;
        if (sd)
            z.armR = false;
        else
            z.armL = false;
        chunks(c, t, ang, (int)(irand(3, 7) * gk * bk), 3, 8);
        addGib(c, G_GUTS, rgba(150, 60, 72), V3(.26f, .14f, .26f), ang + frand(-.5f, .5f), frand(1, 3));
        corpse(z, ang, true, chance(.5f) ? 0 : 1);
    } else { // red mist: almost nothing left
        for (int i = 0; i < (int)(irand(10, 16) * gk); i++)
            addGib(c, G_CHUNK, chance(.6f) ? rgba(120, 20, 26) : t.skin,
                   V3(frand(.04f, .09f), frand(.03f, .07f), frand(.04f, .09f)), frand(0, 2 * PI), frand(3, 12));
        mist(c, (int)(26 * gk), rgba(140, 12, 18, 190));
        spray(c, -999, (int)(70 * gk), 0, 3, 14, BLOOD_FRESH, false, 2.f);
        decal(z.p, .5f, BLOOD_DRY, 2.2f);
    }
    spray(c, ang, (int)(frand(40, 70) * gk * bk), frand(.9f, 1.4f), 2, 12, BLOOD_FRESH);
    mist(c, (int)(frand(6, 14) * gk * bk), rgba(130, 12, 18, 160));
    for (int i = 0; i < irand(2, 4); i++)
        streak(Vector2Add(z.p, polar(ang, .3f)), ang + frand(-.7f, .7f), frand(1.2f, 3.2f));
    for (int i = 0; i < irand(2, 5); i++)
        decal(Vector2Add(z.p, polar(ang + frand(-.6f, .6f), frand(.5f, 3))), frand(.5f, 1.3f), BLOOD_DRY);
    if (pat != 3)
        decal(z.p, .4f, BLOOD_DRY, frand(1.1f, 1.8f));
    sfxAt(SFX_GORE, z.p);
    shake = std::min(.7f, shake + .2f);
    if (big) {
        slowMo = .15f;
        shake = std::min(.9f, shake + .2f);
    }
    if (Vector2Distance(z.p, P.p) < 3)
        screenGore(5);
}
static void acidBoom(Vector2 p);
static int hitStyle = HS_OTHER; // set by whoever deals the hit (bullets, melee, kicks, explosions)
static int hitMelee = -1;       // melee weapon dealing it, if any
static void killZ(Zombie& z, float ang, bool head, float overkill, bool explosive) {
    if (z.dead)
        return;
    z.dead = true;
    kills++;
    if (chance(.3f))
        sfxAt(SFX_ZDEATH, z.p, .6f, .2f);
    gainXP(ZT[z.type].xp * (1 + .1f * (dayN() - 1)));
    const ZDef& t = ZT[z.type];
    if (z.type == Z_BLOATER)
        acidBoom(z.p);
    else if (explosive || overkill > z.mhp * .5f || (z.type == Z_RUNNER && overkill > 25))
        gibZombie(z, ang);
    else if (head) {
        Vector3 hp = xz3(z.p, t.h - .15f);
        float gk = goreK();
        bool decap = (hitStyle == HS_BLADE && (hitMelee == MK_AXE || hitMelee == MK_MACHETE || chance(.4f))) ||
                     (hitStyle == HS_RIFLE && chance(.25f));
        if (decap) {
            throwLimb(z, PG_HEAD, ang + frand(-.4f, .4f),
                      frand(3, 6)); // clean off: the head flies, the neck gushes, the body drops
            spray(hp, -999, (int)(26 * gk), 0, 1, 4, BLOOD_FRESH, false, 4.f);
            spray(hp, ang, (int)(14 * gk), .5f, 2, 7, BLOOD_FRESH, false, 2.f);
            z.bleed = 0;
            corpse(z, ang, false, 2);
            floatText(hp, "DECAPITATED", rgba(255, 210, 63), 14);
        } else if (hitStyle == HS_BLUNT) { // caved in: nothing flies far, it just comes apart where it stands
            for (int i = 0; i < irand(3, 6); i++)
                addGib(hp, G_CHUNK, chance(.5f) ? t.skin : rgba(130, 18, 24),
                       V3(frand(.05f, .1f), frand(.04f, .08f), frand(.05f, .1f)), frand(0, 2 * PI), frand(.5f, 2.5f));
            spray(hp, -999, (int)(22 * gk), 0, .5f, 3, BLOOD_FRESH, false, 1.5f);
            decal(z.p, .35f, BLOOD_DRY, frand(1.2f, 1.8f));
            corpse(z, ang, false, 2);
            floatText(hp, "CAVED IN", rgba(255, 210, 63), 14);
        } else { // popped
            for (int i = 0; i < (int)(irand(4, 10) * gk); i++)
                addGib(hp, G_CHUNK, chance(.4f) ? t.skin : rgba(130, 18, 24),
                       V3(frand(.04f, .11f), frand(.04f, .09f), frand(.04f, .11f)), ang + frand(-.9f, .9f),
                       frand(3, 9));
            for (int i = 0; i < irand(0, 2); i++)
                addGib(hp, G_RIB, rgba(230, 220, 200), V3(.05f, .03f, frand(.08f, .14f)), ang + frand(-.4f, .4f),
                       frand(3, 6));
            spray(hp, ang, (int)(frand(24, 40) * gk), frand(.5f, .9f), 3, 11, BLOOD_FRESH, false, 1.2f);
            mist(hp, (int)(frand(4, 8) * gk), rgba(150, 14, 20, 170));
            streak(Vector2Add(z.p, polar(ang, .4f)), ang + frand(-.15f, .15f), frand(1.5f, 3.f));
            corpse(z, ang, false);
            floatText(hp, "HEADSHOT", rgba(255, 210, 63), 14);
        }
        sfxAt(SFX_GORE, z.p, .7f);
    } else {
        impactBlood(z.p, t.h, ang, 30, false, hitStyle);
        corpse(z, ang, true);
    }
    if (z.type == Z_VOLATILE)
        sfxAt(SFX_SCREAM, z.p);
    dropFromZombie(z.p, z.type, z.job);
}
// part: 0 arm L, 1 arm R, 2 leg L, 3 leg R. lvl: 1 below the joint (forearm, shin), 2 the whole limb
static void dismember(Zombie& z, float ang, int part = -1, int lvl = 0) {
    if (part < 0) { // any arm still on
        if (z.cut[0] && z.cut[1])
            return;
        part = !z.cut[0] && (z.cut[1] || chance(.5f)) ? 0 : 1;
    }
    if (z.cut[part] >= 2)
        return;
    if (lvl <= 0)
        lvl = (hitStyle == HS_BLADE && (hitMelee == MK_AXE || hitMelee == MK_MACHETE)) || hitStyle == HS_BLAST ||
                      (hitStyle == HS_SHOTGUN && chance(.5f)) || chance(.25f)
                  ? 2
                  : 1;
    lvl = std::max(lvl, (int)z.cut[part] + 1);
    bool leg = part >= 2;
    int sd = part & 1;
    z.cut[part] = (signed char)lvl;
    if (!leg) {
        if (sd)
            z.armR = false;
        else
            z.armL = false;
    }
    Vector2 sp = Vector2Add(z.p, polar(z.a + (sd ? PI / 2 : -PI / 2), z.r * .8f));
    float hy = leg ? (lvl == 2 ? .85f : .45f) : (lvl == 2 ? 1.45f : 1.2f);
    int pk = leg ? (sd ? PG_LEG_R : PG_LEG_L) : (sd ? PG_ARM_R : PG_ARM_L);
    if (lvl == 2 && z.look >= 0 && haveChars && cfg.gore > 0)
        addPartGib(z, pk, ang + frand(-.8f, .8f), frand(3, 7));
    else
        addGib(xz3(sp, hy), G_LIMB, leg ? ZT[z.type].pants : ZT[z.type].skin,
               leg ? V3(.14f, lvl == 2 ? .8f : .45f, .14f) : V3(.11f, lvl == 2 ? .6f : .32f, .11f),
               ang + frand(-.8f, .8f), frand(3, 7));
    spray(xz3(sp, hy), ang, (int)(16 * goreK()), .7f, 1, 6, BLOOD_FRESH);
    spray(xz3(sp, hy), -999, (int)(6 * goreK()), 0, .5f, 2, BLOOD_FRESH, false, 3.f); // the stump spurts
    z.bleed = 8;
    if (leg) {
        z.notice = 1; // it goes down, but it's still coming
        if (z.cut[2] && z.cut[3])
            z.spd *= .8f;
    } else
        z.dmg *= .75f;
    sfxAt(SFX_CHOP, z.p, .7f);
}
static void hurtZ(Zombie& z, float dmg, bool head, float ang, bool dot, float dism = 0, bool explosive = false) {
    if (z.dead)
        return;
    if (head && z.type == Z_SOLDIER && chance(.5f)) {
        head = false;
        dmg *= .5f;
        for (int i = 0; i < 4; i++)
            parts.push_back({xz3(z.p, 1.7f), V3(frand(-3, 3), frand(1, 3), frand(-3, 3)), .15f, .15f, .04f,
                             rgba(255, 230, 160), P_SPARK});
    }
    if (head)
        dmg *= 2.5f;
    z.hp -= dmg;
    if (!dot) {
        z.flash = .06f;
        z.aggro = std::max(z.aggro, 8.f);
        z.stun = .1f;
        // where it landed: head, torso, an arm or a leg (top-down, so it's a roll)
        float zr = frand(0, 1);
        bool legsOk = z.type != Z_BRUTE && z.type != Z_BLOATER;
        int zone = head ? 0 : zr < .5f ? 1 : zr < .75f ? 2 : legsOk ? 3 : 1; // 2 arm, 3 leg
        float wd = Clamp(dmg / std::max(1.f, z.mhp) * 1.6f, .05f, .6f);
        int wz = zone == 0 ? 0 : zone == 3 ? 2 : 1;
        z.wnd[wz] = std::min(1.f, z.wnd[wz] + wd);
        if (ang > -99) {
            z.hitT = .3f;
            z.hitA = ang;
            z.hitK = Clamp(dmg / 40.f, .35f, 1.f) * (z.type == Z_BRUTE ? .4f : 1);
            if (z.type == Z_BRUTE && z.chgT > 0) { // big hits stop a charge: it stumbles, dazed (shotgun blast, axe)
                z.chgT -= dmg / 100.f;
                if (z.chgT <= 0) {
                    z.chgT = 0;
                    z.stun = std::max(z.stun, 2.f);
                    floatText(xz3(z.p, 2.3f), "STAGGERED", rgba(255, 210, 63), 14);
                }
            }
            float kb = z.type == Z_BRUTE ? .6f : z.type == Z_VOLATILE ? 1.5f : 3.f;
            z.k = Vector2Add(z.k, polar(ang, kb));
            impactBlood(z.p, ZT[z.type].h, ang, dmg, head, hitStyle, z.type == Z_BLOATER);
        }
        if (z.hp > 0 && dism > 0 && z.type != Z_BLOATER && (zone == 2 || zone == 3)) {
            float sk = hitStyle == HS_BLUNT ? .35f : hitStyle == HS_SHOTGUN ? 1.2f : hitStyle == HS_BLAST ? 1.5f : 1.f;
            if (chance(std::min(.95f, dism * 2 * sk))) {
                int sd = chance(.5f) ? 1 : 0;
                dismember(z, ang, zone == 2 ? sd : 2 + sd);
                if (hitStyle == HS_SHOTGUN && Vector2Distance(z.p, P.p) < 4 && chance(.3f))
                    dismember(z, ang, zone == 2 ? 2 + sd : 1 - sd); // close-range buckshot takes two
                if (z.cut[0] && z.cut[1] && z.cut[2] && z.cut[3])
                    z.hp = 0; // nothing left to move with
            }
        }
        sfxAt(SFX_HIT, z.p, .5f, .2f);
    }
    if (z.hp <= 0)
        killZ(z, ang < -99 ? frand(0, 2 * PI) : ang, head, -z.hp + (dot ? 0 : dmg * .3f), explosive);
}
static void explode(Vector2 p, float R, float dmg, float self) {
    int hs0 = hitStyle;
    hitStyle = HS_BLAST;
    for (auto& z : zombies) {
        float d = Vector2Distance(z.p, p);
        if (d < R + z.r) {
            float a = atan2f(z.p.y - p.y, z.p.x - p.x);
            z.k = Vector2Add(z.k, polar(a, 10));
            z.burn = std::max(z.burn, 2.f);
            hurtZ(z, dmg * (1 - .6f * d / (R + z.r)), false, a, false, .7f, true);
        }
    }
    hitStyle = hs0;
    float pd = Vector2Distance(P.p, p);
    if (self > 0 && pd < R * .8f)
        hitPlayer(self * (1 - pd / R), p, 99);
    for (int i = 0; i < 70; i++) {
        Vector2 h = polar(frand(0, 2 * PI), frand(1, 16));
        parts.push_back({xz3(p, .5f), V3(h.x, frand(1, 9), h.y), frand(.3f, .9f), .9f, frand(.15f, .4f),
                         pick(std::vector<Color>{rgba(255, 180, 60), rgba(255, 120, 40), rgba(255, 220, 140)}),
                         P_FIRE});
    }
    for (int i = 0; i < 30; i++) {
        Vector2 h = polar(frand(0, 2 * PI), frand(.5f, 4));
        parts.push_back({xz3(p, .8f), V3(h.x, frand(.5f, 2.5f), h.y), frand(2, 4), 4, frand(1, 2.2f),
                         rgba(50, 50, 54, 200), P_SMOKE});
    }
    lights.push_back({xz3(p, 2), R * 4, .5f, .5f, rgba(255, 170, 80)});
    decal(p, R * .5f, rgba(12, 12, 12, 230));
    shake = std::min(1.2f, shake + .8f);
    sfxAt(SFX_BOOM, p, 1.3f, .1f);
    noiseAt(p, 160);
}
static void acidBoom(Vector2 p) {
    acid.push_back({p, 3.2f, 8});
    spray(xz3(p, 1), -999, 50, 0, 2, 9, ACID_C, true);
    mist(xz3(p, 1), 12, rgba(140, 190, 50, 150));
    for (int i = 0; i < 6; i++)
        addGib(xz3(p, 1), G_CHUNK, rgba(150, 180, 60), V3(.15f, .1f, .15f), frand(0, 2 * PI), frand(3, 8), true);
    for (auto& z : zombies)
        if (z.type != Z_BLOATER && Vector2Distance(z.p, p) < 3.5f)
            hurtZ(z, 30, false, atan2f(z.p.y - p.y, z.p.x - p.x), false);
    if (Vector2Distance(P.p, p) < 3.5f)
        hitPlayer(16, p, Z_BLOATER);
    lights.push_back({xz3(p, 1.5f), 9, .4f, .4f, rgba(150, 220, 80)});
    shake = std::min(.8f, shake + .3f);
    sfxAt(SFX_SPLAT, p);
    noiseAt(p, 40);
}
