# PLAN: more rural houses and shacks, more variety

## Goal
The countryside is mostly identical farms (same size house, same red barn) and identical cabins.
Add variety, plus new roadside homesteads: small ranch houses, two-story houses and weathered shacks.
Houses already get random paint, brick/siding, condition, rooms, basements (32%) and an upstairs
when they have 2 floors, so new buildings get all of that for free.

## Rules for the coder
- All edits are in `src/game/15_gen_world.h`, inside `genWorld()`. Touch nothing else.
- Helpers you can use (already exist): `frand(a, b)`, `irand(a, b)`, `chance(p)`, `pick(std::vector<Color>{...})`,
  `rgba(r, g, b)`, `V2(x, z)`, `Vector2Add`, `makeBuilding(x0, z0, w, d, type, floors, doorSide, town, wallC, roofC)`
  (returns the building index, or -1 if it didn't fit), `woodpile(V2, -1)`, `car(x, z, yaw, burnP)`,
  `track(Vector2 from, int tier)`, `townAt(p, k)`, `hotAt(p, k)`, `roadDist(p)`, `nearestRoadPt(p)`,
  `HOUSE_WALL`, `ROOF_C`, `blds[i]`.
- `doorSide`: 0 = north (z0), 1 = south (z1), 2 = west (x0), 3 = east (x1).
- Do one step, run `./build.sh`, play a new life to look, then commit.
- After each step run `clang-format -i src/game/15_gen_world.h`.

---

## Step 1: Farmhouse size and floors
In the `// farms (random)` loop, find the line starting `int fh = makeBuilding(fx, fz, frand(8, 10), frand(7, 8), B_HOUSE,`.
- Change the width `frand(8, 10)` to `frand(8, 12)`.
- Change the depth `frand(7, 8)` to `frand(7, 10)`.
- Change the floors `chance(.5f) ? 2 : 1` to `chance(.55f) ? 2 : 1`.
Nothing else on that line changes.

## Step 2: Barn size and colour
Same farms loop, the line starting `if (makeBuilding(fx + 16, fz - 4, 14, 20, B_BARN,`.
- Just before that `if`, add:
  ```cpp
  Color barnC = pick(std::vector<Color>{rgba(130, 50, 40), rgba(130, 50, 40), rgba(150, 146, 136), rgba(118, 96, 74),
                                        rgba(200, 196, 186)});
  float barnW = frand(12, 16), barnD = frand(16, 22);
  ```
  (red twice so red stays the most common)
- In the `makeBuilding(...)` call, replace `14, 20` with `barnW, barnD` and replace the wall colour `rgba(130, 50, 40)` with `barnC`.

## Step 3: Cabin size, colour and loft
In the `// forests` loop, the line starting `int cb = makeBuilding(p.x, p.y, 7, 6, B_CABIN,`.
- Just before that line, add:
  ```cpp
  Color cabC = pick(std::vector<Color>{rgba(110, 82, 58), rgba(92, 70, 52), rgba(128, 100, 72), rgba(84, 78, 70)});
  ```
- In the call, replace `7, 6` with `frand(6, 9), frand(5, 7)`, replace floors `1` with `chance(.25f) ? 2 : 1`,
  and replace the wall colour `rgba(110, 82, 58)` with `cabC`.

## Step 4: Roadside homesteads (new block)
Insert a new block directly AFTER the whole `// trailer parks` loop ends and BEFORE the `// pine ridge` comment.

```cpp
// roadside homesteads: ranch houses, two-stories and weathered shacks, facing the road
for (int k = 0, tries = 0; k < (int)(14 * AREA) && tries < (int)(600 * AREA); tries++) {
    // ... see the spec below
}
```

Spec for one homestead (each loop pass):
1. `Vector2 p = V2(frand(80, MAP - 80), frand(80, MAP - 80));`
   Skip (`continue`) if `townAt(p, 1.4f) >= 0` or `hotAt(p, 1.2f) >= 0`.
   `float rd = roadDist(p);` skip if `rd < 12 || rd > 30`.
2. Face the road: `Vector2 r = nearestRoadPt(p); float dx = r.x - p.x, dz = r.y - p.y;`
   `int side = fabsf(dx) > fabsf(dz) ? (dx < 0 ? 2 : 3) : (dz < 0 ? 0 : 1);`
3. Roll the kind with `float roll = frand(0, 1);`
   - `roll < .45f`: ranch house. `B_HOUSE`, `w = frand(10, 13)`, `d = frand(7, 8.5f)`, 1 floor,
     wall `pick(HOUSE_WALL)`, roof `pick(ROOF_C)`.
   - `roll < .7f`: two-story. `B_HOUSE`, `w = frand(8, 10)`, `d = frand(8, 9)`, 2 floors,
     wall `pick(HOUSE_WALL)`, roof `pick(ROOF_C)`.
   - else: shack. `B_CABIN`, `w = frand(5.5f, 6.5f)`, `d = frand(5, 5.5f)`, 1 floor,
     wall `pick(std::vector<Color>{rgba(120, 104, 84), rgba(96, 90, 82), rgba(140, 128, 106), rgba(110, 96, 80)})`,
     roof `rgba(70, 64, 60)`.
4. `int hb = makeBuilding(p.x - w / 2, p.y - d / 2, w, d, type, floors, side, -1, wall, roof);`
   If `hb < 0`, `continue` (it didn't fit).
5. Yard extras (all optional rolls):
   - `if (chance(.5f)) woodpile(V2(p.x - w / 2 - 1.5f, p.y + d / 2 - 1), -1);`
   - `if (chance(.35f)) makeBuilding(p.x + w / 2 + 3, p.y - d / 2, 3.2f, 3.4f, B_SHED, 1, side, -1, rgba(130, 110, 86), rgba(70, 64, 60));`
   - `if (chance(.3f)) car(p.x + w / 2 + 2, p.y + d / 2 + 3, frand(0, 6), .3f);`
6. Driveway: `const Building& b = blds[hb];`
   `track(b.doors.empty() ? p : b.doors[0].out, 1);`
7. `k++;`

## Step 5: Changelog
In `CHANGELOG.md`, under `## v0.8.8` (create it above `## v0.8.7` if missing):
`- Countryside: roadside ranch houses, two-stories and shacks; farmhouses, barns and cabins vary in size and colour; some cabins have a loft.`

---

## Manual test (Jorma)
1. New life, open the map, drive or walk the back roads: houses and shacks should sit 12-30 m off the road, door facing it.
2. Enter a few: two-stories have stairs up, some houses and shacks have a basement.
3. Farms: barns in different sizes and colours (red still most common), farmhouses in different sizes.
4. Forest cabins: different sizes and shades, a few with an upstairs.
5. Nothing spawns on a road or inside a town, and nothing overlaps.
