# Nightfall County 3D (v0.8.5)

See `CHANGELOG.md` for what changed in each version.

Top-down 3D open-world zombie roguelite. C++17 + raylib 5.5. Models by Quaternius (CC0), surface textures from ambientCG (CC0), see `assets/CREDITS.txt`.

## Build (Debian 13 / Ubuntu)

```
./build.sh
./build/nightfall
```

`build.sh` installs the X11/GL dev headers if missing, fetches raylib 5.5 from GitHub and builds a release binary. The build copies `assets/` next to the binary, so run it from anywhere. Runs on KDE Wayland via XWayland. F11 is borderless fullscreen, F10 mutes.

## Look

- Real photo-scanned terrain: 12 ground layers (short and long grass, forest floor, leaf litter, packed earth, gravel, dark field soil, concrete, rubble) blended by where you are, with baked contact shadows.
- Real photo-scanned PBR surfaces (color, normal and roughness maps): asphalt roads that turn to wet asphalt in the rain, concrete sidewalks, brick and dirty plaster walls, rusted sheet metal, hospital tile in clinics and stores, clay roof tiles, hardwood floors, log cabin walls, barn wood, plywood boarded windows, chain-link at the base. Siding, shingles, planks and log walls get generated normal maps.
- Sun and moon shadow maps: buildings, trees, cars and every zombie cast real shadows that sweep across the day.
- Baked contact shadows where walls, cars and crates meet the ground, darkening at the base of walls, filmic (ACES) tone mapping with a slightly drained grade.
- Wet streets: puddles go glossy, and streetlights and your flashlight streak across the road.
- Characters are rigged Quaternius people animated in code. Zombies are the same townspeople rotted. Dismemberment and headshots act on the skeleton.
- Furnished interiors, real trees, rocks, grass, streetlights, signs and trash cans.

If the `assets` folder is missing, the game falls back to the old blocky models and generated textures.

## Every life is different

Dying ends that character and that map.

- **New map, 3.2 km across.** The six towns (Cedar Falls, Harlow, Millbrook, Pine Ridge, Dunmore, Fort Calloway) keep their street layouts but land somewhere new. Highways are rebuilt to connect them, the edge roads you wander in from move, and forests, farms, wrecks, cabins and all loot are rerolled.
- **Hot zones.** 4 to 6 per map, marked in red on the map: Quarantine Checkpoint, Crashed Medevac, Burned Convoy, Mass Grave, Overrun Evac Point. About 2.5x the zombies, more runners, brutes and soldiers, bodies everywhere, and guaranteed caches with Magic-or-better guns, ammo, pipe bombs and meds.
- **Random class.** Police Officer, Paramedic, Mechanic, Hunter or Ex-Con, each with a class skill on **Z** and three 4-skill trees. No starting kit: everyone wakes up with fists, a ragged T-shirt, jeans or sweatpants, sneakers, socks, two rags and an apple.
- **You don't spend skill points.** You start with 2, every level adds 1, and fate drops each one into a skill you've unlocked (a skill needs a point in the one above it). **K** shows where they landed.

| Class | Z skill | Trees |
|---|---|---|
| Police Officer | Adrenaline Dump: 6s of +60% fire rate, instant reloads | Marksman, Patrol, Riot |
| Paramedic | Trauma Kit: heal 40, stop all bleeding | Triage, Pharmacist, Survivor |
| Mechanic | Nail Bomb: free pipe bomb | Demolition, Grease Monkey, Wrench |
| Hunter | Deadeye: 8s of guaranteed headshots | Rifleman, Tracker, Woodsman |
| Ex-Con | Bloodlust: 8s of double melee damage, hits heal | Butcher, Street, Hard Time |

## Controls

| Key | Action |
|---|---|
| WASD | Move. Aim-relative (default): W toward the cursor, S back, A/D circle-strafe. Classic (Settings): fixed compass directions. Backpedal 70%, strafe 85% |
| Mouse / LMB | Aim / shoot (or swing if no gun out) |
| Shift / C | Sprint / crouch (crouch = quieter, harder to see) |
| Right mouse | Tap: shove the one in your face. Hold: block. Frontal hits mostly stopped, costs stamina; a shield stops nearly all of it |
| Q | Kick: shoves and staggers what's in front of you, boots closed doors open |
| X | Autorun (any move key, crouch, roll, a wall, or being spotted stops it) |
| Space | Vault a low fence/crate/car/rock in front of you; otherwise dodge roll |
| V / MMB | Melee |
| R | Reload (pulls rounds of the right caliber from your backpack) |
| E | Pick up / open and close doors / climb hayloft ladders / search cars / feed a fire |
| B / N | Quick bandage / best meds for what's wrong (splint, morphine, antibiotics, painkillers) |
| 1-0 | Hotbar (weapons in 1-3 by default) |
| Ctrl+wheel | Cycle weapons |
| Mouse wheel | Zoom |
| F11 | Fullscreen |
| Tab | Inventory (see below) |
| F / U | Flashlight / UV mode |
| G / H | Pipe bomb / road flare (both lure the dead) |
| M | Map |
| Z | Class skill |
| K | Skill trees (view only) |
| Ctrl + wheel | Zoom |
| P / Esc | Pause |

The world keeps running while the backpack, map or skill screen is open.

## Survival

- **Hunger and thirst** drain over time, faster while sprinting. At zero they eat your health.
- **Bleeding**: zombie hits can open wounds, each one draining health. Bandages close them.
- **Sickness**: bloaters and bites can infect you. Antibiotics cure it.
- **Broken leg**: brute charges can break it, so no sprinting or rolling. Morphine fixes it.
- **Temperature**: night, rain and being outside cool you down; buildings and fires warm you. Cold makes your aim shake and eventually hurts.
- Health only regenerates while you're fed, hydrated, warm, healthy and not bleeding.
- Your flashlight makes you visible from much farther at night, and the battery runs down.

## Inventory and clothing

DayZ rules: **your clothes are your storage.** Shirts, pants, jackets and backpacks each have a pocket grid (a T-shirt is 2x2, jeans 2x3, a military jacket 4x3, a military pack 6x6). Items take up space (a can is 1x2, a log 2x3). Lose the jacket and you lose what's in it.

- **Tab** opens it: what you're wearing on the left, your pockets in the middle, the ground around you and crafting on the right.
- **Drag** items between pockets, onto the ground to drop them, or from the ground into a pocket. **R** while dragging rotates. Drop a stack on a matching stack to merge.
- Drag clothing onto its slot to wear it (the old piece goes into a pocket or onto the ground). Right-click for use / wear / drop. Shift-click drops.
- **Slots:** head, eyes, face, shirt, jacket, hands, legs, socks, feet, back. Guns still go in primary / sidearm, melee in its own slot.
- Every piece is rolled: color, pattern (plaid, camo, faded), sometimes a print ("Harlow High Football"), condition from Pristine to Ruined, and a rarity (Common, Uncommon, Rare, Legendary) that adds affixes: Insulated, Reinforced, Waterproofed, Deep-Pocket (extra rows), Quiet.
- **Stats:** warmth, waterproofing, bite armor (less damage, fewer bleeds), noise (rain jackets rustle, sneakers are quiet) and visibility (blaze orange is a target, camo hides you). Hover anything for the numbers.
- **Wear and tear:** hits shred the outer layers first. A ruined jacket keeps almost no warmth or armor.
- **Repair and upgrade:** drag the tool onto the clothing, or right-click it and pick the target. Rag (patch up to Worn), duct tape (up to Worn), sewing kit (near new), leather patch (+bite armor, up to 3), wax (waterproofing), black / olive / brown dye (darker, harder to spot).
- **Wet:** rain soaks you unless your clothes are waterproof; wet clothes make you cold. Dry off inside or by a fire.
- **Rags:** drag a piece of clothing onto the Craft panel to tear it into rags. A rag stops bleeding (not always) and works in a splint instead of a bandage.
- Your character's shirt, pants and shoes change color to match what you're wearing.

## Settings

Pause (Esc) → Settings, or Settings on the title screen. Difficulty (Easy to Nightmare; zombies also scale with your level), fullscreen and resolution, control mode, frame-rate cap (30/60/90/144/unlimited), shadows (off / low / high), foliage density (applies to the next map), master/effects/music volume. Saved to `~/.nightfall3d_settings`. On a weak machine: 60 FPS cap, shadows Low or Off, foliage Low.

## Sound

Recorded sounds from the Sonniss GDC 2026 bundle and Kenney (see credits): weapon hits by material (bat thunks wood, blades slice, axe chops, fists punch), zombie groans, attack snarls, screams, brute roars and death rattles, gore. Ambience follows the world: meadow insects by day, crickets at night, rain outside or drumming on the roof when you're in, wind, storms with thunder, fires crackling, dogs in town. Adaptive music: a dark tension pad that swells as anything that's noticed you closes in, a heartbeat when one's on top of you or you're nearly dead, war drums when a horde is near, a ghostly layer when volatiles hunt at night. Guns are still synthesized until we find real gunshot recordings.

## Survival tools

- **Axe or hatchet:** swing at a tree to chop it. Every 4 hits drops a log, 3 logs per tree. Found at woodpiles, sheds, garages, barns, basements and fire stations.
- **Other melee:** knife, machete, bat, crowbar, pipe wrench, lead pipe. Each hits with its own sound.
- **Logs and kindling:** with an axe or hatchet in hand, right-click a log in your pockets to split it into 4 kindling.
- **Plank shield:** 1 log + 3 kindling + rope or duct tape (Craft panel; again to repair). Hold right-click to block. Wears down as it takes hits.
- **Splint:** 2 kindling + 1 bandage or rag (Craft panel). On a broken leg you go from 45% speed to 52% right away, 70% after 12 in-game hours, then fully healed over the next 2 days. Morphine is still the instant fix. No sprinting or rolling until the leg's healed.
- **Campfire:** 3 kindling + 1 match, outdoors only. Warms you, lights the night, and draws the dead from about 40 m at night. Feed it kindling or logs with E. Rain kills it in seconds unless it's under a tree canopy.

## Hiding

- **Doors** open and close with E. A closed door blocks sight and movement. Zombies that saw you go in will beat it down (brutes fast); ones that didn't will wander off.
- **Barn haylofts:** climb the ladder with E. The dead can't climb. They only notice you up there if you fire a gun.
- **Tall grass and bushes:** crouch in them with your light off. Still, you're nearly invisible; crouch-walking, much harder to spot. A straggler that walks right into you still finds you.
- **Hordes** cross the map along a path instead of homing in on you. If one spots you, the pack hunts where it last saw you, searches a while, then gives up and moves on.

## Cars

Every wreck can be searched once with E: glovebox and trunk. Ammo, water, snacks, flares, meds, batteries, matches, now and then a pistol or an axe. Burned wrecks are mostly ash.

## Places

- **Roads:** paved highways and town streets, gravel village streets and back roads, dirt tracks out to farms, cabins and hunting stands. Dirt turns to mud in the rain and slows you down.
- **Buildings:** houses (some two-story, some with basements), sheds in back yards and on farms, garages, barns with haylofts, grain silos, cabins, trailer parks, gas stations with pump islands, stores, clinics, police and fire stations, schools, factories and warehouses on the edge of the city, churches, the military base.
- **Upstairs and basements:** take the stairs with E. Upstairs is bedrooms and a bath. Basements are pitch black (bring a light), have better loot (tools, canned food, guns, ammo, fuel) and sometimes something already living down there. Zombies chasing you follow you up or down a few seconds later.

## Loot

Loot follows the place and what makes sense: tools, tape, rope, wax and leather in sheds, garages and factories; clothes in closets, dressers and store racks; uniforms and gear in police and fire stations and the base; hunting gear at hunting stands and barns; snacks and fuel at gas stations; backpacks at schools. Zombies sometimes drop the clothes they died in (mostly wrecked). Rarer stuff is rarer, and it gets a little better the longer you live.

Guns use real calibers (9mm, 12ga, 5.56, .308) and keep the Diablo rarity tiers (Common / Magic / Rare / Legendary) with random affixes.

## Gore

Blood, corpses and body parts stay on the map for the run. Limbs come off, headshots pop heads, and overkill (point-blank shotgun, deer rifle, pipe bomb) blows bodies apart.

## Coming next

Drivable, fixable vehicles (parts and fuel; jerry cans already turn up), real gunshot recordings, better trees, runners that vault fences.

## Code layout

`src/main.cpp` holds the includes and `main()`. The rest of the game lives in `src/game/NN_name.h`, one file per system (settings, buildings, loot, zombie AI, rendering, menus, and so on). They're not standalone headers: `main.cpp` includes them once, in number order, so it's still one translation unit and a file can use anything from a lower-numbered file. To change one system, open just that file. New code goes in the file for its system; a new system gets a new numbered file in the right spot in the include list.
