// Nightfall County: death.
// Not a standalone header: included once, in order, from src/main.cpp. Everything above it in that list is visible here.
#pragma once
// ============================================================ save / death
static void deleteSave();
static std::string savePath() { const char* h = getenv("HOME"); return std::string(h ? h : ".") + "/.nightfall3d_best"; }
static void loadBest() { std::ifstream f(savePath()); if (f) f >> bestHours >> bestKills; }
static bool newRecord = false; static std::string deathLine;
static void die() {
    state = ST_DEAD; P.hp = 0; deadT = 0; deleteSave(); /* permadeath: the save goes with you */ int hrs = (int)(P.alive / CYCLE * 24);
    newRecord = hrs > bestHours || (hrs == bestHours && kills > bestKills);
    if (newRecord) { bestHours = hrs; bestKills = kills; std::ofstream f(savePath()); f << bestHours << " " << bestKills; }
    static const std::vector<std::string> lines = {"YOU DIED.", "THE COUNTY KEEPS YOU.", "ANOTHER ONE FOR THE ROAD.", "SHOULD HAVE STAYED INSIDE."}; deathLine = pick(lines);
    spray(xz3(P.p, 1), -999, 80, 0, 1, 6, BLOOD_FRESH); decal(P.p, .5f, BLOOD_DRY, 2.f); screenGore(10);
}

