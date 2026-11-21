# To do

## Next build (Nov 20 playtest notes)

### Audio pops and clicks
- [ ] Slight pops/clicks in the audio. Suspects to check: sounds cut off mid-play when an alias slot gets reused (voice stealing with no fade), loop beds or music volumes jumping instead of ramping, and sounds starting/stopping without a few-ms fade at the edges. Check on fry and the Xeon box: if it's only on one, look at the audio buffer size instead.

### Stairs blocking rooms
- [ ] Sometimes you can't get into a room because the stairs (up or down) sit in the doorway. Stair placement in building gen needs to keep clear of interior doorways and exterior doors (a ~1.2 m clear zone in front of each), or pick another wall when it can't.

## v0.8.9 (next build)

### Smouldering wrecks: make the warning impossible to miss
- [ ] Engine glow (orange light under the hood) plus heavier hood smoke on smouldering cars, readable from a distance.
- [ ] Louder, longer hiss when it triggers, and a flashing "!!" over the car.
- [ ] Fuse 2-3 s -> 3-4 s.
- [ ] While it's hissing, the prompt says "It's about to blow!" and the car can't be searched.
- [ ] Check the re-tint: a cooked-off car should turn into a charred shell with a fire burning on it. The red pickup in the Nov 13 playtest stayed red (might have been a nail bomb instead: Z was selected).

### Packaging
- [ ] If the assets folder can't be found, show a clear message ("Extract the whole zip first: nightfall.exe needs the assets folder next to it") instead of silently running with box cars and synth sounds. Happens when Windows runs the exe straight from the zip.
- [ ] macOS: world doesn't draw with the lit shader on Apple Silicon (debug modes NF_DBG=1..8 in place, results pending). Also the window is shorter than its drawing surface (screen 2560x967 vs render 2560x1080), which crops the HUD.

### Check
- [ ] Driveways: make sure the dirt/gravel lane from each plot's door to the road is visible (the fence gate gap is there).

### Next build batch
- [ ] Rain warning ("the rain is putting the fire out") fires for burning wrecks too: wreck fires go into the same `fires` list as campfires. Add a `bool wreck` (or `camp`) flag to `Fire` (21_state.h), set it where wrecks push their fire (35_update_world.h `fires.push_back({c.p, 90})`), and only warn for campfires. Same check anywhere else a fire is treated as a campfire (feeding prompts, warmth).
- [ ] Gas: jerry cans (item already exists, no use yet), siphon from intact cars, pour trails/pools, drop a full can as a bomb (movie rules: a gunshot sets it off), molotov (bottle + rag + gas, first crafting recipe). Ignites from flares, fires, explosions, burning zombies.
- [ ] Flashlight blocked by walls when Shadows is Off (~16 rays against walls, cut the beam in the shader).
- [ ] Windows release build without the console window (CMake WIN32 / mainCRTStartup), log to a file instead.
- [ ] ABANDON RUN (pause menu, 45_menus.h): confirm first ("Are you sure? This deletes your save"). With Permadeath Off it should just quit to the title and keep the save; only delete under Permadeath On.
- [ ] Gun noise by gun: every shot currently pulls the whole area (`noiseAt` in `fire()`, 31_actions.h). Give each gun its own noise radius (pistols medium, shotgun and rifles far, scaled by caliber), so a pistol shot doesn't empty three towns.
- [ ] Suppressors: rare loot, fit pistols and the MP5, cut a gun's noise radius a lot (a weapon affix or an attachment item).
- [ ] Crossbow and bow: near-silent ranged weapons, slow to reload, arrows/bolts can be picked back up (they stick in zombies and drop on the corpse or ground).
- [ ] Thrown knives: cheap quiet option, stack, can be picked back up.

### Z4 / Aider queue (small, one file each)
- [ ] Volatiles kill too fast on Normal: tune speed / damage / bleed chance.
- [ ] Healing: maybe drop the "healthy" bar from 45 to 30 food/water.
- [ ] Settings toggle: "Pause when unfocused" On/Off (copy the Auto-equip setting pattern).
- [ ] Backpack only shows on the character when one is equipped.
- [x] Stacked loot labels readable.
- [x] Quit auto-saves (permadeath style), Save as its own button.
- [ ] Clearer bundled font for Windows builds (pick a free font, e.g. Inter or Roboto).

## Ideas (not scheduled)
- Driving: get in intact cars (keys or hotwire), fuel, damage, engine noise draws zombies, trunk storage, zombies grab on. Needs a PLAN first.
- More plot types: hunting camp, back-road gas station, church with graveyard.
- Windows, more: shoot out through them, zombies smash in through them.
