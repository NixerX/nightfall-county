// Nightfall County: autotest.
// Not a standalone header: included once, in order, from src/main.cpp. Everything above it in that list is visible here.
#pragma once
#ifdef NF_AUTOTEST
static int atFrame = 0;
static void autotest() {
    atFrame++; if (getenv("NF_JUMP") && atFrame == 26) atFrame = atoi(getenv("NF_JUMP")); int f = atFrame;
    auto shot = [](const char* n) { TakeScreenshot(n); };
    if (f == 20) shot("shot_title.png");
    if (f == 25) { newGame(); P.p = Vector2Add(TOWNS[0].c, V2(-30, 20)); camPos = P.p; gTime = CYCLE * .2f; for (int i = 0; i < 12; i++) spawnAround(Z_WALKER, 6, 14); P.gun[0] = makeWeapon(W_SHOTGUN, 2); P.hasGun[0] = true; P.active = 0; P.gun[0].clip = 6; invAdd(IT_A12, 30); }
    if (f == 60) shot("shot_day_city.png");
    if (f == 62) { camZoom = 1.6f; }
    if (f == 70) shot("shot_closeup.png");
    if (f == 72) { camZoom = 1.f; }
    if (f > 60 && f < 140 && f % 6 == 0) { // blast nearby zombies
        int best = -1; float bd = 1e9; for (int i = 0; i < (int)zombies.size(); i++) { float d = Vector2Distance(zombies[i].p, P.p); if (d < bd) { bd = d; best = i; } }
        if (best >= 0) { P.a = atan2f(zombies[best].p.y - P.p.y, zombies[best].p.x - P.p.x); P.cd = 0; P.reload = 0; if (P.gun[0].clip == 0) P.gun[0].clip = 6; fire(); } }
    if (f == 150) shot("shot_gore.png");
    if (f == 151) { P.p = Vector2Add(TOWNS[0].c, V2(40, 40)); camPos = P.p; for (int i = 0; i < 6; i++) { Zombie& z = spawnZ(i % 2 ? Z_RUNNER : Z_SOLDIER, Vector2Add(P.p, V2(-6 + i * 2.5f, -7))); killZ(z, PI / 2 + frand(-.3f, .3f), i == 2, 0, false); } for (int i = 0; i < 3; i++) spawnZ(i == 0 ? Z_BRUTE : Z_BLOATER, Vector2Add(P.p, V2(-4 + i * 4.f, 5))); }
    if (f == 154) shot("shot_corpses.png");
    if (f == 156) camZoom = 1.5f;
    if (f == 160) shot("shot_corpses_close.png");
    if (f == 162) camZoom = 1.f;
    if (f == 155) { int b = -1; for (int i = 0; i < (int)blds.size(); i++) if (blds[i].enter && blds[i].town == 0 && blds[i].type == B_HOUSE) { b = i; break; } if (b < 0) for (int i = 0; i < (int)blds.size(); i++) if (blds[i].enter && blds[i].town == 0) { b = i; break; }
        if (b >= 0) { P.p = V2((blds[b].x0 + blds[b].x1) / 2, (blds[b].z0 + blds[b].z1) / 2); camPos = P.p; } }
    if (f == 200) shot("shot_interior.png");
    if (f == 205) { gTime = CYCLE * (16.f / 24.f) + CYCLE; P.light = true; P.batt = 100; P.p = Vector2Add(TOWNS[0].c, V2(10, -10)); camPos = P.p; staticLights.push_back({xz3(Vector2Add(P.p, V2(-6, -4)), 1.2f), rgba(255, 120, 40), 10, 2}); for (int i = 0; i < 8; i++) { Zombie* z = spawnAround(i < 2 ? Z_VOLATILE : Z_WALKER, 5, 12); if (z) z->aggro = 0; } }
    if (f == 250) shot("shot_night.png");
    if (f == 255) { P.p = V2(500, 500); for (auto& t : trees) if (Vector2Distance(t.p, P.p) < 3000 && Vector2Distance(t.p, TOWNS[3].c) < 140) { P.p = Vector2Add(t.p, V2(4, 4)); break; } camPos = P.p; gTime = CYCLE * 1.35f; rainLevel = 1; rainTarget = 1; P.light = false; }
    if (f == 300) shot("shot_forest_rain.png");
    if (f == 305) { rainLevel = 0; rainTarget = 0; gTime = CYCLE * 2.2f; P.p = Vector2Add(TOWNS[4].c, V2(-20, 10)); camPos = P.p; }
    if (f == 30) { printf("START melee=%s guns=%d,%d worn:", MB[P.melee].n, P.hasGun[0], P.hasGun[1]); for (int i = 0; i < CS_COUNT; i++) if (P.eq[i]) printf(" [%s %s %.0f%% %dx%d]", CSN[i], P.eq[i]->name.c_str(), P.eq[i]->cond * 100, gridW(*P.eq[i]), gridH(*P.eq[i])); Gear g = gear(); printf("\n  gear warm=%.2f water=%.2f armor=%.2f slots=%d used=%d rags=%d\n", g.warm, g.water, g.armor, g.slots, g.used, invCount(IT_RAG));
        int left = invAdd(IT_BEANS, 20); printf("INV beans added=%d leftover=%d\n", 20 - left, left);
        auto pack = makeCloth(cdIndex("Hiking Pack"), 2); takeCloth(pack); left = invAdd(IT_BEANS, 20); printf("INV with %s (%dx%d): leftover=%d count=%d\n", pack->name.c_str(), gridW(*pack), gridH(*pack), left, invCount(IT_BEANS));
        invTake(IT_BEANS, 999); auto sh = P.eq[CS_TORSO]; float c0 = sh->cond; invAdd(IT_SEWKIT, 1); applyTool(invFind(IT_SEWKIT), *sh); printf("REPAIR %s %.2f -> %.2f kits=%d\n", sh->name.c_str(), c0, sh->cond, invCount(IT_SEWKIT));
        invAdd(IT_LEATHER, 1); float a0 = clothArmor(*sh); applyTool(invFind(IT_LEATHER), *sh); printf("PATCH armor %.3f -> %.3f\n", a0, clothArmor(*sh)); for (int k = 0; k < 10; k++) wearDamage(20); printf("WEAR shirt cond=%.2f\n", sh->cond);
        for (int k = 0; k < 6; k++) { auto c = makeCloth(clothFor(k % 2 ? F_MIL : F_CIV), rollRarity()); printf("ROLL %s [%s] cond=%.0f%% rar=%d warm=%.2f armor=%.2f water=%.2f grid=%dx%d decor='%s'\n", c->name.c_str(), CSN[CD[c->def].slot], c->cond * 100, c->rar, clothWarm(*c), clothArmor(*c), clothWater(*c), gridW(*c), gridH(*c), c->decor.c_str()); }
        int nc = 0, tools = 0, clothInGarage = 0; for (auto& l : loot) { if (l.kind == LK_CLOTH) nc++; if (l.kind == LK_MELEE && l.id >= MK_CROWBAR) tools++; } printf("LOOT clothes=%d newTools=%d total=%d\n", nc, tools, (int)loot.size()); (void)clothInGarage; }
    if (f == 31) { camZoom = 1.6f; gTime = CYCLE * .3f; for (auto& z : zombies) z.gone = true; }
    if (f == 36) { shot("shot_outfit.png"); P.eq[CS_OUTER] = makeCloth(cdIndex("Hunting Coat"), 1); P.eq[CS_LEGS] = makeCloth(cdIndex("Cargo Pants"), 0); P.eq[CS_FEET] = makeCloth(cdIndex("Rubber Boots"), 0); }
    if (f == 40) { shot("shot_outfit2.png"); camZoom = 1.f; }
    if (f == 351) { for (int k = 0; k < 4; k++) spawnCloth(xz3(Vector2Add(P.p, V2(frand(-1, 1), frand(-1, 1))), .05f), makeCloth(clothFor(F_STORE | F_MIL), irand(0, 3))); spawnItem(xz3(P.p, .05f), IT_TAPE, 1); invAdd(IT_BANDAGE, 2); invAdd(IT_WATER, 1); }
    if (f == 350) shot("shot_military.png");
    if (f == 352) state = ST_INV;
    if (f == 356) shot("shot_inventory.png");
    if (f == 360) shot("shot_inventory.png");
    if (f == 362) state = ST_MAP;
    if (f == 370) shot("shot_map.png");
    if (f == 372) { state = ST_SKILLS; for (int i = 0; i < 6; i++) { pendingLv++; } P.lvl = 7; grantLevelPoints(); }
    if (f == 376) shot("shot_skills.png");
    if (f == 377) { state = ST_PLAY; if (!hotZones.empty()) { P.p = Vector2Add(hotZones[0].c, V2(3, 6)); camPos = P.p; gTime = CYCLE * 3.25f; dressHotZones(); } }
    if (f == 379) { useActive(); }
    if (f == 381) shot("shot_hotzone.png");
    if (f == 386) { gTime = CYCLE * 4.3f; rainLevel = 0; rainTarget = 0; camZoom = 1.35f; for (auto& z : zombies) z.gone = true;
        if (!lofts.empty()) { P.p = lofts[0].base; camPos = P.p; doAct({A_LADDER, 0}); } }
    if (f == 392) { shot("shot_loft.png"); printf("LOFT loft=%d hideK=%.2f\n", P.loft, P.hideK); }
    if (f == 394) { if (P.loft >= 0) doAct({A_LADDER, P.loft}); int di = -1; for (int i = 0; i < (int)doorObjs.size(); i++) if (blds[doorObjs[i].bld].type == B_HOUSE) { di = i; break; }
        if (di >= 0) { DoorObj& d = doorObjs[di]; d.open = true; Vector2 n = d.horiz ? V2(0, 1.7f) : V2(1.7f, 0); P.p = Vector2Add(d.c, n); if (buildingAt(P.p) >= 0) P.p = Vector2Subtract(d.c, n); camPos = P.p; P.a = atan2f(d.c.y - P.p.y, d.c.x - P.p.x); } }
    if (f == 395) { camZoom = 1.6f; P.p = Vector2Add(P.p, V2(1.2f, 0)); }
    if (f == 396) shot("shot_door_open.png");
    if (f == 397) { for (auto& d : doorObjs) if (Vector2Distance(d.c, P.p) < 3) printf("DOOR c=%.1f,%.1f horiz=%d w=%.1f open=%d anim=%.2f bld=%d cut=%.2f\n", d.c.x, d.c.y, d.horiz, d.w, d.open, d.anim, d.bld, blds[d.bld].cut); nearAct = findAct(); printf("ACT kind=%d label=%s\n", nearAct.kind, actLabel(nearAct).c_str()); if (nearAct.kind == A_DOOR) doAct(nearAct); }
    if (f == 401) shot("shot_door.png");
    if (f == 402) { Vector2 cp = V2(-1, -1); for (auto& c : covers) if (townAt(c.p, 1.2f) < 0 && hotAt(c.p, 1.5f) < 0) { cp = c.p; break; } P.p = cp; camPos = cp; P.crouch = true; P.light = false; gTime = CYCLE * 4.6f;
        invAdd(IT_KINDLING, 8); invAdd(IT_MATCHES, 3); invAdd(IT_LOG, 1); P.melee = MK_AXE; P.active = 2; P.a = 0; buildFire(); }
    if (f == 408) { printf("HIDE hideK=%.2f fires=%d\n", P.hideK, (int)fires.size()); shot("shot_fire_hide.png"); }
    if (f == 410) { int ci = -1; for (int i = 0; i < (int)carObjs.size(); i++) if (!carObjs[i].burned) { ci = i; break; } if (ci >= 0) { size_t n0 = loot.size(); P.p = Vector2Add(carObjs[ci].p, V2(0, 2.6f)); camPos = P.p; doAct({A_CAR, ci}); printf("CAR loot +%d\n", (int)(loot.size() - n0)); } }
    if (f == 412) { P.broken = true; P.splinted = false; craftSplint(); quickUse(true); printf("SPLINT splinted=%d leg=%.2f", P.splinted, legFactor()); P.splintT = CYCLE / 2; printf(" after12h=%.2f", legFactor()); P.splintT = CYCLE * 2.5f; printf(" after2.5d=%.2f\n", legFactor()); }
    if (f == 414) { state = ST_INV; }
    if (f == 418) shot("shot_craft.png");
    if (f == 419) { state = ST_PLAY; camZoom = 1.f; gTime = CYCLE * 5.25f; rainLevel = 0; P.crouch = false; Vector2 bp = P.p; float bd = 1e9f;
        for (auto& t : trees) { float d = Vector2Distance(t.p, TOWNS[1].c); if (d > TOWNS[1].r * 1.4f && d < bd) { bd = d; bp = t.p; } } P.p = Vector2Add(bp, V2(6, 6)); camPos = P.p; }
    if (f == 422) shot("shot_ground.png");
    if (f == 426 - 2) {}
    if (f == 428) { int ov = 0; for (auto& b : blds) if (rectOnRoad(b.x0, b.z0, b.x1, b.z1, 0)) ov++; int ovT = 0; for (auto& b : blds) for (auto& r : roads) if (r.tier > 0 && segDist(V2((b.x0 + b.x1) / 2, (b.z0 + b.z1) / 2), r.a, r.b) < std::min(b.x1 - b.x0, b.z1 - b.z0) / 2 - .5f) { ovT++; break; } printf("TRACKS through_buildings=%d\n", ovT); printf("ROADS touching_any_road=%d of %d\n", ov, (int)blds.size());
        for (auto& z : zombies) z.gone = true; P.hp = P.perks.maxhp; P.a = 0; Zombie& z = spawnZ(Z_WALKER, Vector2Add(P.p, V2(1.3f, 0))); z.losT = 99; float hp0 = P.hp; P.blocking = true; P.sta = 100; hitPlayer(20, z.p, Z_WALKER); float b1 = hp0 - P.hp;
        P.shieldHP = 120; hp0 = P.hp; hitPlayer(20, z.p, Z_WALKER); float b2 = hp0 - P.hp; P.blocking = false; P.iframe = 0; hp0 = P.hp; hitPlayer(20, z.p, Z_WALKER); float b3 = hp0 - P.hp;
        printf("BLOCK dmg bare-block=%.1f shield=%.1f unblocked=%.1f shieldHP=%.0f\n", b1, b2, b3, P.shieldHP); Vector2 zp = z.p; P.kickCd = 0; P.sta = 100; kick(); printf("KICK knock=%.1f stun=%.2f\n", Vector2Length(z.k), z.stun); (void)zp; }
    if (f == 430) { gTime = CYCLE * 6.95f; int b = -1; for (int i = 0; i < (int)blds.size(); i++) if (blds[i].enter && blds[i].type == B_HOUSE && blds[i].x1 - blds[i].x0 > 8) { b = i; break; }
        if (b >= 0) { P.p = V2(blds[b].x0 + 2.2f, (blds[b].z0 + blds[b].z1) / 2); camPos = P.p; P.a = PI; } P.light = true; P.batt = 100; camZoom = 1.3f; }
    if (f == 436) shot("shot_flash_wall.png");
    if (f == 437) { P.a = 0; }
    if (f == 442) shot("shot_flash_room.png");
    if (f == 443) { camZoom = 1.f; state = ST_SETTINGS; }
    if (f == 447) { shot("shot_settings.png"); state = ST_PLAY; }
    if (f == 452) { state = ST_PLAY; camZoom = 1.25f; gTime = CYCLE * 8.3f; rainLevel = 0; rainTarget = 0; for (auto& z : zombies) z.gone = true; int hb = -1;
        for (int i = 0; i < (int)blds.size(); i++) if (blds[i].type == B_HOUSE && blds[i].enter && !blds[i].doors.empty() && blds[i].doors[0].obj >= 0 && blds[i].x1 - blds[i].x0 > 9) { hb = i; break; }
        if (hb >= 0) { Building& b = blds[hb]; P.p = V2((b.x0 + b.x1) / 2, (b.z0 + b.z1) / 2); collide(P.p, .35f); camPos = P.p; P.light = false;
            for (auto& dr : b.doors) if (dr.obj >= 0) { DoorObj& d = doorObjs[dr.obj]; d.open = false; d.broken = false; d.hp = d.hpMax; }
            DoorObj& d0 = doorObjs[b.doors[0].obj]; for (int k = 0; k < 3; k++) { Zombie& z = spawnZ(Z_WALKER, Vector2Add(Vector2Add(d0.c, Vector2Scale(d0.outN, 1.8f)), V2(frand(-1.f, 1.f), 0))); z.aggro = 30; z.losT = 0; }
            printf("HOUSE type=%d cond=%d door metal=%d hp=%.0f c=%.1f,%.1f outN=%.1f,%.1f P=%.1f,%.1f bld=%.1f,%.1f-%.1f,%.1f\n", b.type, b.cond, d0.metal, d0.hp, d0.c.x, d0.c.y, d0.outN.x, d0.outN.y, P.p.x, P.p.y, b.x0, b.z0, b.x1, b.z1);
} }
    if (f == 466) shot("shot_house_rooms.png");
    if (f == 478) { for (auto& d : doorObjs) if (Vector2Distance(d.c, P.p) < 12) { printf("POUND door hp=%.0f/%.0f broken=%d\n", d.hp, d.hpMax, d.broken); break; } int np = 0; for (auto& z : zombies) if (z.pound >= 0) np++; printf("POUNDING zombies=%d\n", np); shot("shot_pound.png"); }
    if (f == 479) { for (auto& z : zombies) if (z.pound >= 0) { z.poundT = .05f; z.aggro = 0; } P.moving = false; }
    if (f == 486) { int np = 0; for (auto& z : zombies) if (z.pound >= 0) { np++; printf("  pounder id=%d poundT=%.1f aggro=%.1f sees=%d ignore=%.1f type=%d\n", z.id, z.poundT, z.aggro, z.sees, z.ignoreT, z.type); } printf("GAVEUP still_pounding=%d firedT=%.1f moving=%d\n", np, P.firedT, P.moving); }
    if (f == 480 || f == 482) for (auto& z : zombies) if (z.pound >= 0 || z.ignoreT > 0) printf("  f%d z%d pound=%d poundT=%.2f ignore=%.1f aggro=%.1f sees=%d\n", f, z.id, z.pound, z.poundT, z.ignoreT, z.aggro, z.sees);
    if (f == 488) { for (auto& z : zombies) z.gone = true; int ci = -1; for (int i = 0; i < (int)carObjs.size(); i++) if (townAt(carObjs[i].p, 1.f) == 0) { ci = i; break; } if (ci < 0) ci = 0;
        if (!carObjs.empty()) { P.p = Vector2Add(carObjs[ci].p, V2(0, 6)); camPos = P.p; } camZoom = 1.1f; }
    if (f == 494) shot("shot_cars.png");
    if (f == 496) { bool done = false; for (int i = 0; i < (int)solids.size() && !done; i++) { const Solid& sd = solids[i]; if (sd.kind != SK_FENCE || sd.h > 1.5f) continue; bool hz = sd.x1 - sd.x0 > sd.z1 - sd.z0; Vector2 c = V2((sd.x0 + sd.x1) / 2, (sd.z0 + sd.z1) / 2);
            Vector2 st = hz ? V2(c.x, sd.z0 - .9f) : V2(sd.x0 - .9f, c.y); if (!freeAt(st.x, st.y, .4f)) continue; P.p = st; P.a = hz ? PI / 2 : 0; P.moving = false; P.sta = 100; P.rollCd = 0; P.roll = 0; dodge(); printf("VAULT t=%.2f from %.1f,%.1f to %.1f,%.1f h=%.2f\n", P.vaultT, P.vaultA.x, P.vaultA.y, P.vaultB.x, P.vaultB.y, P.vaultH); done = true; } }
    if (f == 504) { Zombie& z = spawnZ(Z_WALKER, P.p); z.aggro = 0; z.losT = 99; }
    if (f == 510) { float md = 99; for (auto& z : zombies) md = std::min(md, Vector2Distance(z.p, P.p)); printf("SPACING nearest=%.2f\n", md); }
    if (f == 512) { int nb = 0; for (auto& b : blds) if (b.enter) nb++; printf("STATS blds=%d enterable=%d props=%d solids=%d trees=%d loot=%d roads=%d zombies=%d decals=%d gibs=%d corpses=%d fps=%d\n", (int)blds.size(), nb, (int)props.size(), (int)solids.size(), (int)trees.size(), (int)loot.size(), (int)roads.size(), (int)zombies.size(), (int)decals.size(), (int)gibs.size(), (int)corpses.size(), GetFPS()); fflush(stdout); }
    if (f == 514) { int up = 0, dn = 0; for (auto& st : stairs) (st.down ? dn : up)++; printf("CELLS stairs up=%d down=%d blds=%d\n", up, dn, (int)blds.size());
        for (auto& z : zombies) z.gone = true; gTime = CYCLE * 8.25f; for (int i = 0; i < (int)stairs.size(); i++) if (stairs[i].down) { P.p = Vector2Add(stairs[i].a, V2(.6f, 0)); camPos = P.p; nearAct = findAct(); printf("STAIRS act=%d label=%s\n", nearAct.kind, actLabel(nearAct).c_str()); spawnZ(Z_WALKER, Vector2Add(P.p, V2(4, 0))).aggro = 20; doAct(nearAct);
            int cb = cellOf(P.p); printf("BASEMENT inCell=%d cell=%d kind=%d dark=%.2f followers=%d loot_here=%d\n", inCell(P.p), cb, cb >= 0 ? blds[cb].cellKind : -1, darkK(), (int)followers.size(), (int)std::count_if(loot.begin(), loot.end(), [](const Loot& l) { return Vector2Distance(V2(l.p.x, l.p.z), P.p) < 12; })); P.light = true; break; } }
    if (f == 522) { int nz = 0; for (auto& z : zombies) if (Vector2Distance(z.p, P.p) < 15) nz++; printf("BASEMENT zombies_near=%d followers_left=%d\n", nz, (int)followers.size()); shot("shot_basement.png"); }
    if (f == 524) { for (auto& z : zombies) z.gone = true; gTime = CYCLE * .45f; P.light = false; for (int i = 0; i < (int)stairs.size(); i++) if (!stairs[i].down) { P.p = Vector2Add(stairs[i].a, V2(.5f, 0)); camPos = P.p; nearAct = findAct(); doAct(nearAct); int cb = cellOf(P.p); printf("UPSTAIRS cell=%d kind=%d spots=%d\n", cb, cb >= 0 ? blds[cb].cellKind : -1, cb >= 0 ? (int)blds[cb].spots.size() : -1); break; } }
    if (f == 530) shot("shot_upstairs.png");
    if (f == 532) { nearAct = findAct(); printf("BACK label=%s\n", actLabel(nearAct).c_str()); doAct(nearAct); printf("BACK inCell=%d\n", inCell(P.p)); }
    if (f == 535) { int bad = 0, tested = 0, badStairs = 0; // walk test: from inside the front door, can you reach every doorway and stair?
        for (int bi = 0; bi < (int)blds.size(); bi++) { Building& b = blds[bi]; if (!b.enter || b.doors.empty() || b.doors[0].obj < 0) continue; tested++;
            const float S = .3f; int nx = (int)((b.x1 - b.x0) / S) + 1, nz = (int)((b.z1 - b.z0) / S) + 1; std::vector<char> seen(nx * nz, 0); std::vector<int> q;
            Vector2 st = Vector2Subtract(doorObjs[b.doors[0].obj].c, Vector2Scale(doorObjs[b.doors[0].obj].outN, .9f));
            int sx = (int)((st.x - b.x0) / S), sz = (int)((st.y - b.z0) / S); if (sx < 0 || sz < 0 || sx >= nx || sz >= nz) continue; seen[sz * nx + sx] = 1; q.push_back(sz * nx + sx);
            for (size_t k = 0; k < q.size(); k++) { int cx = q[k] % nx, cz = q[k] / nx; static const int D4[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
                for (auto& dd : D4) { int ax = cx + dd[0], az = cz + dd[1]; if (ax < 0 || az < 0 || ax >= nx || az >= nz || seen[az * nx + ax]) continue; float wx = b.x0 + ax * S, wz = b.z0 + az * S; if (!freeAt(wx, wz, .3f)) continue; seen[az * nx + ax] = 1; q.push_back(az * nx + ax); } }
            auto reach = [&](Vector2 p) { for (int dz = -2; dz <= 2; dz++) for (int dx = -2; dx <= 2; dx++) { int ax = (int)((p.x - b.x0) / S) + dx, az = (int)((p.y - b.z0) / S) + dz; if (ax >= 0 && az >= 0 && ax < nx && az < nz && seen[az * nx + ax]) return true; } return false; };
            bool ok = true; for (auto& g : b.gaps) if (!reach(g)) ok = false; for (auto& stp : stairs) if (stp.bA == bi && !reach(stp.a)) { ok = false; badStairs++; }
            if (!ok) { bad++; if (bad <= 3) printf("  WALKFAIL type=%d at %.0f,%.0f gaps=%d\n", b.type, b.x0, b.z0, (int)b.gaps.size()); } }
        printf("WALKTEST tested=%d blocked=%d stairs_blocked=%d\n", tested, bad, badStairs); }
    if (f == 536) { int bt[18] = {}; for (auto& b : blds) if (!b.cellKind) bt[b.type]++; printf("BTYPES house=%d store=%d garage=%d barn=%d cabin=%d shed=%d factory=%d fire=%d school=%d trailer=%d gas=%d\n", bt[B_HOUSE], bt[B_STORE], bt[B_GARAGE], bt[B_BARN], bt[B_CABIN], bt[B_SHED], bt[B_FACTORY], bt[B_FIRE], bt[B_SCHOOL], bt[B_TRAILER], bt[B_GAS]);
        int rt[3] = {}; for (auto& r : roads) rt[r.tier]++; printf("ROADTIERS paved=%d gravel=%d dirt=%d\n", rt[0], rt[1], rt[2]);
        for (auto& z : zombies) z.gone = true; gTime = CYCLE * .4f; camZoom = .8f; for (auto& b : blds) if (b.type == B_TRAILER) { P.p = V2(b.x1 + 3, b.z1 + 2); camPos = P.p; break; } }
    if (f == 540) shot("shot_trailers.png");
    if (f == 541) { for (auto& b : blds) if (b.type == B_GAS) { P.p = V2(b.x1 + 4, b.z1 + 6); camPos = P.p; break; } }
    if (f == 545) shot("shot_gas.png");
    if (f == 546) { for (auto& b : blds) if (b.type == B_BARN && !b.cellKind) { P.p = V2(b.x1 + 8, b.z0 + 4); camPos = P.p; break; } camZoom = .7f; }
    if (f == 550) shot("shot_farm.png");
    if (f == 551) { for (auto& b : blds) if (b.type == B_FACTORY) { P.p = V2((b.x0 + b.x1) / 2, (b.z0 + b.z1) / 2); camPos = P.p; break; } camZoom = 1.f; }
    if (f == 555) shot("shot_factory.png");
    if (f == 556) { for (auto& b : blds) if (b.type == B_FIRE) { P.p = V2((b.x0 + b.x1) / 2, (b.z0 + b.z1) / 2 + 1); camPos = P.p; break; } }
    if (f == 559) shot("shot_firestation.png");
    if (f == 640) { state = ST_PLAY; P.cls = 0; for (const char* n : {"Riot Helmet", "Police Shirt", "Police Vest", "Combat Boots"}) { auto c = makeCloth(cdIndex(n), 0); makeSetPiece(*c, 0); P.eq[CD[c->def].slot] = c; } recalcPerks();
        printf("SETS police=%d armor=%.2f pistol=%.2f cdr=%.2f name=%s\n", setCount(0), P.perks.armor, P.perks.pistol, P.perks.cdr, P.eq[CS_TORSO]->name.c_str());
        invAdd(IT_BANDAGE, 3); P.hp = 55; P.lvl = 4; kills = 17; hotbar[7] = {1, IT_BANDAGE}; Vector2 pp = P.p; int nl = (int)loot.size(); int d0 = -1; for (int i = 0; i < (int)doorObjs.size(); i++) if (!doorObjs[i].broken) { doorObjs[i].broken = true; d0 = i; break; }
        saveGame(); printf("SAVE wrote=%d size=%ld\n", hasSave(), (long)GetFileLength(runSavePath().c_str()));
        P.hp = 1; P.eq[CS_TORSO].reset(); kills = 0; loadGame();
        printf("LOAD hp=%.0f lvl=%d kills=%d torso=%s set=%d bandages=%d hot7=%d pos_ok=%d loot=%d/%d door_broken=%d\n", P.hp, P.lvl, kills, P.eq[CS_TORSO] ? P.eq[CS_TORSO]->name.c_str() : "none", setCount(0), invCount(IT_BANDAGE), hotbar[7].id, Vector2Distance(pp, P.p) < .01f, (int)loot.size(), nl, d0 >= 0 ? doorObjs[d0].broken : -1);
        deleteSave(); printf("SAVE deleted=%d\n", !hasSave()); }
    if (f == 642) { state = ST_INV; }
    if (f == 645) { int up = 0; for (auto& l : lofts) { P.loft = (int)(&l - &lofts[0]); P.p = l.top; up++; break; } float x0 = P.p.x; for (int k = 0; k < 30; k++) { P.p.x -= .2f; } printf("LOFT walkable test start=%.1f\n", x0); state = ST_PLAY; }
    if (f == 650) { printf("LOFT pos clamp ok=%d loft=%d\n", P.loft >= 0 && P.p.x >= lofts[P.loft].x0, P.loft); P.loft = -1; CloseWindow(); exit(0); }
    if (f == 600) { gTime = CYCLE * .42f; rainLevel = 0; rainTarget = 0; for (auto& z : zombies) z.gone = true; camZoom = .75f; state = ST_PLAY; P.hp = 1e6f;
        Vector2 c = V2(MAP * .5f, MAP * .5f); for (int t = 0; t < 400; t++) { Vector2 q = V2(frand(200, MAP - 200), frand(200, MAP - 200)); if (roadDist(q) < 5 && townAt(q, 1.2f) < 0) { c = q; break; } } P.p = c; camPos = c;
        for (int i = 0; i < 70; i++) { Vector2 q = Vector2Add(c, polar(frand(-.6f, .6f), frand(7, 22))); if (!freeAt(q.x, q.y, .4f)) continue; Zombie& z = spawnZ(i % 9 == 0 ? Z_RUNNER : i % 23 == 0 ? Z_BRUTE : Z_WALKER, q); z.aggro = 20; } P.a = 0; }
    if (f == 615) shot("shot_horde_day.png");
    if (f == 616) { gTime = CYCLE * .9f; P.light = true; P.batt = 100; for (auto& z : zombies) z.gone = true; Vector2 c = P.p; for (int i = 0; i < 80; i++) { Vector2 q = Vector2Add(c, polar(frand(0, 6.28f), frand(5, 16))); if (freeAt(q.x, q.y, .4f)) spawnZ(i % 7 == 0 ? Z_RUNNER : Z_WALKER, q).aggro = 20; } }
    if (f == 628) shot("shot_horde_night.png");
    if (f == 630) { CloseWindow(); exit(0); }
    if (f == 560) { printf("DOORS %d LOFTS %d COVERS %d CARS %d\n", (int)doorObjs.size(), (int)lofts.size(), (int)covers.size(), (int)carObjs.size()); for (int i = 0; i < 6; i++) printf("TOWN %s %.0f %.0f\n", TOWNS[i].name, TOWNS[i].c.x, TOWNS[i].c.y); for (auto& h : hotZones) printf("HOT %s %.0f %.0f\n", HZN[h.kind], h.c.x, h.c.y); printf("CLASS %s pts=%d\n", CL().n, P.points); fflush(stdout); CloseWindow(); exit(0); }
}
#endif
