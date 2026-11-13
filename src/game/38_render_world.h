// Nightfall County: render world.
// Not a standalone header: included once, in order, from src/main.cpp. Everything above it in that list is visible here.
#pragma once
static void renderWorld3D() {
    float vr = 48 / camZoom;
    Vector2 c = state == ST_TITLE ? camPos : camPos;
    auto vis = [&](float x, float z, float m) {
        return fabsf(x - c.x) < vr + m && fabsf(z - c.y + 6) < vr * .8f + m;
    };
    float dk = darkK();
    int uDet1 = 1, uDet0 = 0;
    BeginShaderMode(lit);
    SetShaderValue(lit, ULOC[0][uDetail], &uDet1, SHADER_UNIFORM_INT);
    groundMat.maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
    if (groundAtlasOk)
        setTexMode(5, 1.f / 5.f);
    DrawMesh(groundMesh, groundMat, MatrixIdentity());
    setTexMode(0);
    SetShaderValue(lit, ULOC[0][uDetail], &uDet0, SHADER_UNIFORM_INT);
    // roads
    for (auto& r : roads) {
        if (r.tier > 0)
            continue; // gravel and dirt are painted into the ground
        Vector2 a = r.a, b = r.b;
        float len = Vector2Distance(a, b), yaw = atan2f(b.y - a.y, b.x - a.x);
        // cull: skip if far from view
        if (segDist(c, a, b) > vr + 20)
            continue;
        float segL = 30;
        int ns = (int)(len / segL) + 1;
        for (int i = 0; i < ns; i++) {
            float s0 = i * len / ns, s1 = (i + 1) * len / ns;
            Vector2 m = Vector2Lerp(a, b, (s0 + s1) / 2 / len);
            if (!vis(m.x, m.y, 20))
                continue;
            drawTexMesh(M_CUBE, TRS(V3(m.x, .02f, m.y), V3(s1 - s0 + .02f, .04f, r.w), yaw),
                        r.town ? rgba(128, 128, 128) : rgba(112, 112, 116),
                        rainLevel > .35f ? TX_ASPHALT_WET : TX_ASPHALT);
            setTexMode(0);
            if (!r.town)
                for (float q = s0; q < s1; q += 6) {
                    Vector2 dp = Vector2Lerp(a, b, (q + 1.5f) / len);
                    drawBox(V3(dp.x, .045f, dp.y), V3(2.5f, .01f, .14f), yaw, rgba(200, 170, 80));
                }
            if (r.tier ==
                0) { // ragged edge: broken patches of asphalt and gravel hugging both sides (fixed per position, so no shimmer)
                Vector2 nrm = V2(-sinf(yaw), cosf(yaw));
                for (float q = s0; q < s1; q += 1.6f)
                    for (int sd = -1; sd <= 1; sd += 2) {
                        float x1 = sinf(q * 12.9898f + sd * 78.233f) * 43758.5453f,
                              x2 = sinf(q * 39.346f + sd * 11.135f) * 24634.634f;
                        float h1 = x1 - floorf(x1), h2 = x2 - floorf(x2);
                        if (h1 < .45f)
                            continue;
                        float off = r.w / 2 + .2f + h2 * 2.6f;
                        Vector2 p = Vector2Add(Vector2Lerp(a, b, (q + .4f) / len), Vector2Scale(nrm, off * sd));
                        if (!vis(p.x, p.y, 4))
                            continue;
                        drawTexMesh(M_CUBE, TRS(V3(p.x, .03f, p.y), V3(.7f + h2, .03f, .5f + h1 * .6f), yaw + h1 * 2),
                                    rgba(94 + (int)(h2 * 30), 92 + (int)(h2 * 26), 88 + (int)(h2 * 20)), TX_ASPHALT);
                        setTexMode(0);
                    }
            } else {
                Vector2 nrm = V2(-sinf(yaw), cosf(yaw));
                for (int sd = -1; sd <= 1; sd += 2)
                    drawTexMesh(M_CUBE,
                                TRS(V3(m.x + nrm.x * (r.w / 2 + .9f) * sd, .08f, m.y + nrm.y * (r.w / 2 + .9f) * sd),
                                    V3(s1 - s0, .16f, 1.8f), yaw),
                                rgba(150, 150, 146), TX_CONCRETE);
                setTexMode(0);
            }
        }
    }
    // static chunks
    for (int cx = gcl(c.x - vr - 20); cx <= gcl(c.x + vr + 20); cx++)
        for (int cz = gcl(c.y - vr - 20); cz <= gcl(c.y + vr + 10); cz++)
            if (chunkHas[cx][cz]) {
                litMat.maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
                DrawMesh(chunkMesh[cx][cz], litMat, MatrixIdentity());
            }
    // props (opaque)
    std::vector<int> transp;
    static std::vector<int> loftT;
    loftT.clear();
    for (int cx = gcl(c.x - vr - 20); cx <= gcl(c.x + vr + 20); cx++)
        for (int cz = gcl(c.y - vr - 20); cz <= gcl(c.y + vr + 10); cz++)
            for (int pi : pgrid[cx][cz]) {
                const Prop& p = props[pi];
                if (p.layer == L_ALWAYS || !vis(p.p.x, p.p.z, 10))
                    continue;
                float fade = 1, cut = 1;
                if (p.bld >= 0) {
                    fade = blds[p.bld].fade;
                    cut = blds[p.bld].cut;
                }
                if (p.layer == L_ROOF && fade < .98f) {
                    if (fade > .03f)
                        transp.push_back(pi);
                    continue;
                }
                if (p.layer == L_LOFT && P.loft < 0 && buildingAt(P.p) == p.bld) {
                    loftT.push_back(pi);
                    continue;
                }
                if (p.layer == L_WALLDET && cut < .6f)
                    continue;
                if (p.layer == L_WALL && cut < .98f) {
                    float h = Lerp(1.0f, p.s.y, cut);
                    drawTexMesh(M_CUBE, TRS(V3(p.p.x, h / 2, p.p.z), V3(p.s.x, h, p.s.z), p.yaw), p.c, p.tex);
                    continue;
                }
                drawTexMesh(p.mesh, TRS(p.p, p.s, p.yaw), p.c, p.tex);
            }
    setTexMode(0);
    // doors
    for (auto& d : doorObjs) {
        if (!vis(d.c.x, d.c.y, 4))
            continue;
        float cut = blds[d.bld].cut, hgt = Lerp(1.0f, 2.45f, cut);
        bool barn = d.w > 3;
        int tx = d.metal ? TX_METAL : barn ? TX_BARNWOOD : TX_HARDWOOD;
        Color dc = d.metal ? rgba(150, 156, 160) : barn ? rgba(150, 70, 50) : rgba(150, 120, 90);
        dc = lerpC(dc, rgba(60, 54, 46),
                   (1 - d.cond) * .5f + (d.hpMax > 0 ? (1 - d.hp / d.hpMax) * .4f : 0)); // weathered, then beaten
        d.anim += ((d.open || d.broken ? 1.f : 0.f) - d.anim) * std::min(1.f, GetFrameTime() * 10);
        d.bang -= GetFrameTime();
        if (d.broken) {
            drawTexMesh(M_CUBE,
                        TRS(V3(d.c.x + (d.horiz ? 0 : .9f), .06f, d.c.y + (d.horiz ? .9f : 0)),
                            d.horiz ? V3(d.w * .9f, .08f, 2.2f) : V3(2.2f, .08f, d.w * .9f), .3f),
                        mulC(dc, .7f), tx);
            continue;
        }
        float sw = d.anim * PI / 2 * .95f, jig = d.bang > 0 ? sinf(gTime * 90) * .03f : 0;
        Vector2 hinge = d.horiz ? V2(d.c.x - d.w / 2, d.c.y) : V2(d.c.x, d.c.y - d.w / 2);
        Vector2 dir = d.horiz ? polar(sw, 1) : polar(PI / 2 - sw, 1); // swings inward-ish
        Vector2 mid = Vector2Add(hinge, Vector2Scale(dir, d.w / 2));
        float yaw = atan2f(dir.y, dir.x) + jig;
        drawTexMesh(M_CUBE, TRS(V3(mid.x, hgt / 2 + .03f, mid.y), V3(d.w, hgt, .08f), yaw), dc, tx);
    }
    setTexMode(0);
    // campfires: stones and logs
    for (auto& f : fires) {
        if (!vis(f.p.x, f.p.y, 2))
            continue;
        for (int k = 0; k < 8; k++) {
            Vector2 sp = Vector2Add(f.p, polar(k * PI / 4, .55f));
            drawMeshM(M_SPHERE, TRS(xz3(sp, .07f), V3(.22f, .16f, .2f), k), rgba(110, 108, 100));
        }
        for (int k = 0; k < 3; k++)
            drawMeshM(M_CUBE, TRS(xz3(f.p, .12f + k * .04f), V3(.8f, .1f, .1f), k * 1.05f), rgba(60, 40, 26));
    }
    // trees
    std::vector<int> tvis;
    for (int cx = gcl(c.x - vr - 10); cx <= gcl(c.x + vr + 10); cx++)
        for (int cz = gcl(c.y - vr - 10); cz <= gcl(c.y + vr + 10); cz++)
            for (int ti : tgrid[cx][cz]) {
                const Tree& t = trees[ti];
                if (!vis(t.p.x, t.p.y, 8))
                    continue;
                tvis.push_back(ti);
                if (t.asset >= 0) {
                    if (t.fade > .97f)
                        drawAsset(t.asset, placeM(t.asset, V3(t.p.x, -.05f, t.p.y), t.sc, t.yaw));
                    continue;
                }
                drawMeshM(M_CYL, TRS(V3(t.p.x, 0, t.p.y), V3(.45f, t.h * .5f, .45f), 0), rgba(70, 52, 38));
                if (t.fade > .97f) {
                    if (t.kind == 0) {
                        drawMeshM(M_CONE, TRS(V3(t.p.x, t.h * .25f, t.p.y), V3(t.r * 2, t.h * .55f, t.r * 2), t.p.x),
                                  t.c);
                        drawMeshM(M_CONE,
                                  TRS(V3(t.p.x, t.h * .55f, t.p.y), V3(t.r * 1.5f, t.h * .45f, t.r * 1.5f), t.p.y),
                                  mulC(t.c, 1.12f));
                    } else {
                        drawMeshM(M_SPHERE, TRS(V3(t.p.x, t.h * .62f, t.p.y), V3(t.r * 2, t.r * 1.6f, t.r * 2), 0),
                                  t.c);
                        drawMeshM(M_SPHERE,
                                  TRS(V3(t.p.x + t.r * .4f, t.h * .78f, t.p.y - t.r * .3f),
                                      V3(t.r * 1.2f, t.r, t.r * 1.2f), 0),
                                  mulC(t.c, 1.15f));
                    }
                }
            }
    // loot
    for (auto& l : loot) {
        if (!vis(l.p.x, l.p.z, 2) || lootHidden(l))
            continue;
        {
            int ai = -1;
            float sc = 1;
            Vector3 at = l.p;
            Matrix M;
            if (l.kind == LK_GUN) {
                ai = gunAsset(l.id);
                if (ai >= 0) {
                    sc = gunScale(l.id, ai);
                    M = MatrixMultiply(MatrixMultiply(MatrixMultiply(MatrixScale(sc, sc, sc), MatrixRotateX(PI / 2)),
                                                      MatrixRotateY(-l.rot)),
                                       MatrixTranslate(at.x, at.y + .05f, at.z));
                    drawAsset(ai, M, 1,
                              l.w.key == W_AR15 || l.w.key == W_M4 || l.w.key == W_MP5 ? rgba(110, 110, 115) : WHITE);
                    continue;
                }
            } else if (l.kind == LK_MELEE) {
                ai = meleeAsset(l.id);
                if (ai >= 0) {
                    sc = meleeScale(l.id, ai);
                    M = MatrixMultiply(MatrixMultiply(MatrixMultiply(MatrixScale(sc, sc, sc), MatrixRotateX(PI / 2)),
                                                      MatrixRotateY(-l.rot)),
                                       MatrixTranslate(at.x, at.y + .03f, at.z));
                    drawAsset(ai, M);
                    continue;
                }
            } else if (l.kind == LK_ITEM) {
                ai = itemAsset(l.id);
                if (ai >= 0) {
                    sc = itemScale(l.id, ai);
                    drawAsset(ai, placeM(ai, at, sc, l.rot));
                    continue;
                }
            }
        }
        if (l.kind == LK_GUN) {
            drawBox(V3(l.p.x, l.p.y + .05f, l.p.z), V3(WB[l.id].len + .3f, .08f, .1f), l.rot, rgba(40, 42, 46));
            drawBox(V3(l.p.x, l.p.y + .05f, l.p.z), V3(.15f, .1f, .12f), l.rot, RARC[l.w.rar]);
        } else if (l.kind == LK_MELEE)
            drawBox(V3(l.p.x, l.p.y + .03f, l.p.z), V3(MB[l.id].len, .05f, .06f), l.rot, MB[l.id].c);
        else if (l.kind == LK_CLOTH) {
            int sl = CD[l.c->def].slot;
            Vector3 sz = sl == CS_BACK   ? V3(.45f, .25f, .32f)
                         : sl == CS_FEET ? V3(.3f, .12f, .22f)
                         : sl == CS_HEAD || sl == CS_EYES || sl == CS_FACE || sl == CS_SOCKS || sl == CS_HANDS
                             ? V3(.2f, .06f, .16f)
                             : V3(.42f, .07f, .34f);
            drawBox(V3(l.p.x, l.p.y + sz.y / 2, l.p.z), sz, l.rot, l.c->col);
            if (l.c->pattern == 1)
                drawBox(V3(l.p.x, l.p.y + sz.y + .005f, l.p.z), V3(sz.x * .9f, .01f, .04f), l.rot, mulC(l.c->col, .6f));
        } else {
            Color ic = IDEF[l.id].c;
            if (isAmmo(l.id))
                drawBox(V3(l.p.x, l.p.y + .06f, l.p.z), V3(.22f, .12f, .14f), l.rot, rgba(80, 74, 52));
            else if (l.id == IT_LOG)
                drawMeshM(
                    M_CYL,
                    MatrixMultiply(MatrixMultiply(MatrixScale(.28f, .9f, .28f), MatrixRotateZ(PI / 2)),
                                   MatrixMultiply(MatrixRotateY(-l.rot), MatrixTranslate(l.p.x, l.p.y + .14f, l.p.z))),
                    ic);
            else if (l.id == IT_KINDLING || l.id == IT_SPLINT) {
                for (int k = 0; k < 3; k++)
                    drawBox(V3(l.p.x, l.p.y + .04f + k * .05f, l.p.z), V3(.5f, .04f, .07f), l.rot + k * .5f, ic);
            } else if (l.id == IT_MATCHES)
                drawBox(V3(l.p.x, l.p.y + .03f, l.p.z), V3(.12f, .04f, .07f), l.rot, ic);
            else
                drawMeshM(M_CYL, TRS(l.p, V3(.12f, .18f, .12f), 0), ic);
        }
    }
    // corpses
    {
        int drawn = 0;
        for (int ci = (int)corpses.size() - 1; ci >= 0; ci--) {
            const Corpse& cp = corpses[ci];
            if (cp.look < 0 || !haveChars || !vis(cp.p.x, cp.p.y, 3))
                continue;
            if (++drawn > 70)
                break;
            PoseIn pin;
            pin.kind = PK_CORPSE;
            pin.seed = cp.seed;
            pin.armL = cp.armL;
            pin.armR = cp.armR;
            pin.head = cp.head;
            drawCharacter(cp.look, pin, cp.p, cp.yaw + PI, rgba(40, 30, 26), .3f, true);
        }
    }
    for (auto& cp : corpses) {
        if (cp.look >= 0 && haveChars)
            continue;
        if (!vis(cp.p.x, cp.p.y, 3))
            continue;
        const ZDef& t = ZT[cp.type];
        drawHuman(cp.p, cp.yaw + PI, 0, t.skin, t.shirt, t.pants, t.h, cp.type == Z_BRUTE ? 1.5f : 1, cp.armL, cp.armR,
                  cp.head, 3, 1, WHITE, cp.type == Z_SOLDIER ? 2 : 0, -1);
    }
    for (auto& g : gibs)
        if (vis(g.p.x, g.p.z, 2))
            drawGibM(g);
    // zombies
    for (auto& z : zombies) {
        if (!vis(z.p.x, z.p.y, 3))
            continue;
        const ZDef& t = ZT[z.type];
        if (z.look >= 0 && haveChars) {
            PoseIn pin;
            bool fast = (z.type == Z_RUNNER || z.type == Z_VOLATILE) && (z.sees || z.aggro > 0 || z.noiseT > 0);
            pin.kind = z.type == Z_BRUTE ? PK_BRUTE : fast ? PK_RUNNER : PK_ZOMBIE;
            pin.phase = z.walk * (fast ? 2.4f : 3.6f);
            pin.move = fast ? 1.5f : 1;
            pin.seed = z.seed;
            pin.armL = z.armL;
            pin.armR = z.armR;
            pin.stagger = z.stun > 0 ? 1 : 0;
            if (z.atk < .3f)
                pin.attack = 1 - z.atk / .3f;
            Color tc = z.flash > 0 ? WHITE : z.burn > 0 ? rgba(255, 120, 40) : z.job ? JOBC[z.job] : WHITE;
            float ta = z.flash > 0 ? .7f : z.burn > 0 ? .35f : z.job ? .5f : 0;
            CharOut co;
            drawCharacter(z.look, pin, z.p, z.a, tc, ta, false, &co);
            if (!z.armL || !z.armR)
                for (int sd = 0; sd < 2; sd++)
                    if (!(sd ? z.armR : z.armL)) {
                        Vector3 sp = Vector3Lerp(co.chest, co.hand[sd], .18f);
                        drawMeshM(M_SPHERE, TRS(sp, V3(.11f, .11f, .11f), 0), rgba(130, 12, 18));
                    }
            continue;
        }
        Color tint = z.flash > 0 ? rgba(255, 255, 255) : z.burn > 0 ? rgba(255, 150, 110) : WHITE;
        Color sk = z.flash > 0 ? rgba(255, 230, 230) : t.skin;
        drawHuman(z.p, z.a, z.walk, sk, t.shirt, t.pants, t.h,
                  z.type == Z_BRUTE     ? 1.5f
                  : z.type == Z_BLOATER ? 1.4f
                                        : 1,
                  z.armL, z.armR, true, 0, 0, tint, z.type == Z_SOLDIER ? 2 : 0, -1);
        if (z.type == Z_BLOATER)
            for (int k = 0; k < 4; k++)
                drawMeshM(
                    M_SPHERE,
                    TRS(xz3(Vector2Add(z.p, polar(z.a + k * 1.5f, .3f)), 1.1f + k * .1f), V3(.22f, .22f, .22f), 0),
                    rgba(190, 220, 110));
    }
    // player
    if (state != ST_DEAD && state != ST_TITLE) {
        Weapon* g = curGun();
        int pose = g ? 1 : 2;
        float swingT = P.swing > 0 ? 1 - P.swing / .22f : -1;
        Color tint = P.hurt > 0 ? rgba(255, 140, 140) : WHITE;
        if (haveChars && playerLooks[0] >= 0) {
            PoseIn pin;
            pin.kind = PK_PLAYER;
            pin.phase = P.step * 1.85f;
            float spd = Vector2Length(P.k) > 3 ? 0 : 1;
            (void)spd;
            bool moving = P.moving; // real movement, so click-to-move and autorun animate the legs too
            pin.move = !moving ? 0 : P.sprint ? 1.9f : P.crouch ? .6f : 1;
            pin.crouch = P.crouch ? 1 : 0;
            pin.hold = g ? (g->slot == 1 ? 1 : 2) : 0;
            pin.swing = P.swing > 0 ? swingT : -1;
            pin.roll = P.roll > 0 ? 1 - P.roll / .28f : -1;
            pin.recoil = g ? Clamp(P.cd / std::max(.05f, g->rate), 0, 1) : 0;
            if (P.sprint && g)
                pin.hold = 0;
            CharOut co;
            int pl = outfitLook();
            float yaw = P.a;
            if (P.roll > 0)
                yaw = atan2f(P.rv.y, P.rv.x);
            else if (pin.move > 0) {
                P.animA +=
                    angDiff(P.moveA, P.animA) *
                    std::min(
                        1.f,
                        GetFrameTime() *
                            14); // legs follow movement (smoothed), upper body tracks the aim; walk backwards when backpedaling
                float df = angDiff(P.a, P.animA);
                if (!P.back) {
                    yaw = P.animA;
                    pin.twist = df;
                } else {
                    yaw = P.animA + PI;
                    pin.twist = angDiff(P.a, yaw);
                    pin.phase = -pin.phase;
                }
            } else
                P.animA = P.a;
            if (P.vaultT > 0) {
                float t = 1 - P.vaultT / .45f;
                yaw = atan2f(P.vaultB.y - P.vaultA.y, P.vaultB.x - P.vaultA.x);
                pin.crouch = .7f;
                pin.move = 0;
                pin.twist = 0;
                charY = sinf(t * PI) * (P.vaultH * .8f + .2f);
            }
            if (P.loft >= 0)
                charY = lofts[P.loft].y;
            if (P.kickT > 0)
                pin.kick = 1 - P.kickT / .35f;
            pin.guard = P.blocking;
            drawCharacter(pl, pin, P.p, yaw, rgba(255, 60, 60), P.hurt > 0 ? .35f : 0, false, &co);
            if (P.shieldHP > 0 && P.roll <= 0) {
                Vector3 hp = Vector3Add(co.hand[0], xz3(polar(P.a, P.blocking ? .2f : .05f), 0));
                if (!P.blocking)
                    hp = Vector3Add(hp, V3(0, -.15f, 0));
                drawTexMesh(M_CUBE, TRS(hp, V3(.62f, .82f, .05f), P.a + PI / 2 + (P.blocking ? 0 : .5f)),
                            rgba(170, 150, 120), TX_PLYWOOD);
                setTexMode(0);
            }
            if (P.roll <= 0) {
                if (g && !P.sprint) {
                    int ga = gunAsset(g->key);
                    if (ga >= 0) {
                        float sc = gunScale(g->key, ga);
                        Vector3 hp = co.hand[1];
                        if (pin.hold == 2)
                            hp = Vector3Lerp(co.hand[1], co.hand[0], .2f);
                        Matrix M = MatrixMultiply(MatrixMultiply(MatrixScale(sc, sc, sc), MatrixRotateY(-P.a)),
                                                  MatrixTranslate(hp.x, hp.y, hp.z));
                        drawAsset(ga, M, 1,
                                  g->key == W_AR15 || g->key == W_M4 || g->key == W_MP5 ? rgba(110, 110, 115) : WHITE);
                    }
                } else if (P.melee != MK_FISTS) {
                    int ma = meleeAsset(P.melee);
                    Vector3 hp = co.hand[1];
                    float ang = P.swing > 0 ? P.swingA - 1.2f + 2.4f * swingT : P.a + .9f;
                    if (ma >= 0) {
                        float sc = meleeScale(P.melee, ma);
                        Matrix M =
                            MatrixMultiply(MatrixMultiply(MatrixMultiply(MatrixScale(sc, sc, sc), meleeAlign(ma)),
                                                          MatrixRotateY(-ang)),
                                           MatrixTranslate(hp.x, hp.y, hp.z));
                        drawAsset(ma, M);
                    } else {
                        Vector3 tip = Vector3Add(hp, xz3(polar(ang, MB[P.melee].len), 0));
                        drawBox(Vector3Lerp(hp, tip, .5f), V3(MB[P.melee].len, .05f, .06f), ang, MB[P.melee].c);
                    }
                }
            }
            charY = 0;
            goto player_done;
        }
        drawHuman(P.p, P.a, P.step * .55f, rgba(200, 160, 126), rgba(64, 86, 96), rgba(52, 58, 76),
                  P.crouch ? 1.4f : 1.8f, 1, true, true, true, pose, 0, tint, 1, swingT);
        float sh = P.crouch ? 1.05f : 1.32f;
        if (g) {
            Vector2 gp = Vector2Add(P.p, polar(P.a, .45f + g->len / 2));
            drawBox(xz3(gp, sh), V3(g->len, .09f, .08f), P.a, rgba(34, 36, 40));
            drawBox(xz3(Vector2Add(P.p, polar(P.a, .5f)), sh - .08f), V3(.12f, .14f, .07f), P.a, RARC[g->rar]);
        } else if (P.melee != MK_FISTS) {
            float sa = P.swing > 0 ? P.swingA - 1.2f + 2.4f * swingT : P.a + .5f;
            Vector2 mp = Vector2Add(P.p, polar(sa, .5f + MB[P.melee].len / 2));
            drawBox(xz3(mp, sh), V3(MB[P.melee].len, .05f, .06f), sa, MB[P.melee].c);
        }
    }
player_done:;
    // ---- batch pass: decals, shadows, particles
    rlDrawRenderBatchActive();
    Matrix I = MatrixIdentity();
    SetShaderValueMatrix(lit, lit.locs[SHADER_LOC_MATRIX_MODEL], I);
    SetShaderValueMatrix(lit, lit.locs[SHADER_LOC_MATRIX_NORMAL], I);
    litMat.maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
    Vector4 wc = {1, 1, 1, 1};
    SetShaderValue(lit, lit.locs[SHADER_LOC_COLOR_DIFFUSE], &wc, SHADER_UNIFORM_VEC4);
    rlDisableDepthMask();
    rlDisableBackfaceCulling();
    // sun shadows
    float sunA = (hourOf() - 6) / 12 * PI;
    float elev = std::max(.25f, sinf(sunA));
    Vector2 sv = Vector2Scale(V2(-cosf(sunA) * .8f, .45f), 1.f / elev * .55f);
    float shA = (1 - dk) * (1 - rainLevel * .6f) * .42f;
    if (shA > .02f && !shadowsActive) {
        rlSetTexture(rlGetTextureIdDefault());
        rlBegin(RL_TRIANGLES);
        rlColor4ub(0, 0, 0, (unsigned char)(shA * 255));
        rlNormal3f(0, 1, 0);
        for (auto& b : blds) {
            if (!vis((b.x0 + b.x1) / 2, (b.z0 + b.z1) / 2, 30))
                continue;
            float hh = b.h + (b.type == B_HOUSE || b.type == B_BARN ? 1.8f : 0);
            Vector2 s = Vector2Scale(sv, hh);
            std::vector<Vector2> pts = {V2(b.x0, b.z0),
                                        V2(b.x1, b.z0),
                                        V2(b.x1, b.z1),
                                        V2(b.x0, b.z1),
                                        V2(b.x0 + s.x, b.z0 + s.y),
                                        V2(b.x1 + s.x, b.z0 + s.y),
                                        V2(b.x1 + s.x, b.z1 + s.y),
                                        V2(b.x0 + s.x, b.z1 + s.y)};
            hull(pts);
            for (size_t i = 1; i + 1 < pts.size(); i++) {
                rlVertex3f(pts[0].x, .046f, pts[0].y);
                rlVertex3f(pts[i + 1].x, .046f, pts[i + 1].y);
                rlVertex3f(pts[i].x, .046f, pts[i].y);
            }
        }
        rlEnd();
        rlSetTexture(softTex.id);
        rlBegin(RL_QUADS);
        for (int ti : tvis) {
            const Tree& t = trees[ti];
            Vector2 s = Vector2Scale(sv, t.h * .55f);
            groundQuad(Vector2Add(t.p, s), .04f, t.r * 1.3f, 0, rgba(0, 0, 0, (int)(shA * 330)), {0, 0, 1, 1});
        }
        rlEnd();
    }
    rlSetTexture(softTex.id);
    rlBegin(RL_QUADS);
    for (auto& z : zombies)
        if (vis(z.p.x, z.p.y, 2))
            groundQuad(z.p, .05f, z.r * 1.5f, 0, rgba(0, 0, 0, shadowsActive ? 70 : 120), {0, 0, 1, 1});
    if (state == ST_PLAY || state == ST_PAUSE || state == ST_INV || state == ST_MAP || state == ST_SKILLS)
        groundQuad(P.p, .05f, .6f, 0, rgba(0, 0, 0, 130), {0, 0, 1, 1});
    for (auto& a : acid) {
        float f = std::min(1.f, a.t / 1.5f);
        groundQuad(a.p, .06f, a.r * f * 1.2f, 0, rgba(140, 200, 50, (int)(170 * f)), {0, 0, 1, 1});
    }
    rlEnd();
    // blood decals
    rlSetTexture(bloodTex.id);
    rlBegin(RL_QUADS);
    for (auto& d : decals) {
        if (!vis(d.p.x, d.p.y, 2))
            continue;
        groundQuad(d.p, d.y, d.r, d.rot, d.c, {d.v * .25f, 0, .25f, 1});
    }
    rlEnd();
    // particles (lit)
    rlSetTexture(softTex.id);
    rlBegin(RL_QUADS);
    for (auto& p : parts) {
        if (!vis(p.p.x, p.p.z, 2))
            continue;
        float a = p.t / p.mt;
        switch (p.kind) {
        case P_BLOOD:
        case P_ACID:
            billboard(p.p, p.s, p.c);
            break;
        case P_SMOKE:
            billboard(p.p, p.s * (2 - a), withA(p.c, a * (p.c.a / 255.f) * .6f));
            break;
        case P_MIST:
            billboard(p.p, p.s * (1.8f - a * .8f), withA(p.c, a * (p.c.a / 255.f) * .6f));
            break;
        case P_DUST:
            billboard(p.p, p.s * (2 - a), withA(p.c, a * .5f));
            break;
        case P_CASING:
            billboard(p.p, p.s, p.c);
            break;
        default:
            break;
        }
    }
    rlEnd();
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
    rlEnableBackfaceCulling();
    // ---- transparent: faded roofs & canopies
    for (int pi : transp) {
        const Prop& p = props[pi];
        Color cc = withA(p.c, blds[p.bld].fade * blds[p.bld].fade);
        drawTexMesh(p.mesh, TRS(p.p, p.s, p.yaw), cc, p.tex);
    }
    for (int pi : loftT) {
        const Prop& p = props[pi];
        drawTexMesh(p.mesh, TRS(p.p, p.s, p.yaw), withA(p.c, .22f), p.tex);
    }
    setTexMode(0);
    for (int ti : tvis) {
        const Tree& t = trees[ti];
        if (t.fade > .97f)
            continue;
        if (t.asset >= 0) {
            drawAsset(t.asset, placeM(t.asset, V3(t.p.x, -.05f, t.p.y), t.sc, t.yaw), t.fade);
            continue;
        }
        Color cc = withA(t.c, t.fade);
        if (t.kind == 0) {
            drawMeshM(M_CONE, TRS(V3(t.p.x, t.h * .25f, t.p.y), V3(t.r * 2, t.h * .55f, t.r * 2), t.p.x), cc);
            drawMeshM(M_CONE, TRS(V3(t.p.x, t.h * .55f, t.p.y), V3(t.r * 1.5f, t.h * .45f, t.r * 1.5f), t.p.y),
                      withA(mulC(t.c, 1.12f), t.fade));
        } else
            drawMeshM(M_SPHERE, TRS(V3(t.p.x, t.h * .62f, t.p.y), V3(t.r * 2, t.r * 1.6f, t.r * 2), 0), cc);
    }
    EndShaderMode();
    // ---- emissive (default shader, additive)
    BeginBlendMode(BLEND_ADDITIVE);
    rlDisableDepthMask();
    rlSetTexture(softTex.id);
    rlBegin(RL_QUADS);
    for (auto& p : parts) {
        if (!vis(p.p.x, p.p.z, 2))
            continue;
        float a = p.t / p.mt;
        if (p.kind == P_FIRE)
            billboard(p.p, p.s * (.6f + a), withA(p.c, a));
        else if (p.kind == P_SPARK)
            billboard(p.p, p.s * 2, withA(p.c, a));
    }
    for (auto& l : lights)
        if (l.r > 10 && l.r < 18 && l.t / l.mt > .4f)
            billboard(l.p, .55f, rgba(255, 220, 160, 230));
    for (auto& b : bombs) {
        if (b.flare) {
            billboard(b.p, .9f + sinf(gTime * 40) * .1f, rgba(255, 70, 40, 255));
            billboard(b.p, .3f, rgba(255, 220, 200, 255));
        } else if (b.blink > 0)
            billboard(b.p, .3f, rgba(255, 50, 30, 255));
    }
    for (auto& l : staticLights)
        if (Vector2Distance(V2(l.p.x, l.p.z), c) < vr + 10)
            billboard(l.p, .5f + .1f * sinf(gTime * 20 + l.p.x), rgba(255, 140, 50, (int)(150 + 100 * dk)));
    for (auto& f : fires)
        if (vis(f.p.x, f.p.y, 2)) {
            billboard(xz3(f.p, .45f), .6f + .1f * sinf(gTime * 23 + f.p.x), rgba(255, 150, 60, 230));
            billboard(xz3(f.p, .5f), 1.6f, rgba(255, 110, 40, (int)(50 + 60 * dk)));
        }
    for (auto& z : zombies) {
        if (!vis(z.p.x, z.p.y, 2))
            continue;
        bool vol = z.type == Z_VOLATILE;
        if (!vol && dk < .5f)
            continue;
        Vector2 hp = Vector2Add(z.p, polar(z.a, .14f + z.r * .3f));
        Vector2 rt = polar(z.a + PI / 2, .07f);
        float hy = ZT[z.type].h * .9f;
        Color e = vol ? rgba(255, 70, 40, 255) : rgba(200, 40, 30, (int)(160 * dk));
        billboard(xz3(Vector2Add(hp, rt), hy), vol ? .09f : .05f, e);
        billboard(xz3(Vector2Subtract(hp, rt), hy), vol ? .09f : .05f, e);
        if (vol)
            billboard(xz3(hp, hy), .4f, rgba(255, 60, 30, 70));
    }
    for (auto& l : loot)
        if (l.kind == LK_GUN && l.w.rar >= 2 && vis(l.p.x, l.p.z, 2) && !lootHidden(l)) {
            Color rc = RARC[l.w.rar];
            for (int k = 0; k < 6; k++)
                billboard(Vector3Add(l.p, V3(0, .3f + k * .45f, 0)), .28f - k * .03f, withA(rc, .35f - k * .05f));
        }
    rlEnd();
    // tracers
    rlSetTexture(rlGetTextureIdDefault());
    rlBegin(RL_QUADS);
    for (auto& b : bullets) {
        Vector2 d = Vector2Normalize(b.v);
        Vector3 a = xz3(Vector2Subtract(b.p, Vector2Scale(d, 2.2f)), 1.3f), e = xz3(b.p, 1.3f);
        Vector3 side = V3(-d.y * .025f, 0, d.x * .025f);
        Color tc = b.burn ? rgba(255, 140, 60, 230) : rgba(255, 225, 160, 200);
        rlColor4ub(tc.r, tc.g, tc.b, 0);
        rlVertex3f(a.x - side.x, a.y, a.z - side.z);
        rlVertex3f(a.x + side.x, a.y, a.z + side.z);
        rlColor4ub(tc.r, tc.g, tc.b, tc.a);
        rlVertex3f(e.x + side.x, e.y, e.z + side.z);
        rlVertex3f(e.x - side.x, e.y, e.z - side.z);
    }
    if (P.swing > 0 && state == ST_PLAY) {
        float t = 1 - P.swing / .22f;
        const MBase& m = MB[P.melee];
        for (int i = 0; i < 10; i++) {
            float a0 = P.swingA - m.arc + 2 * m.arc * t * i / 10.f,
                  a1 = P.swingA - m.arc + 2 * m.arc * t * (i + 1) / 10.f;
            Vector2 p0 = Vector2Add(P.p, polar(a0, m.range * .5f)), p1 = Vector2Add(P.p, polar(a1, m.range * .5f)),
                    q0 = Vector2Add(P.p, polar(a0, m.range)), q1 = Vector2Add(P.p, polar(a1, m.range));
            int al = (int)(90 * (1 - t));
            rlColor4ub(220, 230, 240, 0);
            rlVertex3f(p0.x, 1.2f, p0.y);
            rlVertex3f(p1.x, 1.2f, p1.y);
            rlColor4ub(220, 230, 240, al);
            rlVertex3f(q1.x, 1.2f, q1.y);
            rlVertex3f(q0.x, 1.2f, q0.y);
        }
    }
    rlEnd();
    // rain
    if (rainLevel > .05f) {
        rlBegin(RL_LINES);
        int n = (int)(700 * rainLevel);
        for (int i = 0; i < n; i++) {
            float rx = c.x + fmodf(i * 97.31f + gTime * 3, vr * 2) - vr,
                  rz = c.y + fmodf(i * 53.7f, vr * 1.7f) - vr * .8f;
            float ry = 14 - fmodf(gTime * (22 + (i % 5) * 2) + i * 3.17f, 14);
            rlColor4ub(170, 190, 220, (unsigned char)(70 * (1 - dk * .6f)));
            rlVertex3f(rx, ry, rz);
            rlVertex3f(rx - .1f, ry + .9f, rz + .05f);
        }
        rlEnd();
    }
    rlDrawRenderBatchActive();
    rlEnableDepthMask();
    EndBlendMode();
}
