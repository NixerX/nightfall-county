# PLAN: blood, visible damage and gore variety

Proposed (Nov 2026). Not built yet.

## Where it stands

- **Every explosion kill looks the same.** `gibZombie` always throws one head, both arms, two legs, 5-9 chunks, 3 ribs and one guts pile.
- **Every headshot looks the same.** 7 chunks, 1 bone, one spray.
- **Gibs are coloured boxes.** A flying arm is a skin-coloured cube.
- **Only arms come off.** Legs and heads (short of a kill) never do.
- **Weapons don't matter.** A machete, a bat, a shotgun and a rifle all spray the same way. Only damage size picks "gib or not".
- **Blood that hits a wall vanishes** (`35_update_world.h` kills the droplet).
- **Ground splats are 4 round blobs,** whatever direction the hit came from.
- **Zombies show no damage.** One that's been shot five times looks the same as a fresh one.

## 1. Blood spatter

- **Directional splats:** 4 new streak shapes in the blood atlas. A hit lays a streak pointing away from you, and its length scales with damage.
- **Wall splatter:** droplets that hit a wall leave a splat on the wall at that height, and the bigger ones run into short drips. They go on their own list (cap ~1500) so they don't eat the floor decals.
- **Spray by weapon:**
  - Pistol: small puff.
  - Rifle: exit spray out the far side.
  - Shotgun up close: wide cone.
  - Blade: an arc along the swing.
  - Blunt: a short burst plus a splat at their feet.
- **Trails:** bleeding zombies (`z.bleed`) and a bleeding player drip a trail as they move.
- **Bloody footprints:** walk through a fresh pool and you leave prints for about 12 steps.
- **Blood dries:** fresh red darkens to brown-black over a few in-game hours. Rain fades it faster.

## 2. Visible damage

- **Zombie wounds:** each hit stains that body zone (head, torso or legs) darker red, and a badly hurt zombie is soaked. You can read roughly how close one is to dropping.
- **Player:** clothes pick up blood as you take hits (and it stays until you change or repair the item).
- **Hurt zombies move worse:** under 30% HP they stagger and drag (ties into PLAN-anim).

## 3. Dismemberment

Hits now land on a zone: head, torso, arm or leg. The roll is weighted by weapon, since the game is top-down and there's no real aim height.

- **Arms:** off at the shoulder or the elbow (forearm stub).
- **Legs:** ⚠️ **gameplay change.** Lose one and the zombie falls and **crawls** at about 30% speed, still coming for you, lower to the ground. Crawlers can grab your ankles if you step over them.
- **Heads:**
  - Pop (now).
  - Clean decapitation: the head flies, the body takes a step and drops. Blades only.
  - Caved in: blunt weapons, no flying parts, big pool.
- **Weapons:**
  - Blades sever cleanly, with a spurt from the stump.
  - Blunt weapons crush but rarely sever.
  - Shotguns up close can take several parts at once.
  - Rifles punch through.
  - Explosives do the full gib.

## 4. Gib variety

- **Real body parts:** a flying arm or leg is the zombie's own model, drawn with every bone except that limb collapsed (the pose system already supports this, `S[bone] = 0`). Parts match the clothes and skin of the zombie they came off. Capped at ~40 model gibs at once, with the box chunks still used beyond that.
- **Explosion patterns:** each gib kill rolls one:
  - Full shred (now, but with rolled part counts).
  - Top half gone: the legs stand a beat, then fold.
  - Split: one side blown off, the rest falls.
  - Red mist (rare): mostly mist and small chunks, almost nothing left.
- **Everything rolls:** part counts, sizes, spin, speed, spray and colour shade. No two kills look the same.
- **Landing:** gibs bounce, roll and leave a smear where they land.
- **Rare big kill (~5%):** double spray, more parts, a slow-mo beat of 0.15 s.

## 5. Gore setting

Settings → Gore: **Low / Normal / Extreme**. Low keeps the blood but no flying parts, and Extreme raises the caps and adds more of everything. It's useful for streaming, or if a laptop chokes.

## Files

- `24_effects.h`: streak and wall decals, trails, footprints, drying, gib patterns.
- `04_render_resources.h`: streak shapes in the blood atlas.
- `19_entities.h`: wound amounts per zone, leg flags, crawler state, model-gib fields, wall decal struct.
- `27_combat.h`: hit zones, weapon styles, dismember and kill variants.
- `06_gltf_assets.h` / `poses.h`: zone wound tint in `drawCharacter`, crawl pose, limb-only gib draw.
- `34_zombie_ai.h`: crawlers (speed, height, ankle grab).
- `35_update_world.h`: droplets hitting walls, gib landing.
- `38_render_world.h`: wall decals, model gibs.
- `02_settings.h` / `45_menus.h`: gore setting.
- `44_save_load.h`: nothing. Gore isn't saved.

## Order

1. Spatter: streaks, walls, weapon spray, trails.
2. Gib variety plus real body parts.
3. Wounds, zones, dismemberment and crawlers.
4. Gore setting, drying, footprints.

## Changelog lines (when built)

- Blood goes where it should: streaks away from the hit, splatter and drips on walls, trails behind the wounded, and footprints if you walk through it. It dries over time.
- Every weapon kills differently. Blades take limbs clean off, bats cave heads in, shotguns shred, and no two explosions leave the same mess. Flying limbs are the actual zombie's arm or leg now.
- Zombies show their wounds, and shooting out a leg leaves a crawler that's still coming for you.
- New Gore setting: Low, Normal, Extreme.
