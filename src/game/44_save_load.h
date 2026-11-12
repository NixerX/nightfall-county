// Nightfall County: save load.
// Not a standalone header: included once, in order, from src/main.cpp. Everything above it in that list is visible here.
#pragma once
// ============================================================ save & quit (one slot; dying deletes it)
static std::string runSavePath() {
    const char* h = getenv("HOME");
    return std::string(h ? h : ".") + "/.nightfall3d_save";
}
static bool hasSave() {
    return FileExists(runSavePath().c_str());
}
static void deleteSave() {
    std::remove(runSavePath().c_str());
}
static void wCloth(std::ostream& o, const Cloth& c);
static void wItem(std::ostream& o, const InvItem& it) {
    o << it.kind << ' ' << it.id << ' ' << it.n << ' ' << it.x << ' ' << it.y << ' ' << it.rot << '\n';
    if (it.kind == 1)
        wCloth(o, *it.c);
}
static void wCloth(std::ostream& o, const Cloth& c) {
    o << c.def << ' ' << c.cond << ' ' << (int)c.col.r << ' ' << (int)c.col.g << ' ' << (int)c.col.b << ' ' << c.pattern
      << ' ' << c.rar << ' ' << c.upg << ' ' << c.extraRows << ' ' << c.set << ' ' << c.bWarm << ' ' << c.bWater << ' '
      << c.bArmor << ' ' << c.bQuiet << ' ' << c.bVis << ' ' << std::quoted(c.name) << ' ' << std::quoted(c.decor)
      << ' ' << c.items.size() << '\n';
    for (auto& it : c.items)
        wItem(o, it);
}
static std::shared_ptr<Cloth> rCloth(std::istream& in);
static InvItem rItem(std::istream& in) {
    InvItem it;
    in >> it.kind >> it.id >> it.n >> it.x >> it.y >> it.rot;
    if (it.kind == 1)
        it.c = rCloth(in);
    return it;
}
static std::shared_ptr<Cloth> rCloth(std::istream& in) {
    auto c = std::make_shared<Cloth>();
    int r, g, b;
    size_t n;
    in >> c->def >> c->cond >> r >> g >> b >> c->pattern >> c->rar >> c->upg >> c->extraRows >> c->set >> c->bWarm >>
        c->bWater >> c->bArmor >> c->bQuiet >> c->bVis >> std::quoted(c->name) >> std::quoted(c->decor) >> n;
    c->col = rgba(r, g, b);
    c->def = std::max(0, std::min(CD_N - 1, c->def));
    for (size_t i = 0; i < n && in; i++)
        c->items.push_back(rItem(in));
    return c;
}
static void wGun(std::ostream& o, const Weapon& w) {
    o << w.key << ' ' << w.rar << ' ' << w.cal << ' ' << w.slot << ' ' << w.dmg << ' ' << w.rate << ' ' << w.spread
      << ' ' << w.spd << ' ' << w.range << ' ' << w.rel << ' ' << w.kick << ' ' << w.len << ' ' << w.dism << ' '
      << w.crit << ' ' << w.leech << ' ' << w.noise << ' ' << w.pel << ' ' << w.mag << ' ' << w.snd << ' ' << w.pierce
      << ' ' << w.clip << ' ' << w.autoF << ' ' << w.burn << ' ' << w.aff.size();
    for (int a : w.aff)
        o << ' ' << a;
    o << ' ' << std::quoted(w.name) << '\n';
}
static Weapon rGun(std::istream& in) {
    Weapon w;
    size_t n;
    in >> w.key >> w.rar >> w.cal >> w.slot >> w.dmg >> w.rate >> w.spread >> w.spd >> w.range >> w.rel >> w.kick >>
        w.len >> w.dism >> w.crit >> w.leech >> w.noise >> w.pel >> w.mag >> w.snd >> w.pierce >> w.clip >> w.autoF >>
        w.burn >> n;
    for (size_t i = 0; i < n && in; i++) {
        int a;
        in >> a;
        w.aff.push_back(a);
    }
    in >> std::quoted(w.name);
    return w;
}
static void saveGame() {
    std::ofstream o(runSavePath());
    if (!o) {
        msg("Couldn't write the save file.", BLOODC);
        return;
    }
    o << "NFSAVE 1\n" << worldSeed << ' ' << cfg.foliage << ' ' << cfg.diff << '\n';
    o << gTime << ' ' << nights << ' ' << kills << ' ' << hordeDay << ' ' << playerFemale << ' ' << gibCount << '\n';
    o << P.p.x << ' ' << P.p.y << ' ' << P.a << ' ' << P.hp << ' ' << P.food << ' ' << P.water << ' ' << P.temp << ' '
      << P.sta << ' ' << P.bleeds << ' ' << P.sick << ' ' << P.pain << ' ' << P.broken << ' ' << P.splinted << ' '
      << P.splintT << ' ' << P.wet << ' ' << P.batt << ' ' << P.xp << ' ' << P.next << ' ' << P.lvl << ' ' << P.melee
      << ' ' << P.active << ' ' << P.cls << ' ' << P.points << ' ' << P.actCd << ' ' << P.shieldHP << ' ' << P.walked
      << ' ' << P.alive << '\n';
    for (int t = 0; t < 3; t++)
        for (int i = 0; i < 4; i++)
            o << P.rank[t][i] << ' ';
    o << '\n';
    for (int g = 0; g < 2; g++) {
        o << P.hasGun[g] << '\n';
        if (P.hasGun[g])
            wGun(o, P.gun[g]);
    }
    for (int s = 0; s < CS_COUNT; s++) {
        o << (P.eq[s] ? 1 : 0) << '\n';
        if (P.eq[s])
            wCloth(o, *P.eq[s]);
    }
    for (auto& h : hotbar)
        o << h.kind << ' ' << h.id << ' ';
    o << '\n';
    o << loot.size() << '\n';
    for (auto& l : loot) {
        o << l.kind << ' ' << l.id << ' ' << l.count << ' ' << l.p.x << ' ' << l.p.y << ' ' << l.p.z << ' ' << l.rot
          << '\n';
        if (l.kind == LK_GUN)
            wGun(o, l.w);
        if (l.kind == LK_CLOTH)
            wCloth(o, *l.c);
    }
    o << horde.on << ' ' << horde.a << ' ' << horde.t << ' ' << horde.left << ' ' << horde.end.x << ' ' << horde.end.y
      << ' ' << horde.lastSeen.x << ' ' << horde.lastSeen.y << ' ' << hordeDay << '\n';
    o << doorObjs.size() << '\n';
    for (auto& d : doorObjs)
        o << d.open << ' ' << d.broken << ' ' << d.hp << ' ';
    o << '\n';
    o << blds.size() << '\n';
    for (auto& b : blds)
        o << b.visited << ' ';
    o << '\n';
    o << carObjs.size() << '\n';
    for (auto& c : carObjs)
        o << c.searched << ' ';
    o << '\n';
    o << "END\n";
}
static bool loadGame() {
    std::ifstream in(runSavePath());
    std::string tag;
    int ver;
    if (!(in >> tag >> ver) || tag != "NFSAVE")
        return false;
    unsigned seed;
    int fol, diff;
    in >> seed >> fol >> diff;
    int keepF = cfg.foliage;
    cfg.foliage = fol;
    newGame(seed);
    cfg.foliage = keepF;
    cfg.diff = diff;
    in >> gTime >> nights >> kills >> hordeDay >> playerFemale >> gibCount;
    in >> P.p.x >> P.p.y >> P.a >> P.hp >> P.food >> P.water >> P.temp >> P.sta >> P.bleeds >> P.sick >> P.pain >>
        P.broken >> P.splinted >> P.splintT >> P.wet >> P.batt >> P.xp >> P.next >> P.lvl >> P.melee >> P.active >>
        P.cls >> P.points >> P.actCd >> P.shieldHP >> P.walked >> P.alive;
    for (int t = 0; t < 3; t++)
        for (int i = 0; i < 4; i++)
            in >> P.rank[t][i];
    for (int g = 0; g < 2; g++) {
        in >> P.hasGun[g];
        if (P.hasGun[g])
            P.gun[g] = rGun(in);
    }
    for (int s = 0; s < CS_COUNT; s++) {
        int has;
        in >> has;
        P.eq[s] = has ? rCloth(in) : nullptr;
    }
    for (auto& h : hotbar)
        in >> h.kind >> h.id;
    size_t n;
    in >> n;
    loot.clear();
    for (size_t i = 0; i < n && in; i++) {
        Loot l;
        in >> l.kind >> l.id >> l.count >> l.p.x >> l.p.y >> l.p.z >> l.rot;
        l.settled = true;
        if (l.kind == LK_GUN)
            l.w = rGun(in);
        if (l.kind == LK_CLOTH)
            l.c = rCloth(in);
        loot.push_back(l);
    }
    {
        int on = 0;
        in >> on >> horde.a >> horde.t >> horde.left >> horde.end.x >> horde.end.y >> horde.lastSeen.x >>
            horde.lastSeen.y >> hordeDay;
        horde.on = on != 0;
    }
    in >> n;
    for (size_t i = 0; i < n && in; i++) {
        DoorObj tmp;
        in >> tmp.open >> tmp.broken >> tmp.hp;
        if (i < doorObjs.size()) {
            doorObjs[i].open = tmp.open;
            doorObjs[i].broken = tmp.broken;
            doorObjs[i].hp = tmp.hp;
            doorObjs[i].anim = tmp.open || tmp.broken ? 1 : 0;
        }
    }
    in >> n;
    for (size_t i = 0; i < n && in; i++) {
        bool v;
        in >> v;
        if (i < blds.size())
            blds[i].visited = v;
    }
    in >> n;
    for (size_t i = 0; i < n && in; i++) {
        bool v;
        in >> v;
        if (i < carObjs.size())
            carObjs[i].searched = v;
    }
    in >> tag;
    if (tag != "END") {
        msg("Save file looked damaged; some things may be off.", SODIUM);
    }
    zombies.clear();
    camPos = P.p;
    phase = isNight() ? 'n' : 'd';
    recalcPerks();
    msgs.clear();
    msg("Picked up where you left off.", OKC);
    state = ST_PLAY;
    return true;
}
