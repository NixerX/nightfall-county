// Nightfall County: update world.
// Not a standalone header: included once, in order, from src/main.cpp. Everything above it in that list is visible here.
#pragma once
static void updateBullets(float dt) {
    for (int i = (int)bullets.size() - 1; i >= 0; i--) {
        Bullet& b = bullets[i];
        b.life -= dt;
        bool dead = b.life <= 0;
        int steps = std::max(1, (int)ceilf(Vector2Length(b.v) * dt / .3f));
        Vector2 st = Vector2Scale(b.v, dt / steps);
        for (int s = 0; s < steps && !dead; s++) {
            b.p = Vector2Add(b.p, st);
            if (pointSolid(b.p.x, b.p.y, 1.1f)) {
                dead = true;
                Vector2 bp = Vector2Subtract(b.p, st);
                for (int k = 0; k < 6; k++)
                    parts.push_back({xz3(bp, 1.3f), V3(frand(-4, 4), frand(0, 4), frand(-4, 4)), .15f, .15f, .03f,
                                     rgba(255, 220, 150), P_SPARK});
                parts.push_back({xz3(bp, 1.3f), V3(0, .3f, 0), .6f, .6f, .2f, rgba(150, 140, 130, 120), P_DUST});
                lights.push_back({xz3(bp, 1.3f), 3, .05f, .05f, rgba(255, 200, 140)});
                break;
            }
            for (int zi : zNear(b.p)) {
                Zombie& z = zombies[zi];
                if (z.dead)
                    continue;
                bool hb = false;
                for (int h = 0; h < b.nh; h++)
                    if (b.hits[h] == z.id)
                        hb = true;
                if (hb)
                    continue;
                if (Vector2DistanceSqr(z.p, b.p) < (z.r + .08f) * (z.r + .08f)) {
                    if (b.nh < 8)
                        b.hits[b.nh++] = z.id;
                    bool hd = chance(P.perks.crit + b.crit) || (P.actT > 0 && P.cls == 3);
                    if (b.burn)
                        z.burn = 3;
                    if (b.leech > 0)
                        P.hp = std::min(P.perks.maxhp, P.hp + b.leech);
                    hurtZ(z, b.dmg, hd, atan2f(b.v.y, b.v.x), false, b.dism,
                          b.shotgun && Vector2Distance(z.p, P.p) < 3.5f);
                    if (b.pierce > 0) {
                        b.pierce--;
                        b.dmg *= .8f;
                    } else {
                        dead = true;
                        break;
                    }
                }
            }
        }
        if (dead) {
            bullets[i] = bullets.back();
            bullets.pop_back();
        }
    }
}
static void updateWorld(float dt) {
    const float G = 18;
    for (int i = (int)bombs.size() - 1; i >= 0; i--) {
        Bomb& b = bombs[i];
        b.v.y -= G * dt;
        Vector3 n = Vector3Add(b.p, Vector3Scale(b.v, dt));
        if (pointSolid(n.x, b.p.z, n.y))
            b.v.x *= -.4f;
        else
            b.p.x = n.x;
        if (pointSolid(b.p.x, n.z, n.y))
            b.v.z *= -.4f;
        else
            b.p.z = n.z;
        b.p.y = n.y;
        if (b.p.y < .08f) {
            b.p.y = .08f;
            b.v.y *= -.3f;
            b.v.x *= .6f;
            b.v.z *= .6f;
        }
        b.t -= dt;
        if (b.flare) {
            if (chance(dt * 30))
                parts.push_back({b.p, V3(frand(-.3f, .3f), frand(1, 2), frand(-.3f, .3f)), .5f, .5f, .2f,
                                 rgba(255, 60, 40, 140), P_SMOKE});
            if (chance(dt * 40))
                parts.push_back(
                    {b.p, V3(frand(-2, 2), frand(1, 3), frand(-2, 2)), .3f, .3f, .05f, rgba(255, 120, 90), P_SPARK});
            for (auto& z : zombies)
                if (Vector2Distance(z.p, V2(b.p.x, b.p.z)) < 30) {
                    z.lureT = .3f;
                    z.lp = V2(b.p.x, b.p.z);
                }
            if (b.t <= 0)
                bombs.erase(bombs.begin() + i);
            continue;
        }
        b.beep -= dt;
        b.blink -= dt;
        if (b.beep <= 0) {
            b.beep = std::max(.08f, b.t * .17f);
            b.blink = .07f;
            sfxAt(SFX_BEEP, V2(b.p.x, b.p.z), .6f, 0);
            lights.push_back({b.p, 5, .08f, .08f, rgba(255, 60, 40)});
        }
        for (auto& z : zombies)
            if (Vector2Distance(z.p, V2(b.p.x, b.p.z)) < 35) {
                z.lureT = .2f;
                z.lp = V2(b.p.x, b.p.z);
            }
        if (b.t <= 0) {
            Vector2 bp = V2(b.p.x, b.p.z);
            bombs.erase(bombs.begin() + i);
            explode(bp, 5.5f * P.perks.blast, 180 * (1 + .1f * (dayN() - 1)), 50);
        }
    }
    for (int i = (int)acid.size() - 1; i >= 0; i--) {
        AcidPool& p = acid[i];
        p.t -= dt;
        if (p.t <= 0) {
            acid.erase(acid.begin() + i);
            continue;
        }
        float f = std::min(1.f, p.t / 1.5f);
        if (Vector2Distance(P.p, p.p) < p.r * f && state == ST_PLAY) {
            P.hp -= 9 * dt;
            P.hurt = std::max(P.hurt, .1f);
            if (P.hp <= 0)
                die();
        }
        for (auto& z : zombies)
            if (z.type != Z_BLOATER && Vector2Distance(z.p, p.p) < p.r * f)
                hurtZ(z, 8 * dt, false, -999, true);
        if (chance(dt * 8))
            parts.push_back({xz3(Vector2Add(p.p, polar(frand(0, 6), frand(0, p.r * .7f))), .1f), V3(0, .6f, 0), .8f,
                             .8f, .12f, rgba(170, 220, 80, 160), P_MIST});
    }
    for (int i = (int)parts.size() - 1; i >= 0; i--) {
        Particle& p = parts[i];
        p.t -= dt;
        bool blood = p.kind == P_BLOOD || p.kind == P_ACID;
        if (blood || p.kind == P_SPARK || p.kind == P_CASING)
            p.v.y -= G * dt;
        if (p.kind == P_SMOKE || p.kind == P_MIST || p.kind == P_DUST)
            p.v = Vector3Scale(p.v, powf(.3f, dt));
        if (p.kind == P_FIRE) {
            p.v = Vector3Scale(p.v, powf(.15f, dt));
            p.v.y += 3 * dt;
        }
        Vector3 np = Vector3Add(p.p, Vector3Scale(p.v, dt));
        if (blood && pointSolid(np.x, np.z, np.y)) {
            p.t = 0;
            p.v = V3(0, 0, 0);
        } else
            p.p = np;
        float floor = buildingAt(V2(p.p.x, p.p.z)) >= 0 ? .07f : .02f;
        if (p.p.y < floor) {
            p.p.y = floor;
            if (blood) {
                decal(V2(p.p.x, p.p.z), p.s * frand(2, 4.5f), p.kind == P_ACID ? ACID_C : BLOOD_DRY);
                p.t = 0;
            } else if (p.kind == P_CASING) {
                p.v = Vector3Scale(p.v, .3f);
                p.v.y = fabsf(p.v.y);
            } else
                p.v.y = 0;
        }
        if (p.t <= 0) {
            parts[i] = parts.back();
            parts.pop_back();
        }
    }
    if (parts.size() > 3500)
        parts.erase(parts.begin(), parts.begin() + (parts.size() - 3500));
    for (auto& g : gibs) {
        if (g.rest)
            continue;
        g.t += dt;
        g.v.y -= G * dt;
        Vector3 n = Vector3Add(g.p, Vector3Scale(g.v, dt));
        if (pointSolid(n.x, g.p.z, n.y)) {
            g.v.x *= -.35f;
            g.rotv *= .6f;
            decal(V2(g.p.x, g.p.z), .12f, BLOOD_DRY);
        } else
            g.p.x = n.x;
        if (pointSolid(g.p.x, n.z, n.y)) {
            g.v.z *= -.35f;
            g.rotv *= .6f;
        } else
            g.p.z = n.z;
        g.p.y = n.y;
        float fl = (g.kind == G_RIB ? .03f : g.s.y * .5f) + (buildingAt(V2(g.p.x, g.p.z)) >= 0 ? .06f : 0);
        if (g.p.y < fl) {
            g.p.y = fl;
            if (fabsf(g.v.y) > 1.5f)
                decal(V2(g.p.x, g.p.z), frand(.12f, .3f), g.acid ? ACID_C : BLOOD_DRY);
            g.v.y *= -.3f;
            g.v.x *= .55f;
            g.v.z *= .55f;
            g.rotv *= .5f;
        }
        g.rot += g.rotv * dt;
        g.trail += Vector3Length(g.v) * dt;
        if (g.trail > .35f && g.p.y < .4f) {
            g.trail = 0;
            decal(V2(g.p.x, g.p.z), frand(.05f, .12f), g.acid ? ACID_C : BLOOD_DRY);
        }
        if (g.t > 1 && Vector3Length(g.v) < .3f)
            g.rest = true;
    }
    for (auto& d : decals)
        if (d.grow > 0) {
            float add = std::min(d.grow, dt * .35f);
            d.r += add;
            d.grow -= add;
        }
    for (int i = (int)floaters.size() - 1; i >= 0; i--) {
        floaters[i].t -= dt;
        floaters[i].p.y += dt * .8f;
        if (floaters[i].t <= 0)
            floaters.erase(floaters.begin() + i);
    }
    for (int i = (int)lights.size() - 1; i >= 0; i--) {
        lights[i].t -= dt;
        if (lights[i].t <= 0) {
            lights[i] = lights.back();
            lights.pop_back();
        }
    }
    for (int i = (int)sdrops.size() - 1; i >= 0; i--) {
        sdrops[i].t -= dt;
        sdrops[i].p.y += dt * 8;
        if (sdrops[i].t <= 0) {
            sdrops[i] = sdrops.back();
            sdrops.pop_back();
        }
    }
    for (int i = (int)msgs.size() - 1; i >= 0; i--) {
        msgs[i].life -= dt;
        if (msgs[i].life <= 0)
            msgs.erase(msgs.begin() + i);
    }
    for (auto& l : loot)
        if (!l.settled) {
            l.v.y -= G * dt;
            l.p = Vector3Add(l.p, Vector3Scale(l.v, dt));
            if (l.p.y < .05f) {
                l.p.y = .05f;
                l.settled = true;
            }
        }
    // campfires: burn fuel, warm you, draw the dead at night, die in the rain unless a tree's over them
    for (int i = (int)fires.size() - 1; i >= 0; i--) {
        Fire& f = fires[i];
        f.fuel -= dt;
        bool sheltered = false;
        for (int cx = gcl(f.p.x - 6); cx <= gcl(f.p.x + 6); cx++)
            for (int cz = gcl(f.p.y - 6); cz <= gcl(f.p.y + 6); cz++)
                for (int ti : tgrid[cx][cz])
                    if (Vector2Distance(trees[ti].p, f.p) < trees[ti].r * .95f + .3f)
                        sheltered = true;
        if (rainLevel > .35f && !sheltered) {
            f.fuel -= dt * 18;
            if (!f.warned && Vector2Distance(f.p, P.p) < 40) {
                f.warned = true;
                msg("The rain's drowning your fire. Should've built it under a tree.", COLDC);
            }
        }
        f.smokeT -= dt;
        if (f.smokeT <= 0 && Vector2Distance(f.p, P.p) < 70) {
            f.smokeT = .07f;
            parts.push_back({xz3(Vector2Add(f.p, V2(frand(-.25f, .25f), frand(-.25f, .25f))), .25f),
                             V3(frand(-.2f, .2f), frand(1.4f, 2.6f), frand(-.2f, .2f)), .6f, .6f, frand(.12f, .24f),
                             rgba(255, 140, 50), P_FIRE});
            if (chance(.3f))
                parts.push_back({xz3(f.p, 1.2f), V3(frand(-.3f, .3f), frand(.8f, 1.6f), frand(-.3f, .3f)), 3, 3,
                                 frand(.6f, 1.2f), rgba(50, 50, 54, 140), P_SMOKE});
        }
        f.lureT -= dt;
        if (f.lureT <= 0) {
            f.lureT = 4;
            noiseAt(f.p, isNight() ? 38.f : 16.f);
        }
        if (f.fuel <= 0) {
            decal(f.p, .9f, rgba(18, 16, 14, 230));
            if (Vector2Distance(f.p, P.p) < 30)
                msg("The fire's out.", STEEL);
            fires.erase(fires.begin() + i);
        }
    }
    // ambient fire smoke
    if (chance(dt * 10))
        for (auto& l : staticLights) {
            if (Vector2Distance(V2(l.p.x, l.p.z), P.p) > 60)
                continue;
            parts.push_back({Vector3Add(l.p, V3(frand(-.2f, .2f), 0, frand(-.2f, .2f))),
                             V3(frand(-.2f, .2f), frand(1.5f, 3), frand(-.2f, .2f)), .6f, .6f, frand(.12f, .25f),
                             rgba(255, 140, 50), P_FIRE});
            if (chance(.4f))
                parts.push_back({Vector3Add(l.p, V3(0, 1, 0)), V3(frand(-.3f, .3f), frand(.8f, 1.6f), frand(-.3f, .3f)),
                                 3, 3, frand(.6f, 1.2f), rgba(40, 40, 44, 150), P_SMOKE});
        }
}
static void update(float dt) {
    gTime += dt;
    P.alive += 0;
    shake = std::max(0.f, shake - dt * 2);
    groanCd -= dt;
    if (banner.t > 0)
        banner.t -= dt;
    char ph = isNight() ? 'n' : 'd';
    if (ph != phase) {
        phase = ph;
        if (ph == 'n') {
            showBanner("NIGHTFALL", "They see your flashlight. Volatiles are hunting.", rgba(255, 90, 60));
            sfx(SFX_HORN, 1, 0);
        } else {
            nights++;
            int bonus = (int)roundf(100.f * nights);
            gainXP((float)bonus);
            showBanner("DAWN", "Survived night " + std::to_string(nights) + ".  +" + std::to_string(bonus) + " XP");
            sfx(SFX_LEVEL);
        }
    }
    // weather
    rainT -= dt;
    if (rainT <= 0) {
        rainT = frand(90, 240);
        rainTarget = chance(.35f) ? frand(.6f, 1.f) : 0;
    }
    rainLevel += (rainTarget - rainLevel) * std::min(1.f, dt * .05f);
    // horde
    if (!horde.on && hordeDay < dayN() && hourOf() > 13 && hourOf() < 18 && dayN() >= 2) {
        hordeDay = dayN();
        horde = {true, frand(0, 2 * PI), 0, 14 + dayN() * 8};
        horde.end = Vector2Add(Vector2Add(P.p, polar(horde.a + PI, 150)), polar(horde.a + PI / 2, frand(-40, 40)));
        horde.lastSeen = P.p;
        static const char* dirs[8] = {"EAST", "SOUTHEAST", "SOUTH", "SOUTHWEST",
                                      "WEST", "NORTHWEST", "NORTH", "NORTHEAST"};
        int di = ((int)roundf(fmodf(horde.a + 2 * PI, 2 * PI) / (PI / 4))) % 8;
        showBanner("HORDE INBOUND", std::string("Moving in from the ") + dirs[di] + ". Get inside or get moving.",
                   rgba(255, 90, 60));
        sfx(SFX_HORN, .6f, 0);
        sfx(SFX_STING, 1, 0);
    }
    for (int i = (int)followers.size() - 1; i >= 0; i--) {
        Follower& f = followers[i];
        f.t -= dt;
        if (f.t > 0)
            continue; // arrives at the top/bottom of the stairs, only if you are still on that floor
        if (buildingAt(P.p) == f.cell || Vector2Distance(P.p, f.to) < 30) {
            Zombie& z = spawnZ(f.type, Vector2Add(f.to, V2(frand(-.4f, .4f), frand(-.4f, .4f))));
            z.aggro = 20;
            z.losT = 0;
        }
        followers.erase(followers.begin() + i);
    }
    if (horde.on && !inCell(P.p)) {
        horde.t -= dt;
        if (horde.t <= 0) {
            horde.t = .25f;
            Zombie* z = spawnAround(chance(.2f) ? Z_RUNNER : Z_WALKER, 70, 85, horde.a, .3f);
            if (z) {
                z->hordeM = true;
                z->noiseT = 240;
                z->np = Vector2Add(horde.end, V2(frand(-12, 12), frand(-12, 12)));
            }
            if (--horde.left <= 0)
                horde.on = false;
        }
    }
    // director
    spawnT -= dt;
    if (inCell(P.p))
        spawnT = 1; // nothing wanders into a basement on its own
    if (spawnT <= 0) {
        spawnT = .6f;
        int t = zoneTown();
        int base = t < 0                          ? 7
                   : TOWNS[t].kind == TK_CITY     ? 46
                   : TOWNS[t].kind == TK_MILITARY ? 38
                   : TOWNS[t].kind == TK_TOWN     ? 28
                                                  : 16;
        if (hotAt(P.p, 1.6f) >= 0)
            base = (int)(std::max(base, 18) * 2.5f);
        int target = (int)(base * (isNight() ? 1.4f : 1) * (1 + .1f * (dayN() - 1)) * (.55f + .45f * scaleK()));
        int nearN = 0;
        for (auto& z : zombies)
            if (Vector2Distance(z.p, P.p) < 110)
                nearN++;
        if (nearN < target)
            spawnAround(pickType(t), 45, 95);
    }
    updatePlayer(dt);
    if (state != ST_PLAY)
        return;
    updateZombies(dt);
    separateFromPlayer();
    updateBullets(dt);
    updateWorld(dt);
    zombies.erase(std::remove_if(zombies.begin(), zombies.end(),
                                 [](const Zombie& z) {
                                     return z.dead || z.gone;
                                 }),
                  zombies.end());
    Vector2 ap = aimPoint();
    Vector2 lead = Vector2Scale(V2(Clamp(ap.x - P.p.x, -12, 12), Clamp(ap.y - P.p.y, -9, 9)), .28f);
    camPos = Vector2Lerp(camPos, Vector2Add(P.p, lead), 1 - powf(.002f, dt));
    if (pendingLv > 0 && state == ST_PLAY)
        grantLevelPoints();
}
