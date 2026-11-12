// Nightfall County: actions.
// Not a standalone header: included once, in order, from src/main.cpp. Everything above it in that list is visible here.
#pragma once
// ============================================================ actions
static Vector2 aimPoint() {
    Ray r = GetScreenToWorldRay(GetMousePosition(), cam3); float y = 1.2f;
    if (fabsf(r.direction.y) < 1e-4f) return P.p; float t = (y - r.position.y) / r.direction.y; return V2(r.position.x + r.direction.x * t, r.position.z + r.direction.z * t);
}
static Weapon* curGun() { return P.active < 2 && P.hasGun[P.active] ? &P.gun[P.active] : nullptr; }
static int curGunHold() { Weapon* g = curGun(); return g ? (g->slot == 1 ? 1 : 2) : 0; }
static void finishReloadNow();
static void startReload() {
    Weapon* w = curGun(); if (!w || P.reload > 0 || w->clip >= w->mag) return;
    if (invCount(ammoItem(w->cal)) <= 0) { floatText(xz3(P.p, 2.2f), std::string("NO ") + CALN[w->cal], rgba(255, 107, 91)); sfx(SFX_EMPTY); return; }
    if (P.actT > 0 && P.cls == 0) { finishReloadNow(); return; }
    P.reload = w->rel / P.perks.reload; sfx(SFX_RELOAD);
}
static void finishReload() { Weapon* w = curGun(); if (!w) return; w->clip += invTake(ammoItem(w->cal), w->mag - w->clip); }
static void finishReloadNow() { finishReload(); sfx(SFX_RELOAD, .7f); }
static void setActive(int i) { if (i == P.active) return; if (i < 2 && !P.hasGun[i]) return; P.active = i; P.reload = 0; P.cd = .2f; sfx(SFX_RELOAD, .5f); }
static bool tryVault();
static void dodge() {
    if (P.vaultT > 0 || P.loft >= 0) return; if (tryVault()) return;
    if (P.roll > 0 || P.rollCd > 0 || P.broken) return; float cost = 25; if (P.sta < cost) return; P.sta -= cost; P.staD = .8f;
    Vector2 in = V2((float)(IsKeyDown(KEY_D) - IsKeyDown(KEY_A)), (float)(IsKeyDown(KEY_S) - IsKeyDown(KEY_W))); if (Vector2Length(in) < .1f) in = polar(P.a, 1);
    P.rv = Vector2Scale(Vector2Normalize(in), 10); P.roll = .28f; P.iframe = .32f; P.rollCd = .45f; P.crouch = false; sfx(SFX_ROLL);
}
static void shove() { // quick two-handed push for the one that's in your face
    if (P.shoveCd > 0 || P.roll > 0 || P.loft >= 0 || P.sta < 6) return; P.sta -= 6; P.staD = .4f; P.shoveCd = .4f; P.kickT = 0; bool hit = false;
    for (auto& z : zombies) { if (z.dead) continue; float d = Vector2Distance(z.p, P.p), a = atan2f(z.p.y - P.p.y, z.p.x - P.p.x);
        if (d < 1.4f + z.r && fabsf(angDiff(a, P.a)) < 1.1f) { z.k = Vector2Add(z.k, polar(a, (z.type == Z_BRUTE ? 1.5f : 5.5f) * P.perks.kick)); z.stun = std::max(z.stun, z.type == Z_BRUTE ? .1f : .5f); z.atk = std::max(z.atk, .5f); hit = true; } }
    sfx(hit ? SFX_HIT_PUNCH : SFX_SWING, hit ? .7f : .35f, .2f); P.blockFx = hit ? .08f : 0;
}
static bool tryVault() { // Space into a low obstacle (fence, crate, car, rock, woodpile) hops it
    Vector2 dir = P.moving ? polar(P.moveA, 1) : polar(P.a, 1); float hb = 0; float tObs = -1;
    for (float t = .35f; t <= 1.2f; t += .15f) { Vector2 q = Vector2Add(P.p, Vector2Scale(dir, t));
        for (int i : sgrid[gcl(q.x)][gcl(q.y)]) { const Solid& sd = solids[i]; if (!solidOn(sd) || q.x < sd.x0 || q.x > sd.x1 || q.y < sd.z0 || q.y > sd.z1) continue;
            if (sd.h > 1.75f || sd.kind == SK_WALL || sd.kind == SK_TOWER || sd.kind == SK_TRUNK || sd.kind == SK_TENT) return false; hb = std::max(hb, sd.h); tObs = t; }
        if (tObs > 0) break; }
    if (tObs < 0) return false;
    for (float t = tObs + .3f; t < tObs + 3.5f; t += .15f) { Vector2 q = Vector2Add(P.p, Vector2Scale(dir, t)); if (freeAt(q.x, q.y, .36f)) {
        if (P.sta < 10) return true; P.sta -= 10; P.staD = .6f; P.vaultA = P.p; P.vaultB = q; P.vaultT = .45f; P.vaultH = hb; P.iframe = .2f; P.crouch = false; P.autorun = false; sfx(SFX_PICK, .8f); sfx(SFX_STEP, .6f); return true; } }
    return false;
}
static void kick() {
    if (P.kickCd > 0 || P.roll > 0 || P.loft >= 0 || P.sta < 14 || P.broken) return; P.sta -= 14; P.staD = .6f; P.kickCd = .75f; P.kickT = .35f; bool hit = false;
    for (auto& z : zombies) { if (z.dead) continue; float d = Vector2Distance(z.p, P.p), a = atan2f(z.p.y - P.p.y, z.p.x - P.p.x);
        if (d < 1.7f + z.r && fabsf(angDiff(a, P.a)) < .95f) { float kb = (z.type == Z_BRUTE ? 2.5f : 8.f) * P.perks.kick; hurtZ(z, 9 * P.perks.kick * P.perks.melee, false, a, false); z.k = Vector2Add(z.k, polar(a, kb)); z.stun = z.type == Z_BRUTE ? .25f : .8f; hit = true; } }
    sfx(hit ? SFX_HIT_PUNCH : SFX_SWING, hit ? 1.f : .5f, .15f); if (hit) { sfx(SFX_HIT_HEAVY, .5f, .1f); shake = std::min(.35f, shake + .12f); }
    if (!hit) for (auto& d : doorObjs) if (!d.open && !d.broken && Vector2Distance(d.c, P.p) < 1.9f) { d.open = true; sfx(SFX_DOOR_BANG); noiseAt(d.c, 15); break; } // boot a closed door open
    noiseAt(P.p, 8 * P.perks.noise);
}
static void melee() {
    const MBase& m = MB[P.melee]; if (P.swingCd > 0 || P.roll > 0 || P.sta < m.sta || P.blocking) return;
    P.sta -= m.sta; P.staD = .6f; P.swing = .22f; P.swingCd = m.rate; P.swingA = P.a; sfx(SFX_SWING); bool hit = false;
    for (auto& z : zombies) {
        if (z.dead) continue; float d = Vector2Distance(z.p, P.p), a = atan2f(z.p.y - P.p.y, z.p.x - P.p.x);
        if (d < m.range + z.r && (fabsf(angDiff(a, P.a)) < m.arc || d < z.r + .75f)) { /* point-blank always connects */ bool rage = P.actT > 0 && P.cls == 4; bool hd = chance(P.perks.crit + .08f); hurtZ(z, m.dmg * P.perks.melee * P.perks.dmg * (rage ? 2 : 1), hd, a, false, m.dism + (rage ? .2f : 0)); if (rage) P.hp = std::min(P.perks.maxhp, P.hp + 4); z.k = Vector2Add(z.k, polar(a, 4)); hit = true; }
    }
    if (hit) { static const int mat[MK_COUNT] = {SFX_HIT_PUNCH, SFX_HIT_SLICE, SFX_HIT_SLICE, SFX_HIT_HEAVY, SFX_HIT_WOOD, SFX_HIT_HEAVY, SFX_HIT_HEAVY, SFX_HIT_HEAVY, SFX_HIT_HEAVY}; sfx(mat[P.melee], 1, .1f); if (canChop(P.melee)) sfx(SFX_CHOP, .6f); shake = std::min(.4f, shake + .15f); }
    else if (canChop(P.melee)) { // nothing alive in reach: chop the nearest tree in front of you
        Tree* tb = nullptr; float bd = 1e9f;
        for (int cx = gcl(P.p.x - 4); cx <= gcl(P.p.x + 4); cx++) for (int cz = gcl(P.p.y - 4); cz <= gcl(P.p.y + 4); cz++) for (int ti : tgrid[cx][cz]) { Tree& t = trees[ti];
            float d = Vector2Distance(t.p, P.p), a = atan2f(t.p.y - P.p.y, t.p.x - P.p.x); if (d < m.range + .5f && fabsf(angDiff(a, P.a)) < m.arc && d < bd) { bd = d; tb = &t; } }
        if (tb) { tb->chops++; tb->shake = .35f; sfx(SFX_CHOP, 1, .15f); for (int i = 0; i < 6; i++) parts.push_back({xz3(Vector2Lerp(P.p, tb->p, .8f), 1.0f), V3(frand(-2, 2), frand(1, 3), frand(-2, 2)), .5f, .5f, .05f, rgba(200, 160, 110), P_DUST});
            if (tb->logs >= 3) msg("Nothing left worth taking from this one.", STEEL);
            else if (tb->chops % 4 == 0) { tb->logs++; Loot l; l.kind = LK_ITEM; l.id = IT_LOG; l.count = 1; l.p = xz3(Vector2Lerp(tb->p, P.p, .5f), .05f); l.rot = frand(0, 6); loot.push_back(l); msg("A log comes loose.", BONE); } }
    }
    noiseAt(P.p, 6 * P.perks.noise);
}
static bool fire() {
    Weapon* wp = curGun(); if (!wp) { melee(); return false; } Weapon& w = *wp;
    if (P.reload > 0 || P.cd > 0 || P.roll > 0 || P.swing > 0 || P.blocking) return false;
    if (w.clip <= 0) { startReload(); if (P.reload <= 0) sfx(SFX_EMPTY); return false; }
    w.clip--; P.firedT = 2; P.cd = w.rate / (P.perks.rate * (P.actT > 0 && P.cls == 0 ? 1.6f : 1)); float sm = (P.sprint ? 1.8f : P.crouch ? .6f : 1) * (1 + P.shiver * .8f) * (P.pain > 0 ? 1.4f : 1);
    Vector2 m0 = Vector2Add(P.p, polar(P.a, .55f + w.len));
    for (int i = 0; i < w.pel; i++) { float a = P.a + frand(-w.spread, w.spread) * sm, sp = w.spd * frand(.94f, 1.04f); Bullet b{}; b.p = m0; b.v = polar(a, sp); b.dmg = w.dmg * P.perks.dmg * (w.key == W_SHOTGUN ? P.perks.shotgun : w.cal == C_9MM ? P.perks.pistol : P.perks.rifle); b.life = w.range / w.spd; b.pierce = w.pierce;
        b.burn = w.burn; b.leech = w.leech; b.crit = w.crit; b.dism = w.dism; b.shotgun = w.key == W_SHOTGUN; bullets.push_back(b); }
    shake = std::min(.6f, shake + w.kick * .02f); lights.push_back({xz3(m0, 1.3f), 14, .06f, .06f, rgba(255, 200, 120)});
    for (int i = 0; i < 5; i++) { Vector2 h = polar(P.a + frand(-.4f, .4f), frand(2, 9)); parts.push_back({xz3(m0, 1.3f), V3(h.x, frand(-.5f, .8f), h.y), .07f, .07f, .05f, rgba(255, 210, 120), P_SPARK}); }
    { Vector2 h = polar(P.a + 1.7f, frand(1.5f, 3)); parts.push_back({xz3(P.p, 1.3f), V3(h.x, 2, h.y), .8f, .8f, .025f, rgba(214, 170, 80), P_CASING}); }
    parts.push_back({xz3(m0, 1.3f), V3(0, .4f, 0), .5f, .5f, .25f, rgba(140, 140, 145, 120), P_SMOKE});
    P.k = Vector2Subtract(P.k, polar(P.a, w.kick * .25f));
    noiseAt(P.p, w.noise); static const int sid[3] = {SFX_PISTOL, SFX_SMG, SFX_SHOTGUN}; sfx(w.key == W_HUNT || w.cal == C_556 ? SFX_RIFLE : sid[w.snd], w.cal == C_556 ? .75f : 1);
    if (w.clip == 0) startReload(); return true;
}
static void throwItem(bool flareK, bool free) {
    int id = flareK ? IT_FLARE : IT_PIPEBOMB; if (!free && !invTake(id, 1)) { msg(std::string("No ") + IDEF[id].n + "s.", STEEL); return; }
    Vector2 tgt = aimPoint(); float d = std::min(22.f, Vector2Distance(tgt, P.p)); float t = .9f; Vector2 h = polar(P.a, d / t);
    bombs.push_back({xz3(P.p, 1.5f), V3(h.x, 4.5f, h.y), flareK ? 60.f : 3.2f, 0, 0, flareK}); sfx(SFX_THROW);
}
static void toggleLight() { if (!P.light && P.batt <= 0) { msg("Flashlight battery is dead.", UVC); return; } P.light = !P.light; if (!P.light) P.uv = false; }
static void buildFire() {
    if (buildingAt(P.p) >= 0) { msg("Not indoors. You'd smoke yourself out.", STEEL); return; }
    if (invCount(IT_KINDLING) < 3) { msg("A fire needs 3 kindling.", STEEL); return; }
    if (invCount(IT_MATCHES) < 1) { msg("Nothing to light it with. Find matches.", STEEL); return; }
    Vector2 p = Vector2Add(P.p, polar(P.a, 1.1f)); if (!freeAt(p.x, p.y, .6f)) { msg("No room for a fire here.", STEEL); return; }
    invTake(IT_KINDLING, 3); invTake(IT_MATCHES, 1); fires.push_back({p, 150}); msg("Fire's going. Feed it kindling or logs with E.", rgba(255, 170, 90)); sfx(SFX_THROW);
}
static void craftSplint() {
    if (invCount(IT_KINDLING) < 2 || invCount(IT_BANDAGE) + invCount(IT_RAG) < 1) { msg("Splint: 2 kindling + 1 bandage or rag.", STEEL); return; }
    invTake(IT_KINDLING, 2); if (!invTake(IT_RAG, 1)) invTake(IT_BANDAGE, 1); sfx(SFX_PICK);
    if (P.broken && !P.splinted) { P.splinted = true; P.splintT = 0; msg("Made a splint and strapped it on. The leg will come back slowly.", OKC); showBanner("LEG SPLINTED", "Slow at first, better in 12 hours", OKC); return; }
    if (invAdd(IT_SPLINT, 1)) { spawnItem(xz3(P.p, .05f), IT_SPLINT, 1); msg("Made a splint. No room, so it's on the ground.", SODIUM); } else msg("Made a splint. It's in your pockets for when you need it.", OKC);
}
static float legFactor() { // broken: 45%. splint: 52% -> 70% over 12 in-game hours -> 100% over 2 more days
    if (!P.broken) return 1; if (!P.splinted) return .45f; float t = P.splintT, h12 = CYCLE / 2, d2 = CYCLE * 2;
    if (t < h12) return .52f + .18f * t / h12; return .70f + .30f * std::min(1.f, (t - h12) / d2);
}

static void useItem(InvItem* it);
static void quickUse(bool meds) { // B: bandage (or a rag). N: whatever you need most
    InvItem* sl = nullptr;
    if (!meds) { sl = invFind(IT_BANDAGE); if (!sl) sl = invFind(IT_RAG); if (!sl) { msg("No bandages or rags.", STEEL); return; } }
    else if (P.broken && !P.splinted && invFind(IT_SPLINT)) sl = invFind(IT_SPLINT);
    else if (P.broken && invFind(IT_MORPHINE)) sl = invFind(IT_MORPHINE);
    else if (P.sick > 0 && invFind(IT_ANTIBIO)) sl = invFind(IT_ANTIBIO);
    else if (invFind(IT_PAINKILL) && (P.pain > 0 || P.hp < P.perks.maxhp * .7f)) sl = invFind(IT_PAINKILL);
    else if (P.broken && !P.splinted) { msg(invCount(IT_KINDLING) >= 2 && (invCount(IT_BANDAGE) || invCount(IT_RAG)) ? "Make a splint in the inventory (Tab)." : "No meds for that leg. Splint = 2 kindling + bandage or rag.", STEEL); return; }
    if (!sl) { msg("Nothing useful to take.", STEEL); return; } useItem(sl);
}
static void useItem(InvItem* it) {
    if (!it || it->kind != 0) return; int id = it->id; const ItemDef& d = IDEF[id]; bool used = true;
    switch (id) {
    case IT_BANDAGE: case IT_RAG: if (P.bleeds > 0) { bool rag = id == IT_RAG; if (rag && chance(.35f)) { msg("The rag soaks through. Still bleeding.", BLOODC); break; } P.bleeds = std::max(0, P.bleeds - (P.perks.medic > 1 ? 2 : 1)); P.hp = std::min(P.perks.maxhp, P.hp + (rag ? 1 : 4) * P.perks.medic); msg(P.bleeds ? "Wrapped one wound." : "Bleeding stopped.", OKC); } else { used = false; msg("You're not bleeding.", STEEL); } break;
    case IT_ANTIBIO: if (P.sick > 0) { P.sick = 0; msg("The fever breaks.", OKC); } else { used = false; msg("You're not sick.", STEEL); } break;
    case IT_PAINKILL: P.pain = 0; P.hp = std::min(P.perks.maxhp, P.hp + 5); msg("Painkillers kick in.", OKC); break;
    case IT_MORPHINE: if (P.broken) { P.broken = false; P.splinted = false; msg("Morphine and a hard yank. Leg's set. You can run again.", OKC); } else { P.pain = 0; P.hp = std::min(P.perks.maxhp, P.hp + 10); } break;
    case IT_BATTERY: if (P.batt >= 100) { used = false; msg("Battery is already full.", STEEL); } else { P.batt = 100; msg("Fresh battery in the flashlight.", UVC); } break;
    case IT_FLARE: case IT_PIPEBOMB: used = false; throwItem(id == IT_FLARE); return;
    case IT_SPLINT: if (!P.broken) { used = false; msg("Nothing's broken. Yet.", STEEL); } else if (P.splinted) { used = false; msg("Already splinted. Give it time.", STEEL); } else { P.splinted = true; P.splintT = 0; msg("Leg splinted. It'll come back slowly.", OKC); sfx(SFX_PICK); } break;
    case IT_LOG: if (!canChop(P.melee)) { used = false; msg("You need an axe or hatchet in hand to split it.", STEEL); } else { it->n--; invPrune(); if (invAdd(IT_KINDLING, 4)) msg("No room for all the kindling.", STEEL); else msg("Split the log into kindling.", OKC); sfx(SFX_CHOP); return; } break;
    case IT_KINDLING: used = false; buildFire(); return;
    default:
        if (d.food != 0 || d.water != 0) { P.food = Clamp(P.food + d.food, 0, 110); P.water = Clamp(P.water + d.water, 0, 110); msg(std::string(d.use) + ": " + d.n, OKC); sfx(SFX_EAT); }
        else { used = false; if (d.use[0]) msg(std::string("Right-click the ") + d.n + " to pick what to use it on.", STEEL); }
    }
    if (used) { it->n--; invPrune(); }
}
static bool toolFits(int id, const Cloth& c) { // what a repair/upgrade item can do to a piece of clothing
    switch (id) {
    case IT_RAG: return c.cond < .6f && c.cond > 0; case IT_SEWKIT: return c.cond < .95f; case IT_TAPE: return c.cond < .75f;
    case IT_LEATHER: return c.upg < 3 && CD[c.def].slot != CS_EYES && CD[c.def].slot != CS_SOCKS; case IT_WAX: return c.bWater < .5f && CD[c.def].slot != CS_EYES;
    case IT_DYE_BLACK: case IT_DYE_GREEN: case IT_DYE_BROWN: return CD[c.def].slot != CS_EYES; } return false; }
static void applyTool(InvItem* tool, Cloth& c) {
    if (!tool || !toolFits(tool->id, c)) { msg("That won't help it.", STEEL); return; } std::string n = c.name;
    switch (tool->id) {
    case IT_RAG: c.cond = std::min(.6f, c.cond + .12f); msg("Patched the " + n + " with a rag.", OKC); break;
    case IT_SEWKIT: c.cond = std::min(.95f, std::max(.05f, c.cond) + .5f); msg("Stitched up the " + n + ".", OKC); break;
    case IT_TAPE: c.cond = std::min(.75f, std::max(.05f, c.cond) + .3f); msg("Duct-taped the " + n + ". Classy.", OKC); break;
    case IT_LEATHER: c.bArmor += .07f; c.upg++; c.cond = std::min(1.f, c.cond + .08f); msg("Sewed leather onto the " + n + ". Harder to bite through.", OKC); break;
    case IT_WAX: c.bWater = std::min(.5f, c.bWater + .3f); msg("Waxed the " + n + ". Rain beads right off.", OKC); break;
    default: { Color dc = tool->id == IT_DYE_BLACK ? rgba(34, 34, 38) : tool->id == IT_DYE_GREEN ? rgba(78, 92, 54) : rgba(96, 70, 44); c.col = lerpC(c.col, dc, .8f); c.bVis = std::min(c.bVis, tool->id == IT_DYE_BLACK ? .94f : .9f);
        if (c.pattern == 1) c.pattern = 0; msg("Dyed the " + n + ". Less of a target now.", OKC); } }
    tool->n--; invPrune(); sfx(SFX_PICK);
}
static void dropCloth(std::shared_ptr<Cloth> c, Vector2 at) { Loot l; l.kind = LK_CLOTH; l.id = c->def; l.count = 1; l.c = c; l.p = xz3(at, .05f); l.rot = frand(0, 6); loot.push_back(l); }
static void spawnCloth(Vector3 p, std::shared_ptr<Cloth> c) { Loot l; l.kind = LK_CLOTH; l.id = c->def; l.count = 1; l.c = c; l.p = p; l.rot = frand(0, 6); loot.push_back(l); }
static bool takeCloth(std::shared_ptr<Cloth> c) { // equip if the slot is free, otherwise stuff it in a pocket
    int s = CD[c->def].slot; if (!P.eq[s]) { P.eq[s] = c; msg("Put on " + c->name + " (" + CONDN[condTier(c->cond)] + ")", RARC[c->rar]); return true; }
    InvItem it; it.kind = 1; it.c = c; if (placeAnywhere(it)) { msg("Stowed " + c->name, RARC[c->rar]); return true; } return false; }
static void pickUp() {
    if (nearLoot < 0 || nearLoot >= (int)loot.size()) return; Loot l = loot[nearLoot];
    if (l.kind == LK_ITEM) { int left = invAdd(l.id, l.count); if (left == l.count) { msg("No room. Find clothes with more pockets.", rgba(255, 107, 91)); return; } msg("Picked up " + std::string(IDEF[l.id].n) + (l.count - left > 1 ? " x" + std::to_string(l.count - left) : ""), BONE);
        if (left > 0) loot[nearLoot].count = left; else loot.erase(loot.begin() + nearLoot); sfx(SFX_PICK); return; }
    if (l.kind == LK_CLOTH) { if (!takeCloth(l.c)) { msg("No room for that. Open the inventory (Tab) to swap.", rgba(255, 107, 91)); return; } loot.erase(loot.begin() + nearLoot); sfx(SFX_PICK); return; }
    loot.erase(loot.begin() + nearLoot);
    if (l.kind == LK_GUN) { int s = l.w.slot; if (P.hasGun[s]) { Loot o; o.kind = LK_GUN; o.id = P.gun[s].key; o.w = P.gun[s]; o.p = xz3(P.p, .05f); o.rot = frand(0, 6); loot.push_back(o); }
        P.gun[s] = l.w; P.hasGun[s] = true;
        if (cfg.autoEquip || P.active == s) { P.active = s; P.reload = 0; msg("Equipped " + l.w.name, RARC[l.w.rar]); } else { msg("Picked up " + l.w.name + " (slot " + std::to_string(s + 1) + ")", RARC[l.w.rar]); }
        if (l.w.rar == 3) sfx(SFX_LEGEND, 1, 0); else sfx(SFX_PICK); }
    else { if (P.melee != MK_FISTS) { Loot o; o.kind = LK_MELEE; o.id = P.melee; o.p = xz3(P.p, .05f); o.rot = frand(0, 6); loot.push_back(o); } P.melee = l.id; msg("Equipped " + std::string(MB[l.id].n), BONE); sfx(SFX_PICK); }
}
static int outfitLook() {
    int pl = playerLooks[playerFemale && playerLooks[1] >= 0 ? 1 : 0]; if (pl < 0) return pl; Look& L = looks[pl]; if (L.col0.empty()) return pl;
    auto wear = [](int s) -> Color { auto& c = P.eq[s]; Color k = c->col; if (c->pattern == 2) k = mulC(k, .85f); float dirt = (1 - c->cond) * .45f; return lerpC(k, rgba(84, 72, 58), dirt); };
    Color skin = rgba(206, 160, 130); for (int i = 0; i < (int)L.col0.size(); i++) if (hasStr(assets[L.asset].matName[i], "Skin")) { skin = L.col0[i]; break; }
    Color top = P.eq[CS_OUTER] ? wear(CS_OUTER) : P.eq[CS_TORSO] ? wear(CS_TORSO) : skin, legs = P.eq[CS_LEGS] ? wear(CS_LEGS) : mulC(skin, .95f), feet = P.eq[CS_FEET] ? wear(CS_FEET) : P.eq[CS_SOCKS] ? wear(CS_SOCKS) : skin;
    for (int i = 0; i < (int)L.col.size(); i++) { int z = L.zone[i]; Color b = L.col0[i]; float lum = (b.r + b.g + b.b) / 3.f / 255.f, sh = .75f + .5f * std::min(1.f, lum * 1.6f); // keep the model's light/dark detail
        L.col[i] = z == 1 ? mulC(top, std::min(1.15f, sh)) : z == 2 ? mulC(legs, std::min(1.15f, sh)) : z == 3 ? mulC(feet, std::min(1.1f, sh)) : b; }
    return pl;
}

