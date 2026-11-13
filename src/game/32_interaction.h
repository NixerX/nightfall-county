// Nightfall County: interaction.
// Not a standalone header: included once, in order, from src/main.cpp. Everything above it in that list is visible here.
#pragma once
// ============================================================ interaction (E): loot first, then doors, ladders, fires, cars
enum ActKind {
    A_NONE,
    A_DOOR,
    A_LADDER,
    A_FIRE,
    A_CAR,
    A_STAIRS
};
struct Act {
    int kind = A_NONE, idx = -1;
};
static Act nearAct;
static Act findAct() {
    Act a;
    float best = 1e9f;
    if (P.loft >= 0) {
        if (Vector2Distance(P.p, lofts[P.loft].top) < 1.6f) {
            a.kind = A_LADDER;
            a.idx = P.loft;
        }
        return a;
    }
    for (int i = 0; i < (int)doorObjs.size(); i++) {
        const DoorObj& d = doorObjs[i];
        if (d.broken)
            continue;
        float dd = Vector2Distance(d.c, P.p);
        if (dd < 2.2f && dd < best) {
            best = dd;
            a = {A_DOOR, i};
        }
    }
    for (int i = 0; i < (int)lofts.size(); i++) {
        float dd = Vector2Distance(lofts[i].base, P.p);
        if (dd < 1.6f && dd < best) {
            best = dd;
            a = {A_LADDER, i};
        }
    }
    for (int i = 0; i < (int)stairs.size(); i++) {
        float dd = std::min(Vector2Distance(stairs[i].a, P.p), Vector2Distance(stairs[i].b, P.p));
        if (dd < 1.9f && dd < best) {
            best = dd;
            a = {A_STAIRS, i};
        }
    }
    if (invCount(IT_KINDLING) || invCount(IT_LOG))
        for (int i = 0; i < (int)fires.size(); i++) {
            float dd = Vector2Distance(fires[i].p, P.p);
            if (dd < 2.2f && dd < best) {
                best = dd;
                a = {A_FIRE, i};
            }
        }
    if (a.kind == A_NONE)
        for (int i = 0; i < (int)carObjs.size(); i++) {
            const CarObj& c = carObjs[i];
            if (c.searched)
                continue;
            float dd = Vector2Distance(c.p, P.p);
            if (dd < 4.f && dd < best) { // car search reach (from centre)
                best = dd;
                a = {A_CAR, i};
            }
        }
    return a;
}
static std::string actLabel(const Act& a) {
    switch (a.kind) {
    case A_DOOR:
        return doorObjs[a.idx].open ? "Close door" : "Open door";
    case A_LADDER:
        return P.loft >= 0 ? "Climb down" : "Climb to the hayloft";
    case A_FIRE:
        return invCount(IT_LOG) ? "Throw a log on the fire" : "Feed the fire kindling";
    case A_CAR:
        return carObjs[a.idx].burned ? "Pick through the wreck" : "Search car";
    case A_STAIRS: {
        const Stair& st = stairs[a.idx];
        bool atA = Vector2Distance(st.a, P.p) < Vector2Distance(st.b, P.p);
        return st.down ? (atA ? "Go down to the basement" : "Climb out of the basement")
                       : (atA ? "Go upstairs" : "Go downstairs");
    }
    }
    return "";
}
static bool bodyInDoor(const DoorObj& d) {
    auto in = [&](Vector2 p, float r) {
        return d.horiz ? fabsf(p.y - d.c.y) < .25f + r && fabsf(p.x - d.c.x) < d.w / 2 + r
                       : fabsf(p.x - d.c.x) < .25f + r && fabsf(p.y - d.c.y) < d.w / 2 + r;
    };
    if (in(P.p, .35f))
        return true;
    for (auto& z : zombies)
        if (in(z.p, z.r))
            return true;
    return false;
}
struct Follower {
    int type;
    float t;
    Vector2 to;
    int cell;
};
static std::vector<Follower> followers; // zombies coming after you through the stairs
static void takeStairs(int si) {
    const Stair& st = stairs[si];
    bool atA = Vector2Distance(st.a, P.p) < Vector2Distance(st.b, P.p);
    Vector2 from = atA ? st.a : st.b, to = atA ? st.b : st.a;
    int toB = atA ? st.bB : st.bA;
    for (auto& z : zombies)
        if (!z.dead && z.aggro > 0 && Vector2Distance(z.p, from) < 26) {
            followers.push_back({z.type, Vector2Distance(z.p, from) / 2.6f + frand(1.5f, 4), to, toB});
            z.gone = true;
        }
    Building& tb = blds[toB];
    Vector2 ctr = V2((tb.x0 + tb.x1) / 2, (tb.z0 + tb.z1) / 2);
    Vector2 dir = Vector2Normalize(Vector2Subtract(ctr, to));
    Vector2 dst = Vector2Add(to, Vector2Scale(dir, 1.5f));
    if (!freeAt(dst.x, dst.y, .35f))
        for (int k = 0; k < 16; k++) {
            Vector2 q = Vector2Add(to, polar(k * PI / 8, 1.5f));
            if (freeAt(q.x, q.y, .35f)) {
                dst = q;
                break;
            }
        }
    P.p = dst;
    camPos = P.p;
    P.k = V2(0, 0);
    P.vel = V2(0, 0);
    P.autorun = false;
    sfx(SFX_STEP, .6f);
    sfx(SFX_DOOR_OPEN, .3f);
    if (tb.cellKind && !tb.visited) {
        tb.visited = true;
        int n = tb.cellKind == 2 ? (chance(.55f) ? irand(1, 3) : 0)
                                 : (chance(.35f) ? irand(1, 2) : 0); // someone was already down here
        for (int k = 0; k < n; k++)
            for (int t = 0; t < 20; t++) {
                Vector2 q = V2(frand(tb.x0 + 1, tb.x1 - 1), frand(tb.z0 + 1, tb.z1 - 1));
                if (Vector2Distance(q, P.p) > 3.5f && freeAt(q.x, q.y, .4f)) {
                    Zombie& z = spawnZ(Z_WALKER, q);
                    z.aggro = 0;
                    break;
                }
            }
    }
    msg(tb.cellKind == 2   ? "The basement. Smells like mildew and worse."
        : tb.cellKind == 1 ? "Upstairs."
                           : "Back on the ground floor.",
        STEEL);
}
static void doAct(const Act& a) {
    switch (a.kind) {
    case A_STAIRS:
        takeStairs(a.idx);
        return;
    case A_DOOR: {
        DoorObj& d = doorObjs[a.idx];
        if (d.open && bodyInDoor(d)) {
            msg("Something's in the doorway.", STEEL);
            return;
        }
        d.open = !d.open;
        sfx(d.open ? SFX_DOOR_OPEN : SFX_DOOR_CLOSE);
        noiseAt(d.c, 6);
        break;
    }
    case A_LADDER:
        if (P.loft >= 0) {
            P.p = lofts[P.loft].base;
            P.loft = -1;
            msg("Back on the ground.", STEEL);
        } else {
            P.loft = a.idx;
            P.p = lofts[a.idx].top;
            P.crouch = true;
            msg("Up in the hayloft. Stay quiet and they'll lose interest.", OKC);
        }
        sfx(SFX_CREAK);
        break;
    case A_FIRE: {
        Fire& f = fires[a.idx];
        if (invTake(IT_LOG, 1)) {
            f.fuel = std::min(900.f, f.fuel + 260);
            msg("Log on the fire.", rgba(255, 170, 90));
        } else if (invTake(IT_KINDLING, 1)) {
            f.fuel = std::min(900.f, f.fuel + 60);
            msg("Fed the fire.", rgba(255, 170, 90));
        }
        sfx(SFX_PICK);
        break;
    }
    case A_CAR: {
        CarObj& c = carObjs[a.idx];
        c.searched = true;
        sfx(SFX_DOOR_OPEN);
        Vector3 drop = xz3(Vector2Add(P.p, polar(P.a + PI, .5f)), .05f);
        if (c.burned) {
            if (chance(.2f)) {
                rollTable(drop, LT_CAR);
                msg("Something survived the fire.", BONE);
            } else
                msg("Burned out. Nothing left but ash.", STEEL);
        } else {
            int n = 0;
            size_t before = loot.size();
            rollTable(drop, LT_CAR);
            if (chance(.5f))
                rollTable(drop, LT_CAR);
            n = (int)(loot.size() - before);
            msg(n ? "Found something in the car." : "Glovebox and trunk: nothing.", n ? BONE : STEEL);
        }
        noiseAt(c.p, 8);
        break;
    }
    }
}
static void interact() {
    if (nearLoot >= 0) {
        pickUp();
        return;
    }
    if (nearAct.kind != A_NONE)
        doAct(nearAct);
}
