// Nightfall County: classes.
// Not a standalone header: included once, in order, from src/main.cpp. Everything above it in that list is visible here.
#pragma once
// ============================================================ classes & random skill trees
// Every life rolls a class. You start with 2 skill points and every level adds 1, all placed by fate, never by you.
enum Stat { S_DMG, S_RATE, S_CRIT, S_METAB, S_STA, S_LUCK, S_BLAST, S_COLD, S_MELEE, S_NOISE, S_MEDIC, S_REGEN, S_MAXHP, S_RELOAD, S_ARMOR, S_SPEED, S_PISTOL, S_SHOTGUN, S_RIFLE, S_STEALTH, S_CDR, S_AMMO, S_BATT, S_BLEEDRES, S_BLOCK, S_KICK, S_COUNT };
struct Skill { const char* n; const char* d; int stat; float per; int max; };
struct SkillTree { const char* n; Skill s[4]; };
struct ClassDef { const char* n; const char* blurb; const char* act; const char* actD; float actCd; Color c; SkillTree t[3]; };
static const ClassDef CLASSES[5] = {
    {"Police Officer", "Sheriff's deputy. Knows a pistol, owns a vest, never got the overtime.", "Adrenaline Dump", "6s of +60% fire rate and instant reloads.", 60, rgba(110, 155, 255),
     {{"Marksman", {{"Steady Hands", "+12% pistol and SMG damage", S_PISTOL, .12f, 3}, {"Sight Picture", "+4% headshot chance", S_CRIT, .04f, 3}, {"Speed Loader", "+15% reload speed", S_RELOAD, .15f, 3}, {"Double Tap", "+8% fire rate", S_RATE, .08f, 2}}},
      {"Patrol", {{"Body Armor", "Take 6% less damage", S_ARMOR, .06f, 3}, {"Beat Cop", "Sprinting drains 10% less stamina", S_STA, .10f, 3}, {"Evidence Locker", "+15% ammo found", S_AMMO, .15f, 3}, {"Tactical Light", "Flashlight drains 20% slower", S_BATT, .20f, 2}}},
      {"Riot", {{"Breacher", "+15% shotgun damage", S_SHOTGUN, .15f, 3}, {"Baton Work", "+15% melee damage", S_MELEE, .15f, 3}, {"Thick Neck", "+10 max health", S_MAXHP, 10, 3}, {"Shield Wall", "Blocks stop 10% more; shields last longer", S_BLOCK, .10f, 2}}}}},
    {"Paramedic", "Twelve years on the ambulance. Has seen worse. Not much worse.", "Trauma Kit", "Heal 40, close every wound, kill the pain, set a broken leg.", 75, rgba(126, 210, 126),
     {{"Triage", {{"Pressure Points", "Bandages heal 50% more and close extra wounds", S_MEDIC, .5f, 3}, {"Clotting Agent", "15% chance a hit doesn't bleed", S_BLEEDRES, .15f, 3}, {"Second Wind", "Regenerate faster while healthy", S_REGEN, .08f, 3}, {"Rapid Response", "Trauma Kit recharges 15% faster", S_CDR, .15f, 2}}},
      {"Pharmacist", {{"Iron Stomach", "Hunger and thirst 10% slower", S_METAB, .10f, 3}, {"Pill Counter", "Better loot and drop odds", S_LUCK, .10f, 3}, {"Immune System", "+8 max health", S_MAXHP, 8, 3}, {"Night Shift", "Lose body heat 15% slower", S_COLD, .15f, 3}}},
      {"Survivor", {{"Light Step", "Make 12% less noise", S_NOISE, .12f, 3}, {"Scrubs Gray", "The dead spot you 8% later", S_STEALTH, .08f, 3}, {"Cardio", "+4% move speed", S_SPEED, .04f, 3}, {"Shove", "Kicks hit 25% harder and send them flying", S_KICK, .25f, 2}}}}},
    {"Mechanic", "Fixed every truck in the county. Now breaks things on purpose.", "Nail Bomb", "Wire a pipe bomb out of scrap and throw it. Free.", 50, rgba(240, 164, 58),
     {{"Demolition", {{"Short Fuse", "+12% blast radius", S_BLAST, .12f, 3}, {"Shrapnel", "+6% damage with everything", S_DMG, .06f, 3}, {"Scrap Hoarder", "+12% ammo found", S_AMMO, .12f, 3}, {"Quick Hands", "Nail Bomb recharges 12% faster", S_CDR, .12f, 2}}},
      {"Grease Monkey", {{"Coveralls", "Take 5% less damage", S_ARMOR, .05f, 3}, {"Work Boots", "Sprinting drains 10% less stamina", S_STA, .10f, 3}, {"Lunchbox", "Hunger and thirst 10% slower", S_METAB, .10f, 3}, {"Welded Shield", "Blocks stop 10% more; shields last longer", S_BLOCK, .10f, 2}}},
      {"Wrench", {{"Heavy Swing", "+15% melee damage", S_MELEE, .15f, 3}, {"Tool Belt", "+12% reload speed", S_RELOAD, .12f, 3}, {"Bull Neck", "+10 max health", S_MAXHP, 10, 3}, {"Shotgun Rack", "+12% shotgun damage", S_SHOTGUN, .12f, 2}}}}},
    {"Hunter", "Hunted elk off the ridge every fall. Elk don't get back up.", "Deadeye", "8s where every bullet finds the head.", 70, rgba(170, 200, 90),
     {{"Rifleman", {{"Hunting Load", "+15% rifle damage", S_RIFLE, .15f, 3}, {"Breathing", "+5% headshot chance", S_CRIT, .05f, 3}, {"Bolt Work", "+7% fire rate", S_RATE, .07f, 3}, {"Big Game", "+5% damage with everything", S_DMG, .05f, 2}}},
      {"Tracker", {{"Downwind", "The dead spot you 10% later", S_STEALTH, .10f, 3}, {"Soft Soles", "Make 12% less noise", S_NOISE, .12f, 3}, {"Long Legs", "+4% move speed", S_SPEED, .04f, 3}, {"Patience", "Deadeye recharges 12% faster", S_CDR, .12f, 2}}},
      {"Woodsman", {{"Wool Layers", "Lose body heat 20% slower", S_COLD, .20f, 3}, {"Jerky", "Hunger and thirst 12% slower", S_METAB, .12f, 3}, {"Skinner", "+12% melee damage", S_MELEE, .12f, 3}, {"Boot to the Head", "Kicks hit 25% harder and send them flying", S_KICK, .25f, 2}}}}},
    {"Ex-Con", "Got out the week it started. Prison was better prep than you'd think.", "Bloodlust", "8s of double melee damage. Every hit heals you.", 60, rgba(255, 107, 91),
     {{"Butcher", {{"Shank", "+18% melee damage", S_MELEE, .18f, 3}, {"Yard Muscle", "+10 max health", S_MAXHP, 10, 3}, {"Bruiser", "Take 5% less damage", S_ARMOR, .05f, 3}, {"Rage", "Bloodlust recharges 12% faster", S_CDR, .12f, 2}}},
      {"Street", {{"Five-Finger", "Better loot and drop odds", S_LUCK, .12f, 3}, {"Lookout", "The dead spot you 8% later", S_STEALTH, .08f, 3}, {"Sticky Fingers", "+15% ammo found", S_AMMO, .15f, 3}, {"Dirty Fighting", "Kicks hit 30% harder and send them flying", S_KICK, .30f, 2}}},
      {"Hard Time", {{"Hard to Kill", "Regenerate faster while healthy", S_REGEN, .08f, 3}, {"Cast Iron Gut", "Hunger and thirst 12% slower", S_METAB, .12f, 3}, {"Jailhouse Ink", "12% chance a hit doesn't bleed", S_BLEEDRES, .12f, 3}, {"Zip Gun", "+12% pistol and SMG damage", S_PISTOL, .12f, 2}}}}}};
static const ClassDef& CL() { return CLASSES[P.cls]; }
static int setCount(int si);
static void recalcPerks() {
    float st[S_COUNT] = {}; const ClassDef& C = CL();
    for (int t = 0; t < 3; t++) for (int i = 0; i < 4; i++) st[C.t[t].s[i].stat] += C.t[t].s[i].per * P.rank[t][i];
    Perks& k = P.perks; k = Perks();
    k.dmg = 1 + st[S_DMG]; k.rate = 1 + st[S_RATE]; k.crit = .06f + st[S_CRIT]; k.metab = 1 - st[S_METAB]; k.sta = 1 - st[S_STA]; k.luck = 1 + st[S_LUCK]; k.blast = 1 + st[S_BLAST]; k.cold = 1 - st[S_COLD];
    k.melee = 1 + st[S_MELEE]; k.noise = 1 - st[S_NOISE]; k.medic = 1 + st[S_MEDIC]; k.regen = st[S_REGEN]; k.maxhp = 100 + st[S_MAXHP]; k.reload = 1 + st[S_RELOAD]; k.armor = st[S_ARMOR]; k.speed = 1 + st[S_SPEED];
    k.pistol = 1 + st[S_PISTOL]; k.shotgun = 1 + st[S_SHOTGUN]; k.rifle = 1 + st[S_RIFLE]; k.stealth = st[S_STEALTH]; k.cdr = st[S_CDR]; k.ammo = 1 + st[S_AMMO]; k.batt = 1 - st[S_BATT]; k.bleedres = st[S_BLEEDRES]; k.block = st[S_BLOCK]; k.kick = 1 + st[S_KICK];
    // set bonuses
    int sc[5]; for (int i = 0; i < 5; i++) sc[i] = setCount(i);
    if (sc[0] >= 2) k.armor += .08f; if (sc[0] >= 4) { k.pistol += .12f; k.shotgun += .12f; k.reload += .1f; }
    if (sc[1] >= 2) k.medic += .4f; if (sc[1] >= 4) { k.bleedres += .15f; k.regen += .3f; }
    if (sc[2] >= 2) k.melee += .1f; if (sc[2] >= 4) { k.sta *= .85f; k.kick += .25f; }
    if (sc[3] >= 2) k.stealth += .08f; if (sc[3] >= 4) { k.rifle += .15f; k.noise *= .88f; }
    if (sc[4] >= 2) k.luck += .12f; if (sc[4] >= 4) { k.melee += .12f; k.kick += .3f; }
    if (P.cls >= 0 && P.cls < 5 && sc[P.cls] >= 2) k.cdr += .25f; // wearing your own class's set
}
// Fate picks: any skill whose rank isn't maxed and whose predecessor in its tree has at least one point.
static std::string randomSkillPoint() {
    std::vector<std::pair<int, int>> c; const ClassDef& C = CL();
    for (int t = 0; t < 3; t++) for (int i = 0; i < 4; i++) if (P.rank[t][i] < C.t[t].s[i].max && (i == 0 || P.rank[t][i - 1] > 0)) c.push_back({t, i});
    if (c.empty()) { P.perks.maxhp += 2; return "Every tree is full. +2 max health instead."; }
    auto pk = c[irand(0, (int)c.size() - 1)]; P.rank[pk.first][pk.second]++; P.points++; float mh = P.perks.maxhp; recalcPerks(); P.hp += P.perks.maxhp - mh;
    const Skill& sk = C.t[pk.first].s[pk.second];
    return std::string(C.t[pk.first].n) + ": " + sk.n + " " + std::to_string(P.rank[pk.first][pk.second]) + "/" + std::to_string(sk.max) + "  (" + sk.d + ")";
}
static void useActive() {
    if (P.actCd > 0) { floatText(xz3(P.p, 2.2f), std::string(CL().act) + " " + std::to_string((int)ceilf(P.actCd)) + "s", STEEL, 13); sfx(SFX_EMPTY); return; }
    P.actCd = CL().actCd * (1 - P.perks.cdr); sfx(SFX_LEVEL, .8f, 0);
    switch (P.cls) {
    case 0: P.actT = 6; msg("Adrenaline dump. Make it count.", CL().c); break;
    case 1: P.hp = std::min(P.perks.maxhp, P.hp + 40); P.bleeds = 0; P.pain = 0;
        if (P.broken) { P.broken = false; P.splinted = false; msg("Trauma kit: patched up and the leg's set properly. You can run again.", CL().c); showBanner("LEG FIXED", "Trauma kit", CL().c); }
        else msg("Trauma kit: patched up.", CL().c); break;
    case 2: throwItem(false, true); msg("Nail bomb out.", CL().c); break;
    case 3: P.actT = 8; msg("Deadeye. Breathe out.", CL().c); break;
    case 4: P.actT = 8; msg("Bloodlust.", CL().c); break;
    }
    floatText(xz3(P.p, 2.4f), CL().act, CL().c, 16);
}
static void grantLevelPoints() {
    while (pendingLv > 0) { pendingLv--; std::string r = randomSkillPoint(); showBanner("LEVEL " + std::to_string(P.lvl), "+1  " + r, SODIUM); msg("+1 " + r, SODIUM); sfx(SFX_LEVEL, 1, 0); }
}

