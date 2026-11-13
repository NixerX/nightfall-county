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
    for (auto& z : zombies)
        if (Vector2DistanceSqr(z.p, p) < r * r && z.aggro <= 0 && z.ignoreT <= 0 && z.pound < 0) {
            z.np = Vector2Add(p, V2(frand(-4, 4), frand(-4, 4)));
            z.noiseT = 12;
        }
}
static void hitPlayer(float dmg, Vector2 src, int srcType) {
    if (P.iframe > 0 || state != ST_PLAY)
        return;
    if (P.blocking && srcType >= 0 && srcType < 99 &&
        fabsf(angDiff(atan2f(src.y - P.p.y, src.x - P.p.x), P.a)) < 1.2f) { // blocked from the front
        float cost = (P.shieldHP > 0 ? 5.f : 11.f) * STA_K;
        if (P.sta >= cost) {
            P.sta -= cost;
            P.staD = .7f;
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
        if (srcType == Z_BRUTE && chance(.12f) && !P.broken) {
            P.broken = true;
            msg("Your leg is broken. Morphine will fix it.", BLOODC);
        }
        float a = atan2f(P.p.y - src.y, P.p.x - src.x);
        P.k = Vector2Add(P.k, polar(a, srcType == Z_BRUTE ? 14.f : 4.f));
    }
    if (P.hp <= 0)
        die();
}
static void corpse(const Zombie& z, float ang, bool head) {
    if (corpses.size() > 260)
        corpses.erase(corpses.begin());
    corpses.push_back({z.p, ang, z.type, z.armL, z.armR, head, 0, z.look, frand(0, 1)});
    decal(z.p, .3f, BLOOD_DRY, frand(.9f, 1.5f));
}
static void gibZombie(Zombie& z, float ang) {
    const ZDef& t = ZT[z.type];
    Vector3 c = xz3(z.p, 1.0f);
    addGib(xz3(z.p, 1.6f), G_HEAD, t.skin, V3(.24f, .26f, .24f), ang + frand(-.5f, .5f), frand(4, 9));
    if (z.armL)
        addGib(c, G_LIMB, t.skin, V3(.12f, .6f, .12f), ang + frand(-1.2f, 1.2f), frand(4, 8));
    if (z.armR)
        addGib(c, G_LIMB, t.skin, V3(.12f, .6f, .12f), ang + frand(-1.2f, 1.2f), frand(4, 8));
    for (int i = 0; i < 2; i++)
        addGib(V3(z.p.x, .5f, z.p.y), G_LIMB, t.pants, V3(.16f, .8f, .16f), ang + frand(-1, 1), frand(2, 6));
    for (int i = 0; i < irand(5, 9); i++)
        addGib(c, G_CHUNK, chance(.5f) ? rgba(120, 20, 26) : t.shirt,
               V3(frand(.1f, .2f), frand(.08f, .16f), frand(.1f, .2f)), ang + frand(-1.4f, 1.4f), frand(3, 10));
    for (int i = 0; i < 3; i++)
        addGib(c, G_RIB, rgba(225, 215, 195), V3(.04f, .04f, .34f), ang + frand(-1, 1), frand(3, 7));
    addGib(c, G_GUTS, rgba(150, 60, 72), V3(.3f, .14f, .3f), ang + frand(-.5f, .5f), frand(1, 4));
    spray(c, ang, 60, 1.2f, 2, 12, BLOOD_FRESH);
    mist(c, 10, rgba(130, 12, 18, 160));
    for (int i = 0; i < 4; i++)
        decal(Vector2Add(z.p, polar(ang + frand(-.6f, .6f), frand(.5f, 3))), frand(.6f, 1.3f), BLOOD_DRY);
    decal(z.p, .4f, BLOOD_DRY, 1.6f);
    sfxAt(SFX_GORE, z.p);
    shake = std::min(.7f, shake + .2f);
    if (Vector2Distance(z.p, P.p) < 3)
        screenGore(5);
}
static void acidBoom(Vector2 p);
static void killZ(Zombie& z, float ang, bool head, float overkill, bool explosive) {
    if (z.dead)
        return;
    z.dead = true;
    kills++;
    if (chance(.3f))
        sfxAt(SFX_ZDEATH, z.p, .6f, .2f);
    gainXP(ZT[z.type].xp * (1 + .1f * (dayN() - 1)));
    if (z.type == Z_BLOATER)
        acidBoom(z.p);
    else if (explosive || overkill > z.mhp * .5f || (z.type == Z_RUNNER && overkill > 25))
        gibZombie(z, ang);
    else if (head) {
        Vector3 hp = xz3(z.p, ZT[z.type].h - .15f);
        for (int i = 0; i < 7; i++)
            addGib(hp, G_CHUNK, chance(.4f) ? ZT[z.type].skin : rgba(130, 18, 24),
                   V3(frand(.05f, .1f), frand(.05f, .09f), frand(.05f, .1f)), ang + frand(-.8f, .8f), frand(3, 8));
        addGib(hp, G_RIB, rgba(230, 220, 200), V3(.05f, .03f, .12f), ang, frand(3, 6));
        spray(hp, ang, 34, .7f, 3, 11, BLOOD_FRESH, false, 1.2f);
        mist(hp, 6, rgba(150, 14, 20, 170));
        decal(Vector2Add(z.p, polar(ang, frand(1.5f, 3))), frand(.8f, 1.4f), BLOOD_DRY);
        corpse(z, ang, false);
        floatText(hp, "HEADSHOT", rgba(255, 210, 63), 14);
        sfxAt(SFX_GORE, z.p, .7f);
    } else {
        spray(xz3(z.p, 1.1f), ang, 18, .8f, 1, 6, BLOOD_FRESH);
        corpse(z, ang, true);
    }
    if (z.type == Z_VOLATILE)
        sfxAt(SFX_SCREAM, z.p);
    dropFromZombie(z.p, z.type, z.job);
}
static void dismember(Zombie& z, float ang) {
    if (!z.armL && !z.armR)
        return;
    bool left = z.armL && (!z.armR || chance(.5f));
    if (left)
        z.armL = false;
    else
        z.armR = false;
    Vector2 sp = Vector2Add(z.p, polar(z.a + (left ? -PI / 2 : PI / 2), z.r));
    addGib(xz3(sp, 1.35f), G_LIMB, ZT[z.type].skin, V3(.12f, .6f, .12f), ang + frand(-.8f, .8f), frand(3, 7));
    spray(xz3(sp, 1.35f), ang, 16, .7f, 1, 6, BLOOD_FRESH);
    z.bleed = 8;
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
        if (ang > -99) {
            float kb = z.type == Z_BRUTE ? .6f : z.type == Z_VOLATILE ? 1.5f : 3.f;
            z.k = Vector2Add(z.k, polar(ang, kb));
            spray(xz3(z.p, 1.2f), ang, 6, .5f, 1.5f, 6, z.type == Z_BLOATER ? ACID_C : BLOOD_FRESH, z.type == Z_BLOATER,
                  1.2f);
            if (chance(.5f))
                decal(Vector2Add(z.p, polar(ang, frand(.4f, 1.6f))), frand(.15f, .35f),
                      z.type == Z_BLOATER ? ACID_C : BLOOD_DRY);
        }
        if (z.hp > 0 && dism > 0 && z.type != Z_BLOATER && chance(dism))
            dismember(z, ang);
        sfxAt(SFX_HIT, z.p, .5f, .2f);
    }
    if (z.hp <= 0)
        killZ(z, ang < -99 ? frand(0, 2 * PI) : ang, head, -z.hp + (dot ? 0 : dmg * .3f), explosive);
}
static void explode(Vector2 p, float R, float dmg, float self) {
    for (auto& z : zombies) {
        float d = Vector2Distance(z.p, p);
        if (d < R + z.r) {
            float a = atan2f(z.p.y - p.y, z.p.x - p.x);
            z.k = Vector2Add(z.k, polar(a, 10));
            z.burn = std::max(z.burn, 2.f);
            hurtZ(z, dmg * (1 - .6f * d / (R + z.r)), false, a, false, .7f, true);
        }
    }
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
