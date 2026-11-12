// Nightfall County: update player.
// Not a standalone header: included once, in order, from src/main.cpp. Everything above it in that list is visible here.
#pragma once
// ============================================================ update
static std::vector<int> zg[GN * 2][GN * 2]; static std::vector<int> znb;
static int zcl(float v) { return std::max(0, std::min(GN * 2 - 1, (int)(v / (GCELL / 2)))); }
static std::vector<int>& zNear(Vector2 p) { znb.clear(); int cx = zcl(p.x), cz = zcl(p.y); for (int i = cx - 1; i <= cx + 1; i++) for (int j = cz - 1; j <= cz + 1; j++) { if (i < 0 || j < 0 || i >= GN * 2 || j >= GN * 2) continue; for (int k : zg[i][j]) znb.push_back(k); } return znb; }
static float stepT = 0;
static void updatePlayer(float dt) {
    P.alive += dt;
    Vector2 in = V2((float)((IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) - (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))), (float)((IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) - (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))));
    float il = Vector2Length(in); if (il > 0) in = Vector2Scale(in, 1 / il);
    Vector2 ap = aimPoint(); P.a = atan2f(ap.y - P.p.y, ap.x - P.p.x);
    // click-to-move (Diablo): left-click the ground to walk, a zombie to go hit it, loot to go grab it. WASD still overrides
    static int cm = 0, cmZ = -1; static Vector2 cmPt{}; bool cmSwing = false, cmFire = false, cmPick = false;
    if (cfg.controls == 2 && state == ST_PLAY && !uiOpen && P.loft < 0) {
        bool lp = IsMouseButtonPressed(MOUSE_BUTTON_LEFT), ld = IsMouseButtonDown(MOUSE_BUTTON_LEFT), alt = IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT); Vector2 gp = ap;
        int onZ = -1; float bz = 1.1f; for (auto& z : zombies) if (!z.dead) { float dd = Vector2Distance(z.p, gp); if (dd < bz) { bz = dd; onZ = z.id; } }
        if (lp && !alt) { if (onZ >= 0) { cm = 2; cmZ = onZ; } else { float bl = .9f; int li = -1; for (int i = 0; i < (int)loot.size(); i++) { float dd = Vector2Distance(V2(loot[i].p.x, loot[i].p.z), gp); if (dd < bl) { bl = dd; li = i; } }
                if (li >= 0) { cm = 3; cmPt = V2(loot[li].p.x, loot[li].p.z); } else { cm = 1; cmPt = gp; } } }
        if (ld && cm == 1 && !alt) cmPt = gp; // hold to keep walking toward the cursor
        if (il > 0) cm = 0;
        bool gun = P.active < 2 && P.hasGun[P.active];
        cmFire = ld && (alt || (gun && onZ >= 0) || (gun && cm == 2));
        if (cm == 2) { Zombie* t = nullptr; for (auto& z : zombies) if (z.id == cmZ && !z.dead) t = &z; if (!t) cm = 0;
            else { float dd = Vector2Distance(t->p, P.p); P.a = atan2f(t->p.y - P.p.y, t->p.x - P.p.x);
                if (gun) { cmFire = ld || cmFire; if (!ld) cm = 0; } else if (dd > MB[P.melee].range + t->r - .25f) { in = Vector2Normalize(Vector2Subtract(t->p, P.p)); il = 1; } else cmSwing = true; } }
        else if (cm == 1 || cm == 3) { Vector2 dv = Vector2Subtract(cmPt, P.p); float dd = Vector2Length(dv); if (dd < (cm == 3 ? .9f : .3f)) { if (cm == 3) cmPick = true; cm = 0; } else { in = Vector2Scale(dv, 1 / dd); il = 1; if (!cmFire) P.a = atan2f(in.y, in.x); } }
        if (alt && !gun && ld) cmSwing = true;
    }
    // autorun (X): keeps going until you touch a movement key, crouch, roll, get hunted, or hit a wall
    if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_A) || IsKeyPressed(KEY_S) || IsKeyPressed(KEY_D) || P.crouch || P.roll > 0 || P.loft >= 0) P.autorun = false;
    if (P.autorun) { for (auto& z : zombies) if (z.sees && Vector2Distance(z.p, P.p) < 25) { P.autorun = false; msg("Autorun off: you've been spotted.", STEEL); break; } }
    if (P.autorun && il == 0) { in = polar(P.autoA, 1); il = 1; }
    // Aim-relative: W walks toward the cursor, S backs off, A/D circle-strafe. Classic: WASD are the 8 compass directions.
    if (cfg.controls == 1 && il > 0 && !(P.autorun && !(IsKeyDown(KEY_W) || IsKeyDown(KEY_S) || IsKeyDown(KEY_A) || IsKeyDown(KEY_D) || IsKeyDown(KEY_UP) || IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_RIGHT)))) {
        Vector2 aimD = Vector2Subtract(ap, P.p); bool close = Vector2Length(aimD) < 1.2f; // cursor on top of you: don't spin in place
        float fwd = -in.y, str = in.x, base = close ? -PI / 2 : P.a; in = Vector2Normalize(Vector2Add(polar(base, fwd), polar(base + PI / 2, str))); }
    P.moving = il > 0; if (il > 0) P.moveA = atan2f(in.y, in.x);
    float dirK = P.blocking ? .6f : 1; if (il > 0) { float df = fabsf(angDiff(P.moveA, P.a)); P.back = P.back ? df > 1.75f : df > 2.1f; dirK *= P.back ? .7f : df > 1.1f ? .85f : 1; } // backpedal 70%, strafe 85%
    P.actCd = std::max(0.f, P.actCd - dt); P.actT -= dt; P.firedT -= dt; P.kickCd -= dt; P.kickT -= dt; P.blockFx -= dt;
    bool rmb = IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && state == ST_PLAY && !uiOpen; P.shoveCd -= dt;
    if (rmb) P.rmbT += dt; else { if (P.rmbT > 0 && P.rmbT < .2f) shove(); P.rmbT = 0; }   // tap = shove, hold = block
    P.blocking = rmb && P.rmbT > .15f && P.roll <= 0 && P.loft < 0 && P.vaultT <= 0;
    if (P.broken && P.splinted) { P.splintT += dt; if (legFactor() >= .999f) { P.broken = false; P.splinted = false; msg("Your leg's healed. Took long enough.", OKC); } }
    P.iframe -= dt; P.hurt -= dt; P.staD -= dt; P.rollCd -= dt; P.swing -= dt; P.swingCd -= dt; P.pain = std::max(0.f, P.pain - dt);
    float noiseR = 0;
    if (P.vaultT > 0) { P.vaultT -= dt; float t = 1 - std::max(0.f, P.vaultT) / .45f; P.p = Vector2Lerp(P.vaultA, P.vaultB, t); P.vel = V2(0, 0); }
    else if (P.roll > 0) { P.roll -= dt; P.p = Vector2Add(P.p, Vector2Scale(P.rv, dt)); }
    else {
        P.sprint = (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) && il > 0 && P.sta > 1 && !P.broken && !P.crouch;
        static float mudK = 1, mudChk = 0; if ((mudChk -= dt) <= 0) { mudChk = .25f; mudK = 1; if (rainLevel > .3f && !inCell(P.p)) for (auto& r : roads) if (r.tier == 2 && segDist(P.p, r.a, r.b) < r.w / 2 + .3f) { mudK = 1 - .25f * std::min(1.f, rainLevel); break; } }
        float sp = mudK * 3.4f * (P.sprint ? 1.85f : P.crouch ? .5f : 1) * (P.reload > 0 ? .8f : 1) * legFactor() * dirK * (P.food <= 0 || P.water <= 0 ? .8f : 1) * P.perks.speed;
        if (P.sprint) { P.sta -= 16 * P.perks.sta * dt; P.staD = .8f; }
        Vector2 tv = Vector2Scale(in, sp); P.vel = Vector2Lerp(P.vel, tv, 1 - expf(-dt * (il > 0 ? 14.f : 18.f))); // quick but not instant: no more snapping on diagonals
        Vector2 mv = Vector2Scale(P.vel, dt); P.p = Vector2Add(P.p, mv); P.walked += Vector2Length(mv);
        if (il > 0) { P.step += dt * sp; noiseR = (P.sprint ? 22.f : P.crouch ? 2.5f : 9.f) * P.perks.noise * gear().noise; stepT -= dt * sp; if (stepT <= 0) { stepT = 1.3f; sfx(SFX_STEP, P.crouch ? .2f : P.sprint ? .55f : .35f, .2f); } }
    }
    if (P.staD <= 0) P.sta = std::min(100.f, P.sta + 18 * dt * (P.food > 20 ? 1 : .4f));
    if (P.loft >= 0) { const Loft& L = lofts[P.loft]; P.k = V2(0, 0); P.p.x = Clamp(P.p.x, L.x0 + .35f, L.x1 - 1.3f); P.p.y = Clamp(P.p.y, L.z0 + .35f, L.z1 - .35f); // walk the loft; bales line the back wall
        if (P.food > 15 && P.water > 15 && P.bleeds == 0 && P.sick <= 0) P.hp = std::min(P.perks.maxhp, P.hp + .35f * dt); } else if (P.vaultT > 0) {} else { Vector2 before = P.p; P.p = Vector2Add(P.p, Vector2Scale(P.k, dt)); P.k = Vector2Scale(P.k, powf(.002f, dt)); collide(P.p, .35f);
        static float stuck = 0; if (P.autorun && il > 0) { stuck = Vector2Distance(before, P.p) < .3f * dt ? stuck + dt : 0; if (stuck > .4f) { P.autorun = false; stuck = 0; } } }
    // hiding: crouched in tall grass or a bush
    { bool inCover = false; for (auto& c : covers) if (fabsf(c.p.x - P.p.x) < 2 && fabsf(c.p.y - P.p.y) < 2 && Vector2Distance(c.p, P.p) < c.r) { inCover = true; break; }
      P.hideK = (inCover && P.crouch && !P.light) ? (il > 0 ? .35f : .12f) : 1;
      { int hb = buildingAt(P.p); bool shut = hb >= 0 && !blds[hb].doors.empty(); if (shut) for (auto& dr : blds[hb].doors) if (dr.obj >= 0 && doorObjs[dr.obj].open) shut = false; if (!inCover && P.crouch && !P.light && shut) P.hideK = .3f; }
      if (P.loft >= 0) P.hideK = .2f; }
    { static float tickT = 0; if (gExposure >= .6f) { tickT -= dt; if (tickT <= 0) { sfx(SFX_BEEP, .22f, .05f); tickT = gExposure > .85f ? .18f : .45f; } } else tickT = 0; } // warning tick at orange and red
    gExposure = 0;
    { static float pkT = 0; if ((pkT -= dt) <= 0) { pkT = .5f; recalcPerks(); } } // picks up set bonuses as you change clothes
    nearAct = findAct(); { int cb = cellOf(P.p); inBasement = cb >= 0 && blds[cb].cellKind == 2; }
    if (noiseR > 0) { static float nt = 0; nt -= dt; if (nt <= 0) { nt = .5f; noiseAt(P.p, noiseR); } }
    // survival
    float burn = (P.sprint ? 2.2f : 1) * P.perks.metab;
    P.food = std::max(0.f, P.food - .055f * burn * dt); P.water = std::max(0.f, P.water - .085f * burn * (P.sick > 0 ? 1.8f : 1) * dt);
    int bi = buildingAt(P.p); bool inside = bi >= 0; float fireNear = 0;
    for (auto& l : staticLights) if (l.kind == 2 && Vector2Distance(P.p, V2(l.p.x, l.p.z)) < 5) fireNear = 1;
    for (auto& f : fires) if (Vector2Distance(P.p, f.p) < 4.5f) fireNear = 1.2f;
    Gear gr = gear(); bool sheltered = inside || P.loft >= 0;
    if (rainLevel > .1f && !sheltered) P.wet = std::min(1.f, P.wet + rainLevel * .02f * (1 - gr.water) * dt); else P.wet = std::max(0.f, P.wet - (fireNear > 0 ? .03f : inside ? .006f : .003f) * dt);
    float target = 37.f - darkK() * 1.6f - rainLevel * 1.4f * (inside ? .15f : 1) - P.wet * 1.8f + (inside ? .6f : 0) + fireNear * 2.5f + (P.sprint ? .6f : 0) + std::min(1.f, gr.warm) * 2.2f - .9f;
    float rate = target < P.temp ? .012f * P.perks.cold : .03f; P.temp += (target - P.temp) * rate * dt * 1.5f;
    P.temp = Clamp(P.temp, 33.f, 38.5f);
    P.shiver = Clamp((35.8f - P.temp) / 1.5f, 0, 1);
    if (P.bleeds > 0) { P.hp -= .45f * P.bleeds * dt; static float bt = 0; bt -= dt; if (bt <= 0) { bt = .25f; decal(Vector2Add(P.p, V2(frand(-.2f, .2f), frand(-.2f, .2f))), frand(.06f, .14f), BLOOD_DRY); } }
    if (P.sick > 0) { P.hp -= .12f * dt; P.sick += dt; }
    if (P.food <= 0) P.hp -= .35f * dt; if (P.water <= 0) P.hp -= .55f * dt;
    if (P.temp < 35.f) P.hp -= .25f * dt;
    bool healthy = P.food > 45 && P.water > 45 && P.bleeds == 0 && P.sick <= 0 && P.temp > 35.5f;
    if (healthy) P.hp = std::min(P.perks.maxhp, P.hp + (.25f + P.perks.regen) * dt);
    if (P.hp <= 0) { die(); return; }
    // weapons
    P.cd -= dt; if (P.reload > 0) { P.reload -= dt; if (P.reload <= 0) finishReload(); }
    bool md = IsMouseButtonDown(MOUSE_BUTTON_LEFT) && state == ST_PLAY && !uiOpen; if (cfg.controls == 2) { md = cmFire || cmSwing; if (cmSwing) P.fired = false; }
    if (md && !P.fired) { if (P.active == 2) { melee(); P.fired = true; } else if (fire()) P.fired = !curGun()->autoF; }
    if (!md) P.fired = false;
    if (P.light) { P.batt -= (P.uv ? 1.6f : .55f) * P.perks.batt * dt; if (P.batt <= 0) { P.batt = 0; P.light = false; P.uv = false; msg("Flashlight died.", UVC); } }
    // loot
    nearLoot = -1; float nd = 1.8f;
    { float ly = P.loft >= 0 ? lofts[P.loft].y : 0; for (int i = 0; i < (int)loot.size(); i++) { Loot& l = loot[i]; if (!l.settled || fabsf(l.p.y - ly) > 1.6f) continue; float d = Vector2Distance(V2(l.p.x, l.p.z), P.p); if (d < nd) { nd = d; nearLoot = i; } } }
    if (cmPick && nearLoot >= 0) pickUp();
    // town entry banner
    { int hz = hotAt(P.p, 1.f); if (hz >= 0 && !hotZones[hz].seen) { HotZone& h = hotZones[hz]; h.seen = true; showBanner("HOT ZONE", std::string(HZN[h.kind]) + ". " + HZD[h.kind], rgba(255, 90, 60)); sfx(SFX_HORN, .7f, 0);
        for (int i = 0; i < 8; i++) { Zombie* z = spawnAround(pickType(-1), 18, 34); if (z) { z->noiseT = 10; z->np = P.p; } } } }
    int t = townAt(P.p, 1.0f); if (t != lastTown) { if (t >= 0) showBanner(TOWNS[t].name, TOWNS[t].kind == TK_MILITARY ? "Restricted area. Heavy infection." : TOWNS[t].kind == TK_CITY ? "Population 42,000. Formerly." : "", BONE); lastTown = t; }
}
static void separateFromPlayer() { // last word on spacing: nothing ends a frame standing inside you (volatiles' lunges used to slip through)
    if (P.loft >= 0) return;
    for (auto& z : zombies) { if (z.dead) continue; Vector2 dv = Vector2Subtract(z.p, P.p); float pd = Vector2Length(dv), minD = z.r + .34f; if (pd >= minD) continue;
        Vector2 nrm = pd > 1e-3f ? Vector2Scale(dv, 1 / pd) : polar(P.a, 1); z.p = Vector2Add(P.p, Vector2Scale(nrm, minD)); collide(z.p, z.r);
        if (Vector2Distance(z.p, P.p) < minD * .8f) { P.p = Vector2Subtract(z.p, Vector2Scale(nrm, minD)); collide(P.p, .35f); } } }
