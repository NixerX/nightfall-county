# PLAN: Survivor Encounters (first pass)

Target: v0.9.0. Followers/escort come **after** this. The rule for this pass is simple and reproducible: every encounter is a survivor standing at a spot, with a need. Meet the need and you get a reward.

## Scope

**In:** six encounter types, one shared encounter system, map markers, rescue → roster stub, save/load.

**Out (later):** followers and escort AI, the radio call, a grateful trader showing up again, safehouse jobs and support roles, playable survivors and take-over-on-death, human factions and raids.

---

## 1. Shared encounter framework

Everything below uses one struct and one update loop. No scenario gets its own system.

```cpp
enum EncType { ENC_HURT_ROAD, ENC_TRAPPED_HOUSE, ENC_HAYLOFT, ENC_FETCH, ENC_SIGNAL, ENC_DEFEND };
enum EncState { ENC_DORMANT, ENC_ACTIVE, ENC_ENGAGED, ENC_RESOLVED, ENC_FAILED };

struct Encounter {
    EncType type;
    EncState state;
    Vector2 pos;          // where the survivor stands
    int building = -1;    // blds[] index if inside one (house, barn)
    int needItem = -1;    // item id for HURT_ROAD / FETCH
    int targetBuilding = -1; // FETCH: where the item is
    int linkedEnc = -1;   // SIGNAL: the encounter it points at
    float timer = 0;      // DEFEND hold timer, despawn timer
    int survivorId = -1;  // index into survivors[] (name, class, perk)
    uint32_t seed;        // reproducible from the world seed
};
static std::vector<Encounter> encs;
```

**Survivor record** (kept tiny for now):

```cpp
struct Survivor {
    std::string name;
    int cls = -1;     // -1 = civilian/support, else an existing class id
    int perk = -1;    // one perk, from that class's tree if cls >= 0
    bool rescued = false, dead = false;
};
```
About 30–40% of survivors roll an existing class (the "some, not all" rule). The rest are civilians, kept for support roles later.

**Lifecycle**
- `DORMANT` → spawned in the world but the player is far away. Nothing simulates.
- `ACTIVE` → player is within ~120 m. The scenario setup spawns (zombies around the house, under the loft, etc.).
- `ENGAGED` → player has talked to them, or the defend timer has started.
- `RESOLVED` → rescued. Pay the reward, mark the survivor `rescued`, start the despawn walk. For now, "sent to the safehouse" just means they walk off toward the nearest road edge and fade out.
- `FAILED` → survivor died or the timer ran out. Leave a body with loot on it, so it isn't a total loss.

**Spawning**
- Seeded off the world seed plus a cell index, so the same map always gets the same encounters.
- Budget: 1 per ~4 towns/farm clusters, minimum 300 m apart, never within 150 m of spawn.
- Respawn: after a resolved/failed encounter, that cell can roll a new one after 2 in-game days.
- Each type has an eligibility check (needs a farmhouse plot, needs a barn with a loft, needs a road segment, etc.). If the check fails, reroll the type.

**Talking to them**
- A prompt reuses the existing interact key: `[E] Talk`.
- One line of text tells you the need ("My leg's busted. Got a splint?"). No dialogue trees.
- While they're `ACTIVE`, survivors take zombie aggro like the player does, at a lower priority than the player.

**UI**
- The map gets a survivor icon once you've seen or heard the survivor, or a signal points at them. It's grey once resolved.
- A one-line "objective" under the sun/moon HUD bar while you're engaged: `Get a splint to Dale (Farmhouse, 80m)`.
- A toast on rescue: `Dale is heading to the safehouse. (+1 survivor)`.

**Save/load:** serialize `encs` and `survivors` into the existing single Save & Continue slot. On load, any `ACTIVE`/`ENGAGED` encounter goes back to `ACTIVE` and re-runs its setup. Don't save the encounter's zombies individually. This avoids a repeat of the bug where hordes vanished on reload.

---

## 2. The six scenarios

### 1. Hurt on the road
- **Setup:** survivor sitting against a wreck or a fence on a road segment, with 2–4 shamblers wandering nearby. They're bleeding or have a broken leg.
- **Need:** one item from a short list: bandage/rag, splint, painkillers, antibiotics. Weight the roll toward whatever is common in this area.
- **Resolve:** hand the item over with `[E] Give <item>`. They get up and walk off.
- **Fail:** they bleed out after ~6 in-game hours once you've seen them, or zombies kill them.
- **Reward:** small: some ammo or food, plus the survivor.
- **Reuses:** the existing wound/bleed items, wrecks, road plots.

### 2. Trapped in a house
- **Setup:** survivor inside a farmhouse or town house with the doors closed. There are 6–12 zombies around the building, and some are beating on the doors using the per-type door damage.
- **Need:** clear the zombies near the house. The win check is no hostile zombies within 15 m of the building for 5 seconds.
- **Resolve:** they open the door and come out.
- **Fail:** the zombies break a door and kill them, so the door-damage timer actually matters. Give the player ~3–4 minutes of real time from `ACTIVE` before the first door can go.
- **Reward:** medium: they tell you about a stash in that house's basement, or one in a building nearby. Mark it on the map.
- **Reuses:** door damage, building interiors, basements.

### 3. Pinned in a hayloft
- **Setup:** survivor up in a barn's hayloft (that's already a walkable safe zone), with 5–10 zombies milling around on the barn floor below. Optionally one brute, at Hard and above only.
- **Need:** clear the barn floor. Or **distract** the zombies: any noise source (a pipe bomb or nail bomb, a car alarm if there is one, a gunshot from 40 m away) that pulls the group out of the barn counts.
- **Resolve:** they climb down once the floor is clear for 5 seconds.
- **Fail:** rare. Zombies can't reach the loft, so the only failure is a timer: they give up and leave after a day, and the encounter goes `FAILED` with no body.
- **Reward:** medium: they give you whatever they were holding up there (rare loot roll).
- **Note:** this is the scenario that teaches distraction rather than fighting. Keep the loft safe and don't let zombies climb.

### 4. Fetch
- **Setup:** survivor holed up somewhere safe-ish (a porch, a tent, a shack), not in immediate danger.
- **Need:** a specific item from a specific nearby building, 60–200 m away. Spawn that item in that building when the encounter goes `ACTIVE`, so it always exists. Mark the building on the map.
- **Resolve:** bring it back and `[E] Give`.
- **Fail:** none, apart from the survivor getting killed by a passing horde. Despawn after 2 in-game days if you ignore them.
- **Reward:** medium-plus, because it's a trip: a class-set clothing piece, or a weapon.
- **Reuses:** loot spawning, building lookup, the map marker system.
- **Variety:** a clinic gives meds, a garage gives a car part, a police car or station gives ammo, a farmhouse gives food/seed. Themed to the building type you already have.

### 5. Signal
- **Setup:** a flare goes up at night, or a smoke column rises during the day. It always points at one of the other five, picked when the signal spawns.
- **Need:** go there.
- **Resolve:** reaching within 30 m flips the linked encounter to `ACTIVE`, and the signal is done.
- **Visibility:** seen from up to ~400 m. A flare is a short-lived bright point with a light, repeated every ~90 s until you get close. Smoke is a particle column that's always on.
- **Also:** the flare makes noise, so it also pulls zombies toward the survivor. That's the risk of leaving it.
- **Reward:** none on its own. It's a pointer.
- **Note:** this is the cheapest one to build and makes the other five easier to find. Build it right after the framework.

### 6. Defend
- **Setup:** survivor barricaded in a building who says "they're coming". When you talk to them, a wave starts.
- **Need:** survive 90–120 seconds. Waves come from 2–3 directions, sized off the current horde scaling (see tuning). The survivor stays inside.
- **Resolve:** the timer ends with the survivor alive.
- **Fail:** the survivor dies (zombies get in through a door), or the player leaves (more than 40 m away for more than 10 s).
- **Reward:** big: a rare loot roll, and this survivor always rolls a real class.
- **Note:** it's the closest to a "horde mode" moment. Brutes are capped per difficulty here even though that's on hold elsewhere, because 2–3 brutes in a defend would be unwinnable right now.

---

## 3. Tuning table (starting numbers)

| Scenario | Zombies at setup | Brutes | Timer | Reward tier |
|---|---|---|---|---|
| Hurt on road | 2–4 | 0 | 6 in-game hrs to bleed out | Small |
| Trapped house | 6–12 | 0 (Normal), ≤1 (Hard+) | ~3–4 min before first door can break | Medium |
| Hayloft | 5–10 | 0 (Normal), ≤1 (Hard+) | gives up after 1 day | Medium |
| Fetch | 0 at survivor | 0 | despawns after 2 days | Medium+ |
| Signal | 0 | 0 | flare every 90 s | None |
| Defend | waves, total ≈ 1.5× a small horde | ≤1 (Normal), ≤2 (Hard+) | 90–120 s hold | Big |

All numbers go in one `encTuning` block so they can be changed with sed.

---

## 4. Build order

1. **Framework:** the `Encounter` and `Survivor` structs, the state machine, seeded spawning, the talk prompt, the map icon, the objective line, save/load. Test with a dummy "survivor stands there, press E, rescued."
2. **Signal:** flare and smoke visuals, linking to an encounter.
3. **Hurt on the road** and **Fetch:** both are item handoffs, so they share the Give code.
4. **Trapped in a house** and **Hayloft:** both are "clear the area" checks, so they share the radius check.
5. **Defend:** the wave spawner and the hold timer.
6. **Rescue roster stub:** a simple list screen (names, class or "civilian", perk) on the pause menu, so rescues feel like they count before the safehouse exists.

Each step gets its own commit. Steps 1–3 could ship as v0.9.0, with 4–6 in v0.9.1, if usage is tight.

---

## 5. Test checklist

- [ ] The same seed gives the same encounters in the same places.
- [ ] Save mid-encounter, reload: the encounter is back to `ACTIVE`, the zombies respawn, and nothing goes missing.
- [ ] A fetch item always exists in its target building.
- [ ] Hayloft: zombies can't reach the survivor, and a noise distraction alone can solve it.
- [ ] Trapped house: the door breaks if you dawdle, and doesn't if you're quick.
- [ ] Defend: walking away fails it, and the brute cap is respected on every difficulty.
- [ ] A signal always points at a valid, still-dormant encounter.
- [ ] A failed encounter leaves a lootable body, except the hayloft one.
- [ ] Rescued survivors show up in the roster, about 1 in 3 with a class.

## 6. Open questions

- Should rescued class survivors be **playable** right away (from the roster), or only once the safehouse exists?
- Should survivors fight back at all in pass one, or are they just bodies that need saving? (Suggested: no fighting until followers.)
- Signal at night only, or day smoke too? (Suggested: both. Smoke is cheap.)
