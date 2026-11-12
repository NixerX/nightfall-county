# src/game manifest

The game is one translation unit: `src/main.cpp` includes these files once, in number order. A file can use anything from a lower-numbered file, never a higher one (except through a forward declaration). New code goes in the file for its system; a new system gets a new numbered file at the right spot in `main.cpp`'s include list.

When working with a local model, give it only the file(s) named here for the job.

## Foundation

| File | Domain | What's in it |
|---|---|---|
| `01_util.h` | Helpers | Random numbers (`frand`, `irand`, `chance`), vector shortcuts (`V2`, `V3`, `polar`), colour helpers (`rgba`, `lerpC`), the `VERSION` string |
| `02_settings.h` | Settings | `Settings cfg` struct, `loadCfg`/`saveCfg` to `~/.nightfall3d_settings`. New setting = field here + load/save here + a row in `45_menus.h` |
| `03_shaders.h` | Graphics | GLSL source for the lit, skinned, fog and night shaders |
| `04_render_resources.h` | Graphics | Shared meshes, the 3D camera (`cam3`), `initRender`, basic box/mesh drawing |
| `05_surface_textures.h` | Graphics | Ground and wall textures: photo PBR sets plus procedural height/normal maps, the ground atlas |
| `06_gltf_assets.h` | Models | Loading the Quaternius glTF models, `drawAsset`, character looks and palettes, `drawCharacter` |

## World

| File | Domain | What's in it |
|---|---|---|
| `07_world_data.h` | World data | Core world structs (`Building`, `Door`, `Tree`, `Road`, `Town`, `Prop`), collision (`collide`, `freeAt`, `losClear`), spatial lookups, hot zones |
| `08_ground_paint.h` | Terrain | Painting the terrain splat map (grass, dirt, gravel...) and baked ground shadows |
| `09_building_parts.h` | Buildings | Wall segments, furniture placement, doors, woodpiles, house colours, loot table ids |
| `10_buildings.h` | Buildings | `makeBuilding`: builds every building type (houses, stores, police, barns...) with rooms, furniture and loot spots |
| `11_wrecks.h` | World props | Car wrecks, trees, roads, fences, tracks |
| `12_towns.h` | World gen | Fixed-seed town layouts (same every run) |
| `13_life_map.h` | World gen | Per-life randomisation: town positions, crates, tents, hot zones |
| `14_interiors.h` | World gen | Upstairs and basement "cells", stairs between them |
| `15_gen_world.h` | World gen | `genWorld`: assembles the whole map, including the random countryside |
| `16_static_merge.h` | Performance | Merges static props into one mesh per 32 m chunk |

## Things and people

| File | Domain | What's in it |
|---|---|---|
| `17_items.h` | Items | Item ids and defs, calibers, gun bases and affixes (`makeWeapon`), melee weapons |
| `18_clothing.h` | Clothing | Clothing slots, defs, rarity, warmth/armour/rain stats, class sets, loot kinds and item model lookups |
| `19_entities.h` | Entities | Zombie types and structs, corpses, particles, gibs, bullets, bombs, decals, lights |
| `20_player.h` | Player | `Player` struct and `Perks` |
| `21_state.h` | Game state | Game state enum, horde, fires, banners and messages (`msg`), day/night clock, XP curve |
| `22_audio.h` | Audio | Synthesised sound effects, `sfx`/`sfxAt` |
| `23_inventory.h` | Inventory | Pocket grids, `invAdd`/`invTake`/`invFind`, gear totals, clothing wear, starting clothes |

## Gameplay

| File | Domain | What's in it |
|---|---|---|
| `24_effects.h` | Effects | Floating text, blood decals and spray, gibs, screen gore |
| `25_spawning.h` | Spawning | Difficulty scaling, zombie spawning and type picks |
| `26_loot_gen.h` | Loot | What spawns where (`genLoot`, loot tables), zombie drops |
| `27_combat.h` | Combat | Damage to player and zombies, kills, dismemberment, explosions, XP, noise |
| `28_death.h` | Death | Player death, best-run record |
| `29_classes.h` | Classes | Class defs, random skill trees, perk recalculation, class actives |
| `30_new_game.h` | New game | `newGame`: resets the player and world for a fresh life |
| `31_actions.h` | Player actions | Aiming, reload, weapon switch, dodge/vault, shove/kick, melee, `fire`, throwing, light, fires, splints, using items, repairing clothes, `pickUp` |
| `32_interaction.h` | Interaction | The E key: finds the nearest action (loot, doors, ladders, fires, cars) and does it |

## Update loop

| File | Domain | What's in it |
|---|---|---|
| `33_update_player.h` | Update | `updatePlayer`: movement, controls, stamina, survival stats |
| `34_zombie_ai.h` | Zombie AI | Detection and stealth tuning per type, hunting, doors, `updateZombies` |
| `35_update_world.h` | Update | Bullets, world updates (fires, horde, weather, effects), and `update`, the per-frame entry point |

## Rendering

| File | Domain | What's in it |
|---|---|---|
| `36_render_entities.h` | Rendering | Billboards, the box humanoid, gibs, lighting setup and fades |
| `37_shadows.h` | Rendering | Sun and flashlight shadow maps |
| `38_render_world.h` | Rendering | `renderWorld3D`: draws the whole 3D scene each frame |

## UI

| File | Domain | What's in it |
|---|---|---|
| `39_hud.h` | UI basics | `text`, bars, buttons, icons, cursor |
| `40_hotbar.h` | HUD | Hotbar (keys 1-0) and the in-game HUD (`drawHUD`) |
| `41_inventory_ui.h` | Inventory UI | Drag state, item boxes, tooltips, gun hover card, tabs |
| `42_inventory_draw.h` | Inventory UI | `drawInventory`: paper doll, weapon slots, pockets, ground, drag and drop |
| `43_map.h` | Map | `drawMap` |
| `44_save_load.h` | Saving | Run save file: writing and reading the player, items, clothes and guns |
| `45_menus.h` | Menus | Title, skills, pause, Settings (`drawSettings`), death screen, display mode |
| `46_autotest.h` | Testing | Scripted autotest run, only built with `-DNF_AUTOTEST` |
| `47_ambience.h` | Audio | Looping ambience beds and adaptive music volumes |

`main()` (window setup, the main loop, state switching) stays in `src/main.cpp`.

## Where to look

- New setting: `02_settings.h` + `45_menus.h`
- Weapon stats or affixes: `17_items.h`; what drops where: `26_loot_gen.h`
- Zombie behaviour or difficulty: `34_zombie_ai.h`, `25_spawning.h`, `19_entities.h`
- Damage and gore: `27_combat.h`
- Controls and moves: `33_update_player.h`, `31_actions.h`
- Inventory screen: `41_inventory_ui.h`, `42_inventory_draw.h`
- HUD: `40_hotbar.h`
- Save format: `44_save_load.h` (changing it breaks old saves unless you handle both)
