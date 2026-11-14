# PLAN: countryside plots

Approved direction (Nov 2026). Not built yet.

Replaces the `// farms (random)` and `// roadside homesteads` blocks in `src/game/15_gen_world.h`. Countryside buildings stop being dropped at fixed offsets and are laid out on **plots**: a plot claims its land first, then fills it from a template that scales with its size and faces the road. Layouts stay coherent (the house fronts the road, the barn sits behind it, the yard is fenced with a gate where the drive goes in), but every plot rolls its own size, optional pieces and look.

World gen changes, so bump `SAVE_VER` in `44_save_load.h`.

## How a plot is made

1. **Pick a type and roll its size** (table below). Plot is a rectangle `W x D`; "front" is the side facing the road.
2. **Find land:**
   - Pick a random point 80 m in from the map edge.
   - Reject if in or near a town (`townAt`), a hot zone (`hotAt`), or on a road (`rectOnRoad`).
   - The front edge has to be between the type's min and max distance from a road (`roadDist`).
   - Reject if `boxFree` fails for the whole rectangle plus a 4 m margin, so plots never overlap each other, forests, or anything else.
3. **Face the road:**
   - `nearestRoadPt` gives the road direction.
   - Snap it to one of 4 sides (N/S/E/W), since buildings are axis-aligned.
   - Lay the template out in "plot space" (front = +v, right = +u), then convert each slot to world space with that side. Mirror left/right on a coin flip.
4. **Reserve it:** keep the plot rectangle in a local list so later plots in this pass skip it.
5. **Place slots** in order: main house, then outbuildings, then props. Each slot is a rectangle in plot space as fractions of `W`/`D` plus an absolute minimum size.
   - A slot that fails `makeBuilding` (returns -1) is skipped. The plot still stands; an optional slot just doesn't appear.
   - **The main house must succeed.** If it doesn't, abandon the plot (nothing else placed) and try again elsewhere.
6. **Driveway and gate:** `track()` from the house door straight out through the front edge to the road. The gate gap in the fence goes where the drive crosses the front edge.
7. **Fence** the plot edge (see Condition), leaving a 3.2 m gate gap at the drive.
   - `fence()` already skips roads and buildings, so it can't cut through anything.
8. **Fields** (farms and ranches only): paint 2-4 field patches next to the plot (outside it, behind or to the sides), using the existing field-painting code.

## Condition

Roll once per plot. It drives both the look and the fence:

| Condition | Chance | Fence | Feel |
|---|---|---|---|
| Kept | 45% | Full fence, all sides, gate at drive | Tidy, paint fine |
| Rundown | 35% | Fence with 1-3 random missing runs | Some junk, a wreck in the yard |
| Abandoned | 20% | Only scraps (1-2 short runs) or none | Junk, wrecks, overgrown |

Building condition (paint wear, broken windows, looted state) leans the same way, using whatever condition knobs `makeBuilding` already has.

## Plot types

| Plot | Size W x D | Front from road | Per map (x AREA) | Slots |
|---|---|---|---|---|
| **Shack lot** | 18-25 x 18-25 | 6-25 m | 6 | Shack (B_CABIN, 5.5-6.5 x 5-5.5, weathered colours); woodpile 70%; outhouse/shed (B_SHED small) 50%; junk car 40%. Fence: half the time none at all, regardless of condition. |
| **Homestead** | 25-35 x 25-35 | 8-30 m | 10 | House: ranch (10-13 x 7-8.5, 1 floor) 60% or two-story (8-10 x 8-9) 40%, front third of the plot; shed 60% back corner; garden patch (painted dirt rows) 40% side; woodpile 60%; car in the drive 50%. |
| **Family farm** | 40-55 x 40-55 | 10-40 m | 8 | House (8-12 x 7-10, 55% two-story) front left; barn (12-16 x 16-22, five colours, red most common) back right; shed 60% back left; 0-2 silos beside the barn; tractor 70% by the barn; woodpile 60%; fields. |
| **Big ranch** | 60-80 x 55-75 | 15-60 m | 3 | Two-story house (10-13 x 9-11) front centre; big barn (16-20 x 20-26) back left; machine shed (B_GARAGE or B_BARN, 12-16 x 10-12) back right; 2-3 silos; corral (fenced 15-20 m square, 1.4 m high) to one side; tractor; long gravel drive (tier 2); fields on several sides. |

Rules for slots:

- **Scale:** slot rectangles are in plot fractions, so a bigger plot spreads things out.
- **Spacing:** building sizes roll within their own ranges, clamped so there's at least 3 m between buildings and 2 m to the fence.
- **Colours:** house uses `HOUSE_WALL` / `ROOF_C`; barns use the 5-colour barn list; sheds and shacks use weathered woods.
- **Interiors:** basements, upstairs, rooms and loot keep working as they do now. That's all inside `makeBuilding`.

## Build order (one file: `15_gen_world.h`)

1. Add a small `struct PlotSlot` and the four templates as data near the top of `genWorld()` (or just above it).
2. Add `static bool makePlot(int type)` implementing "How a plot is made" (steps 1-8). Returns false if it couldn't place.
3. Replace the `// farms (random)` loop with plot loops: try each type `count x AREA` times (with a try cap, e.g. 40 per wanted plot).
4. Delete the `// roadside homesteads` block (now covered by Shack lot and Homestead).
5. Bump `SAVE_VER`.
6. Changelog.

Test: start new lives and fly the map. No overlaps, every house faces a road, drives connect to roads, gates line up with drives, sizes vary, and abandoned plots look abandoned.

## Open questions (defaults used above)

- Extra plot types later: hunting camp, back-road gas station, church with graveyard.
- Counts per map are a first guess (27 x AREA total vs about 24 today). Tune after playing.

## Changelog line (when built)

- The countryside is laid out properly: shack lots, homesteads, family farms and big ranches, each sized and arranged to make sense, facing the road with a driveway and a fenced yard. Some are kept up, some rundown, some abandoned.
