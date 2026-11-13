// Nightfall County: zombie ai.
// Not a standalone header: included once, in order, from src/main.cpp. Everything above it in that list is visible here.
#pragma once
// ---- detection and hunting tuning, per zombie type
static float detRate(int t) {
    switch (t) {
    case Z_RUNNER:
        return 1.0f;
    case Z_VOLATILE:
        return .9f;
    case Z_SOLDIER:
        return .8f;
    case Z_BRUTE:
        return .55f;
    case Z_BLOATER:
        return .5f;
    default:
        return .65f;
    }
}
static float
keepT(const Zombie& z) { // seconds a zombie keeps hunting after losing sight: walkers give up fast, specials stick
    float t;
    switch (z.type) {
    case Z_WALKER:
        t = 2.f;
        break;
    case Z_RUNNER:
        t = 3.f;
        break;
    case Z_BRUTE:
        t = 4.f;
        break;
    case Z_VOLATILE:
        t = 7.f;
        break;
    case Z_SOLDIER:
        t = 4.5f;
        break;
    default:
        t = 3.f;
    }
    if (P.hideK < 1 || !P.moving)
        t *= .6f;
    return t;
} // hidden or standing still: they lose you sooner
static float doorMul(int t) {
    switch (t) {
    case Z_WALKER:
        return .3f;
    case Z_RUNNER:
        return .5f;
    case Z_BRUTE:
        return 2.4f;
    case Z_SOLDIER:
        return .7f;
    case Z_BLOATER:
        return .8f;
    default:
        return .6f;
    }
} // how hard each type hits a door
static void updateZombies(float dt) {
    for (auto& c : zg)
        for (auto& v : c)
            v.clear();
    for (int i = 0; i < (int)zombies.size(); i++)
        zg[zcl(zombies[i].p.x)][zcl(zombies[i].p.y)].push_back(i);
    bool night = isNight();
    int pb = buildingAt(P.p);
    float sight = (night ? 14.f : 30.f) * (P.crouch ? .55f : 1) * (rainLevel > .5f ? .8f : 1) + (P.light ? 18.f : 0);
    sight *= (1 - P.perks.stealth) * P.hideK * gear().vis;
    for (int zi = 0; zi < (int)zombies.size(); zi++) {
        Zombie& z = zombies[zi];
        if (z.dead)
            continue;
        z.flash -= dt;
        z.stun -= dt;
        z.slow -= dt;
        z.aggro -= dt;
        z.lureT -= dt;
        z.losT -= dt;
        z.ignoreT -= dt;
        if (z.burn > 0) {
            z.burn -= dt;
            hurtZ(z, 12 * dt, false, -999, true);
            if (chance(dt * 18))
                parts.push_back({xz3(Vector2Add(z.p, V2(frand(-.3f, .3f), frand(-.3f, .3f))), frand(.5f, 1.6f)),
                                 V3(0, frand(1, 2.5f), 0), .5f, .5f, frand(.1f, .2f), rgba(255, 150, 50), P_FIRE});
            if (z.dead)
                continue;
        }
        if (z.bleed > 0) {
            z.bleed -= dt;
            if (chance(dt * 6))
                decal(Vector2Add(z.p, V2(frand(-.2f, .2f), frand(-.2f, .2f))), frand(.06f, .15f), BLOOD_DRY);
        }
        float dx = P.p.x - z.p.x, dz = P.p.y - z.p.y, d = sqrtf(dx * dx + dz * dz);
        if (d > 180) {
            z.gone = true;
            continue;
        }
        bool lit = P.uv && P.light && d < 14 && fabsf(angDiff(atan2f(-dz, -dx), P.a)) < .35f;
        if (lit) {
            if (z.type == Z_VOLATILE) {
                z.slow = .25f;
                hurtZ(z, 30 * dt, false, -999, true);
                if (chance(dt * 20))
                    parts.push_back({xz3(z.p, 1.5f), V3(frand(-.5f, .5f), 1.5f, frand(-.5f, .5f)), .6f, .6f, .3f,
                                     rgba(200, 170, 255, 160), P_SMOKE});
                if (z.dead)
                    continue;
            } else if (z.type == Z_RUNNER)
                z.slow = .2f;
        }
        if (z.losT <= 0) {
            z.losT = frand(.2f, .35f);
            z.inSight = P.loft >= 0 ? (P.firedT > 0 && d < 9 && losClear(z.p, P.p)) : (d < sight && losClear(z.p, P.p));
            if (z.inSight && z.hordeM) {
                horde.lastSeen = P.p;
                for (auto& o : zombies)
                    if (o.hordeM && Vector2Distance(o.p, z.p) < 30)
                        o.aggro = std::max(o.aggro, keepT(o));
            }
        }
        {
            float rate = detRate(z.type) * (1 + 1.5f * Clamp(1 - d / std::max(sight, 1.f), 0, 1)) *
                         (P.sprint ? 1.4f : 1) * (P.crouch ? .8f : 1);
            if (!P.crouch)
                rate = 99.f; // standing: spotted at once, like before. The meter is for crouching only
            if (z.inSight)
                z.notice = std::min(1.2f, z.notice + rate * dt);
            else
                z.notice = std::max(0.f, z.notice - .8f * dt);
            bool close = d < 2.2f && P.hideK < 1;
            if (close)
                z.notice = 1;
            z.sees = z.notice >= 1;
            if (P.crouch && d < 40 && z.notice > gExposure)
                gExposure = std::min(1.f, z.notice);
        }
        Vector2 tgt = P.p;
        float mult = 1;
        bool chase = false;
        if (z.lureT > 0)
            tgt = z.lp;
        else if (z.sees || (z.aggro > 0 && d < 60)) {
            chase = true;
            if (z.sees)
                z.aggro = std::max(z.aggro, keepT(z));
        } else if (z.noiseT > 0) {
            tgt = z.np;
            z.noiseT -= dt;
            mult = z.hordeM ? .75f : .8f;
            if (Vector2Distance(tgt, z.p) < (z.hordeM ? 4.f : 1.5f))
                z.noiseT = 0;
            if (z.noiseT <= 0 &&
                z.hordeM) { // reached a waypoint: searching done -> resume the march; march done -> drift off
                if (z.hstate == 1 || Vector2Distance(z.p, horde.end) > 10) {
                    z.hstate = 0;
                    z.np = Vector2Add(horde.end, V2(frand(-10, 10), frand(-10, 10)));
                    z.noiseT = 240;
                } else
                    z.hordeM = false;
            }
        } else {
            z.wt -= dt;
            if (z.wt <= 0) {
                z.wa = frand(0, 2 * PI);
                z.wt = frand(3, 7);
            }
            tgt = Vector2Add(z.p, polar(z.wa, 3));
            mult = .3f;
        }
        if (chase && z.type == Z_VOLATILE && z.lureT <= 0 && d > 6) {
            float dd = std::max(d, .01f);
            Vector2 perp = V2(-dz / dd, dx / dd);
            tgt = Vector2Add(P.p, Vector2Scale(perp, sinf(gTime * .9f + z.seed * 6.28f) * 2.2f));
        } // weave in, don't charge
        if (z.chasing && !chase && z.hordeM) {
            z.hstate = 1;
            z.np = Vector2Add(horde.lastSeen, V2(frand(-9, 9), frand(-9, 9)));
            z.noiseT = 14;
        } // lost you: search your last spot
        z.chasing = chase;
        // doors
        bool banging = false;
        if (chase || z.noiseT > 0) {
            int zb = buildingAt(z.p), tb = buildingAt(tgt);
            if (zb != tb) {
                int b = zb >= 0 ? zb : tb;
                const Building& B = blds[b];
                bool exiting = zb >= 0;
                float bd = 1e9f;
                const Door* best = nullptr;
                for (auto& dr : B.doors) {
                    float dd = Vector2Distance(z.p, exiting ? dr.in : dr.out);
                    if (dd < bd) {
                        bd = dd;
                        best = &dr;
                    }
                }
                if (best) {
                    Vector2 first = exiting ? best->in : best->out, second = exiting ? best->out : best->in;
                    DoorObj* dobj = best->obj >= 0 ? &doorObjs[best->obj] : nullptr;
                    bool shut = dobj && !dobj->open && !dobj->broken;
                    if (shut &&
                        z.via !=
                            2) { // closed door between us: walk up to it, then pound until it gives or they lose interest
                        Vector2 tan = V2(-dobj->outN.y, dobj->outN.x);
                        Vector2 spot = Vector2Add(Vector2Add(dobj->c, Vector2Scale(dobj->outN, exiting ? -.85f : .85f)),
                                                  Vector2Scale(tan, z.side * .45f * (z.id % 3)));
                        if (Vector2Distance(z.p, spot) < .7f) {
                            if (z.pound != best->obj) {
                                z.pound = best->obj;
                                z.poundT = (chase || z.aggro > 0) ? frand(22, 40) : frand(10, 16);
                            }
                        } else {
                            tgt = spot;
                            z.via = 1;
                        }
                    } else if (z.via == 2 || Vector2Distance(z.p, first) < 1.0f) {
                        tgt = second;
                        z.via = 2;
                    } else {
                        tgt = first;
                        z.via = 1;
                    }
                }
            } else
                z.via = 0;
        }
        if (z.pound >= 0) { // pounding state
            DoorObj& dd = doorObjs[z.pound];
            int db = dd.bld;
            if (dd.open || dd.broken) {
                z.pound = -1;
                z.via = 1;
            } else {
                z.poundT -= dt;
                if (z.sees)
                    z.poundT = std::max(z.poundT, 20.f);
                if (buildingAt(P.p) == db && Vector2Distance(P.p, dd.c) < 22 &&
                    (P.firedT > 0 || (P.moving && !P.crouch)))
                    z.poundT = std::max(z.poundT, 12.f); // they hear you in there
                if (z.poundT <= 0) {
                    z.pound = -1;
                    z.aggro = 0;
                    z.noiseT = 0;
                    z.ignoreT = 10;
                    z.wa = atan2f(z.p.y - dd.c.y, z.p.x - dd.c.x) + frand(-.8f, .8f);
                    z.wt = 6;
                } else {
                    banging = true;
                    tgt = dd.c;
                    z.atk -= dt;
                    if (z.atk <= 0) {
                        z.atk = frand(.85f, 1.3f);
                        dd.hp -= z.dmg * doorMul(z.type) * frand(.7f, 1.3f);
                        dd.bang = .25f;
                        sfxAt(dd.metal ? SFX_DOOR_METAL : SFX_DOOR_BANG, dd.c, dd.metal ? .8f : .95f, .06f);
                        if (dd.hp < dd.hpMax * .35f && chance(.4f))
                            sfxAt(SFX_CREAK, dd.c, .6f, .2f);
                        if (!dd.metal)
                            for (int k = 0; k < 3; k++)
                                parts.push_back({xz3(dd.c, frand(.6f, 2.f)),
                                                 V3(frand(-1.5f, 1.5f), frand(.5f, 2), frand(-1.5f, 1.5f)), .5f, .5f,
                                                 .04f, rgba(140, 100, 60), P_DUST});
                        if (dd.hp <= 0) {
                            dd.broken = true;
                            sfxAt(SFX_BOOM, dd.c, .25f);
                            sfxAt(dd.metal ? SFX_DOOR_METAL : SFX_DOOR_BANG, dd.c, 1.f, .1f);
                            for (int k = 0; k < 12; k++)
                                parts.push_back({xz3(dd.c, 1.2f), V3(frand(-3, 3), frand(1, 3), frand(-3, 3)), .6f, .6f,
                                                 .06f, dd.metal ? rgba(150, 150, 150) : rgba(140, 100, 60), P_DUST});
                            if (Vector2Distance(dd.c, P.p) < 30)
                                msg(dd.metal ? "They tore the metal door off its hinges." : "They broke the door down.",
                                    BLOODC);
                        }
                    }
                }
            }
        }
        float ang = atan2f(tgt.y - z.p.y, tgt.x - z.p.x);
        if (z.type == Z_VOLATILE && lit)
            ang += PI;
        if (z.type == Z_WALKER || z.type == Z_SOLDIER)
            ang += sinf(gTime * 1.3f + z.seed) * .3f;
        float look = z.r + .5f;
        if (pointSolid(z.p.x + cosf(ang) * look, z.p.y + sinf(ang) * look, .4f)) {
            bool ok = false;
            for (float da : {.6f, 1.1f, 1.6f, 2.2f}) {
                float a2 = ang + da * z.side;
                if (!pointSolid(z.p.x + cosf(a2) * look, z.p.y + sinf(a2) * look, .4f)) {
                    ang = a2;
                    ok = true;
                    break;
                }
            }
            if (!ok)
                z.side = -z.side;
        }
        float sp = z.spd * mult *
                   (night && (z.type == Z_WALKER || z.type == Z_RUNNER || z.type == Z_SOLDIER) ? 1.35f : 1) *
                   (z.slow > 0 ? .4f : 1) * (z.stun > 0 ? .3f : 1);
        if (z.type == Z_BRUTE) {
            z.chg -= dt;
            if (z.chgT > 0) {
                z.chgT -= dt;
                ang = z.ca;
                sp = z.spd * 4.5f;
                if (chance(dt * 30))
                    parts.push_back({xz3(z.p, .1f), V3(frand(-1, 1), .5f, frand(-1, 1)), .6f, .6f, .4f,
                                     rgba(90, 80, 70, 150), P_DUST});
                if (d < z.r + .5f && !wallBetween(z.p, P.p)) {
                    hitPlayer(z.dmg * 1.3f, z.p, Z_BRUTE);
                    z.chgT = 0;
                }
            } else if (chase && d < 12 && d > 3 && z.chg <= 0) {
                z.chgT = .8f;
                z.chg = 5;
                z.ca = atan2f(dz, dx);
                sfxAt(SFX_ROAR, z.p);
                shake = std::min(.5f, shake + .15f);
            }
        }
        z.a = ang;
        Vector2 mv = polar(ang, sp * dt);
        z.p = Vector2Add(z.p, mv);
        z.walk += Vector2Length(mv);
        z.p = Vector2Add(z.p, Vector2Scale(z.k, dt));
        z.k = Vector2Scale(z.k, powf(.002f, dt));
        for (int o : zNear(z.p)) {
            if (o == zi)
                continue;
            Zombie& q = zombies[o];
            float ex = z.p.x - q.p.x, ez = z.p.y - q.p.y, md = z.r + q.r, e2 = ex * ex + ez * ez;
            if (e2 < md * md && e2 > 1e-4f) {
                float e = sqrtf(e2), pp = (md - e) * .5f;
                z.p.x += ex / e * pp;
                z.p.y += ez / e * pp;
            }
        }
        collide(z.p, z.r);
        if (P.loft < 0 && P.roll <= 0 && P.vaultT <= 0) { // they get in your face, but they can't stand inside you
            Vector2 dv = Vector2Subtract(z.p, P.p);
            float pd = Vector2Length(dv), minD = z.r + .36f;
            if (pd < minD) {
                Vector2 nrm = pd > 1e-3f ? Vector2Scale(dv, 1 / pd) : polar(frand(0, 6), 1);
                float ov = minD - pd;
                float zk = z.type == Z_BRUTE ? .25f : .7f;
                z.p = Vector2Add(z.p, Vector2Scale(nrm, ov * zk));
                P.p = Vector2Subtract(P.p, Vector2Scale(nrm, ov * (1 - zk)));
                collide(z.p, z.r);
                collide(P.p, .35f);
            }
        }
        if (z.type == Z_BLOATER && d < 1.4f) {
            z.hp = 0;
            killZ(z, atan2f(dz, dx), false, 0, false);
            continue;
        }
        if (banging) {
            z.a = atan2f(tgt.y - z.p.y, tgt.x - z.p.x);
            for (int o : zNear(z.p)) {
                if (o == zi)
                    continue;
                Zombie& q = zombies[o];
                float ex = z.p.x - q.p.x, ez = z.p.y - q.p.y, md = z.r + q.r, e2 = ex * ex + ez * ez;
                if (e2 < md * md && e2 > 1e-4f) {
                    float e = sqrtf(e2), pp = (md - e) * .5f;
                    z.p.x += ex / e * pp;
                    z.p.y += ez / e * pp;
                }
            }
            collide(z.p, z.r);
            continue;
        }
        if (d < z.r + .35f + .35f && P.roll <= 0 && P.loft < 0 && !wallBetween(z.p, P.p)) {
            z.atk -= dt;
            if (z.atk <= 0) {
                z.atk = z.type == Z_VOLATILE ? .6f : .95f;
                if (chance(.45f))
                    sfxAt(z.type == Z_BRUTE      ? SFX_ROAR
                          : z.type == Z_VOLATILE ? SFX_SCREAM
                                                 : SFX_ZATTACK,
                          z.p, .8f, .15f);
                hitPlayer(z.dmg, z.p, z.type);
            }
        } else
            z.atk = std::max(z.atk, .35f);
        if (groanCd <= 0 && d < 18 && chance(dt * .5f)) {
            groanCd = frand(1.5f, 4);
            sfxAt(SFX_GROAN, z.p, .8f, .2f);
        }
    }
    (void)pb;
}
