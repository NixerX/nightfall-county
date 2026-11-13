# PLAN: car alarms and wrecks that cook off

Approved design (Nov 2026). Not built yet.

## 1. Car alarms

When you search a regular (not burned) car:

- **11% chance** the alarm goes off.
- **Sound:** a new synthesized "whoop-whoop" alarm (`SFX_CAR_ALARM` in `22_audio.h`), looping for about **10 s**. The existing `SFX_HORN` is a long low blast, so it doesn't fit.
- **Shutting it off:** hitting the car with a melee weapon kills the alarm early.
- **Lure:** while it's sounding, every zombie within about **40 m** is pulled to the car (`noiseAt`). A normal search only alerts 8 m.
- **Horde:** spawns **12-16, plus 1 per in-game day, capped at 22**.
  - About **85% walkers**, the rest runners. A few more runners at night.
  - The odd **bloater after day 2**. No brutes or volatiles.
  - They fan in from **one direction**, 30-45 m out, so there's always a way to run. The point is to make you think about running first.
- **Message:** "Car alarm! Get moving."

Files: `22_audio.h` (new sound), `32_interaction.h` (the A_CAR search), `25_spawning.h` / `35_update_world.h` (alarm timer and horde spawn), `31_actions.h` (melee hit on a car kills the alarm).

## 2. Wrecks that cook off

- At map generation, about **1 in 12 intact wrecks** are flagged "live" (realism: a burned-out car has nothing left to burn).
- A live wreck **smoulders**: a thin smoke plume is the visual tell.
- When the player first comes within about **25 m** (roughly on screen in the top-down view), it starts a **2-3 s warning**: thicker smoke and a hiss or crackle. Then it explodes with the existing `explode()` from `27_combat.h`: fireball, smoke, flash, scorch mark, shake, boom.
- **Damage only within about 5 m**, and the warning is long enough that it's never a cheap death.
- **Afterwards:** the car becomes a burned-out wreck (charred tint, `burned = true`) with a fire left burning on it for a while.
  - The fiddly part: `CarObj` doesn't remember which props are its model, so store the prop index range when the car is built (`11_wrecks.h`).
- **Zombie magnet:** keep the explosion's big lure (pipe bomb is 160 m) for chaos. Tune it down (e.g. 60-100 m) if it drags too much in.

Files: `07_world_data.h` (CarObj: live flag, prop range, timer), `11_wrecks.h` (flag and store props at build), `35_update_world.h` (proximity trigger, smoke, warning, boom, re-tint), `22_audio.h` (hiss).

## Changelog lines (when built)

- Car alarms: searching a car can set off its alarm and draw a crowd. Smash it to shut it up, or run.
- Some wrecks are still smouldering. Give them room: they can cook off.
