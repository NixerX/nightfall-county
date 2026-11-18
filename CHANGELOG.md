# Nightfall County changelog

## v0.8.9
- Characters move like people. Poses blend instead of snapping (swings, guards, aiming, crouching, walk to sprint). The walk is fixed: the swinging leg bends at the knee (it was the planted one) and feet no longer skate. Hips and shoulders move with the stride; standing characters breathe and shift their weight; guns bob as you walk; you lean into turns; hits shove the body the way they came.
- Zombies shamble and lurch, some drag a foot, runners flail and trip, and you can feel a brute's footsteps. Their arms hang while they wander and come up when they spot you. Badly hurt ones reel.
- Zombies can't turn on a dime: walkers are slow to come about and have to face you to attack, so juking works. Brutes are slower still; volatiles still spin.
- Bodies topple over when they die (face first, onto their back, or crumpling at the knees) instead of appearing flat.
- Blood goes where it should: streaks away from the hit, splatter and drips on walls, trails behind the wounded, bloody footprints if you walk through a pool. It dries from red to brown over a few hours.
- Every weapon kills differently. Blades take limbs off clean and can take heads; bats cave heads in; shotguns shred and can take two limbs at close range; rifles spray out the far side. Explosions roll one of several messes (shredded, top half gone, one side blown off, or a red mist) and the odd one gets a beat of slow motion.
- Flying limbs, heads and torsos are the zombie's own arm, leg, head or upper body now, not boxes.
- Zombies get bloodier where they've been hit, so you can see which ones are close to dropping.
- Shoot or chop off a leg and the zombie drops and crawls after you. Crawlers grab ankles.
- Settings: Gore (Low / Normal / Extreme). Low keeps the blood but nothing flies.
- Hordes are bigger: 22 + 12 a day (was 14 + 8).
- Zombies stuck against a fence between you climb over it (slowly, crouched) instead of grinding into it.
- Settings: Permadeath On/Off. On (default): death deletes your save and quitting always saves. Off: death offers LOAD LAST SAVE.
- Pause menu: SAVE and QUIT are separate buttons, so you can save as you go.
- Repairs go by material: rags and sewing kits fix fabric, leather patches fix leather (boots, jackets, gloves, holsters) and reinforce it once it's in good shape, duct tape patches anything up to 75%. Helmets and hard hats only take tape.
- Cut bushes with any blade for dry sticks (kindling), up to three cuts a bush. First time near a tree with an axe or a bush with a blade, a tip tells you how (chopping trees for logs, and splitting logs into kindling, were already in but never mentioned).
- Zombies that walk into a fire catch it and burn for 6-9 s (about 9 damage a second: most walkers drop, brutes come out cooked but standing). Burning zombies set others alight when they bump into them, and scorch you if they get within arm's reach (3 damage every 0.6 s, no bleeding or infection). Bloaters don't catch.
- Fixed: melee clicks got eaten. Clicking while your weapon was still recovering (or while out of breath) used up the click, so nothing happened until you let go and clicked again; in a runner fight most swings vanished. Now a click held through the recovery swings the moment you're ready, and being too winded to swing says so.
- Loot labels never overlap: the item you'd pick up keeps its spot, the rest slide to the nearest free space with a thin line back to their item.
- Footsteps sound like what you're walking on: grass, leaf litter, moss, dirt, gravel, sand, pavement, wood floors, store tile, concrete slabs and carpet upstairs. Roads sound paved, gravel or dirt by type. In the rain, outdoor steps splash (mud squelches more). Brute footfalls follow the ground too.
- Fixed: lost arms stretched skin down to the feet; the Settings BACK button was off the bottom of the screen at 720p.

## v0.8.8
- Countryside plots: shack lots, homesteads, family farms and big ranches, each sized and laid out to make sense. They face the road with a driveway and a gated, fenced yard; farms have barns, silos, tractors and fields; ranches add a machine shed and a corral. Some are kept up, some rundown, some abandoned.
- Car alarms: searching a car can set off its alarm (about 1 in 9). It wails for ten seconds and draws a big, mostly slow crowd in from out of sight. Run, or smash the car with a melee weapon to shut it up.
- Some wrecks are still smouldering. Watch for the smoke and give them room: get close and they hiss, then cook off.
- Brutes: base health 480 to 300, and they no longer get tougher as you level. The charge is telegraphed (it plants its feet and roars first), a charge that misses leaves it dazed for two seconds, and only a charge hit can break your leg (8%).
- Walls are walls: no hitting, getting hit, looting or searching cars through them. Walls also muffle noise: zombies outside hear what you do inside at less than half the distance.
- Windows: zombies can see you through unboarded windows. Stay away from the glass.
- Everything costs 30% less stamina: sprinting, swings, dodges, kicks, vaults and blocks.
- Sickness is slower: the first day it saps your stamina (down to half), then a fever slowly drains health. Antibiotics still cure it.
- Picking up a gun no longer switches to it. It goes in its slot and you keep what you're holding (unless it replaces the gun in your hand). Settings: "Auto-equip guns" On/Off, default Off.
- Click-to-move: hold Ctrl or Alt to stand your ground and attack where you click. Also stops any walk or chase in progress.
- Search car works from anywhere around the car, not just the middle of its side.
- Loot inside buildings stays hidden until you go in or peek through the door.
- The game no longer pauses when you click away from the window. Press P or Esc to pause.
- Fixed: trees growing inside buildings, fences running through barns, furniture flickering, grey interior walls showing through outside walls, chairs stuck in tables, shelves near doorways with no collision, loot lying in empty fields, medevac loot without its helicopter, and the gun hover card hiding under inventory panels.
- Saves from older versions start a new life instead of loading onto the wrong map.
- Under the hood: the game is split into src/game/ files (see src/game/MANIFEST.md) and formatted one statement per line.

## v0.8.7
- Stealth: while crouching, zombies build up a notice meter while they can see you (standing is still instant). Point-blank or hidden-and-close spots you at once. Sprinting and crouching change how fast they notice.
- Exposure colour on the HIDDEN/NOTICED status: green, yellow, orange, red, and blinking red when you're about to be spotted. A warning tick plays at orange and speeds up at red.
- Hunting time by type: walkers give up in about 2 seconds, runners 3, brutes 4, soldiers 4.5, volatiles 7. Hidden or standing still shortens it.
- Hiding indoors: crouch in a building with its doors shut to hide, unless you're in a lit spot.
- Volatiles weave in on the approach instead of charging straight in, and are less common.
- Doors: walkers barely dent them, runners are slow, brutes break them faster.
- Day birds and night crickets in the ambience beds.
- Banner says "YOU ARE AN Ex-Con" (vowel check).
- Fixed: the horde is saved and comes back when you continue. Legs animate when moving by click-to-move or autorun.
- Map: horde marker within 150 m, with heading arrow and edge pin.
- HUD: sun/moon bar under the clock, dusk or dawn countdown, horde window status.
- Smoother day/night: one eased darkness curve, continuous sun path, dawn and dusk tint fades in and out.
- Roads: soft gravel shoulder and ragged asphalt edges.
- Settings: Film grain On/Off.
- Ammo tooltips show caliber, rarity and every weapon that takes it.
- Hovering a gun in the inventory's weapon slots shows its rarity, type, damage, rate, magazine, ammo in the pack and its affixes.

## v0.8.6
- Fixed: the horde is saved and comes back when you continue (it used to vanish after a reload).
- Fixed: legs animate when moving by click-to-move or autorun.
- Map: a pulsing horde marker shows only within 150 m of you, with a heading arrow and an edge pin when it's off-screen.
- HUD: sun/moon bar under the clock, dusk or dawn countdown, and horde window status.
- Smoother day/night: one eased darkness curve, continuous sun path, dawn and dusk tint fades in and out.
- Roads: a soft gravel-and-dirt shoulder and ragged asphalt edges instead of a hard line.
- Settings: Film grain On/Off.

## v0.8.5
- Houses: interior walls can no longer seal off a room, and furniture can't block doorways or sit right behind a door. A walk test checks every building. Barns and garages no longer have a shelf behind the door. Stairs land in reachable rooms (about twice as many houses now have working upstairs/basements).
- Hayloft is a real floor: walk around up there, zombies can't follow, you heal slowly if fed. Its floor goes see-through when you're underneath.
- Flashlight is blocked by closed doors. Tent loot sits at the entrance.
- Hotbar (keys 1-0): weapons in 1-3, drag items onto it from the inventory, right-click to clear. Layout survives death.
- Inventory: bigger cells and a tinted icon for every item (game-icons.net). Gear / Skills tabs.
- Class sets (Thin Blue Line, Golden Hour, Grease Monkey, Deep Woods, County Issue): bright set colors, a glowing SET border, 2- and 4-piece bonuses, and a class bonus when the set matches your class. Set progress shows on the Gear tab and in tooltips.
- Save & Quit from the pause menu; Continue from the title screen. One slot. Dying deletes it.
- Map: wheel zoom, drag to pan, Space finds you, Home shows the whole county. Building outlines when zoomed in.
- New control mode, Click-to-move (Diablo style): click to walk, click a zombie to attack, click loot to grab it, Alt+click attacks in place.
- Crafting a splint with a broken leg straps it on. Trauma Kit fully heals a broken leg.
- Brutes break legs less often (12% per hit, was 35%).
- Antibiotics turn up in houses, kitchens, cars, barns and hunting stands.
- Uniformed zombies: paramedics, cops, firefighters, hunters, mechanics, hikers, tinted by uniform and carrying themed loot.
- Flies only buzz when you're swarmed by a crowd or a horde (removed from the daytime background).

## v0.8
- Difficulty: Easy / Normal / Hard / Nightmare in Settings. Sets zombie health, damage, speed and numbers, and they get about 7% tougher every level you gain.
- Fullscreen and resolution in Settings (1280x720 up to 4K, or native). F11 toggles fullscreen.
- Mouse wheel zooms again (wider range). Ctrl+wheel cycles weapons; 1 2 3 still work.
- Volatiles (and anything else) can no longer end up standing inside you, and a melee swing always hits something touching you whichever way you face.

## v0.7
- Fists-only start, DayZ style. Classes keep their skills and Z ability but no kit. You start in a ragged T-shirt, jeans or sweatpants, sneakers and socks, with 2 rags and an apple.
- Inventory rebuilt: clothes are storage. Pocket grids per garment, item sizes, drag and drop, rotate (R), stack merging, right-click menu, hover tooltips, a ground panel for what's around you, 10 clothing slots plus weapon slots.
- 56 kinds of clothing (hats, helmets, glasses, goggles, masks, shirts, sweaters, hoodies, jackets, coats, vests, gloves, pants, socks, boots, backpacks). Each is rolled for color, pattern, print, condition and rarity with affixes. Stats: warmth, waterproofing, bite armor, noise, visibility, storage.
- Clothes wear out when you get hit. Repair and upgrade with rags, duct tape, sewing kits, leather patches, wax and dyes. Tear clothes into rags.
- Wetness: rain soaks you through non-waterproof clothes and chills you.
- Your character model wears your clothes' colors.
- New melee: crowbar, hatchet (chops trees), pipe wrench, lead pipe.
- New items: rag, duct tape, sewing kit, leather patch, wax, three dyes, rope, jerry can.
- Loot by place: tools in sheds, garages and factories; clothes in bedrooms and store racks; uniforms at police and fire stations and the base; hunting gear at stands and barns. Zombies drop clothes.
- Roads: paved, gravel and dirt. Dirt tracks run to farms, cabins and hunting stands. Mud slows you in the rain.
- New places: sheds, grain silos, hunting stands, trailer parks, gas stations with pumps and canopies, fire stations (with a truck in the bay), schools, factories and warehouses.
- Upstairs and basements: separate interior floors reached by stairs (E). Basements are dark, hold better loot and sometimes zombies. Zombies chasing you follow you through the stairs.

## v0.6
- Map is 3.2 km across (4x the area). Forests, farms, rocks, cabins and hot zones scale with it; 8 edge roads in.
- Real vehicles (Kenney Car Kit, CC0): sedans, sports cars, hatchbacks, SUVs, vans, taxis, pickups, flatbeds, delivery vans, garbage and fire trucks, ambulances, police cars, tractors, olive military trucks. Placed by context (police at stations, ambulances at clinics and medevac crashes, tractors on farms, military at the base, checkpoints and convoys). Burned wrecks are charred with parts scattered around; others rusty and grimy.
- Vault: Space into a low fence, crate, car, rock or woodpile hops it. Space in the open still rolls. Tall walls and chain-link stay blocked.
- Controls: two modes in Settings. Aim-relative (default): W walks toward the cursor, S backs off, A/D circle-strafe. Classic: fixed compass directions. Movement accelerates smoothly; leg/body turning is smoothed and backpedal has hysteresis, so diagonals don't snap.
- Zombies have bodies: they press into you and push you, but can't stand inside you. Tap right-click to shove the one in your face (hold still blocks).
- Houses: random paint, 3-5 rooms each (living room, kitchen, bedrooms, bathroom) with doorways and furniture per room. Condition: kept / worn / abandoned, visible in paint, windows (intact, broken, boarded), yard, trash, roof moss, toppled or missing furniture, blood and bodies. Loot follows condition; abandoned houses sometimes hide a stash.
- Doors: wood or metal, random condition and strength. Different pounding sounds per material.
- Door pounding: zombies that know you're inside walk to the door and pound it (several at once, door shakes, splinters) until it breaks or they lose interest after 20-40 s. Noise inside keeps them at it. When they give up they wander off and ignore the door for a while. Fixes zombies glitching at doors.
- Version shown on the title screen and window title.

## v0.5.1
- Recorded ambience and adaptive music, material melee sounds, zombie voices; settings menu (FPS cap, shadows, foliage, volume); buildings kept off roads; flashlight blocked by walls; block, kick, plank shield; autorun; defense skills.

## v0.5
- Real ground and building textures; splint, axe, logs/kindling, campfires; doors, haylofts, hiding in grass; sweeping hordes; car searching; mouse-steered movement.

## v0.4
- Realism pass and roguelite lives: PBR textures, sun shadows, tone mapping; random class, random skill points, new map every death, hot zones.

## v0.3
- Rigged characters and real models.

## v0.2
- First 3D version.

## v0.1
- Original 2D game.
