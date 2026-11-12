# PLAN: Gun pickup doesn't auto-equip (with a Settings toggle)

## Goal
Picking up a gun puts it in its slot but keeps whatever you're holding.
A Settings row "Auto-equip guns" (default Off) brings back the old behaviour.
Melee pickups are NOT changed.

## Rules for the coder
- Only touch the file and lines named in each step. Each step names its file under `src/game/`.
- Add only that step's file to Aider, e.g. `/add src/game/02_settings.h`.
- Follow the existing one-line style. No new files, no new headers, no refactors.
- Copy how the `grain` setting works; it's the template for everything here.
- Do one step, build, then stop.

Build check after every step:
```
./build.sh
```

---

## Step 1: Add the setting to the Settings struct
File: `src/game/02_settings.h` (`struct Settings { ... } cfg;`)
- Add `int autoEquip = 0;` as the LAST field, after `int grain = 1;`
- Nothing else.

## Step 2: Load and save it
File: `src/game/02_settings.h`, `loadCfg()` and `saveCfg()`
- In `saveCfg()`: append `<< " " << cfg.autoEquip` to the end of the stream, after `cfg.grain`.
- In `loadCfg()`: right after `if (!(f >> cfg.grain)) cfg.grain = 1;` add
  `if (!(f >> cfg.autoEquip)) cfg.autoEquip = 0; cfg.autoEquip = cfg.autoEquip ? 1 : 0;`
- Old config files have no value there, so the `if (!(f >> ...))` fallback is required. Do not reorder existing fields.

## Step 3: Change pickUp() for guns
File: `src/game/31_actions.h`, `static void pickUp()`, the `if (l.kind == LK_GUN)` branch
Current behaviour: sets `P.active = s; P.reload = 0;` and messages "Equipped <name>".
New behaviour:
- Keep the existing "drop the old gun from slot s if the slot is full" code exactly as is.
- Keep `P.gun[s] = l.w; P.hasGun[s] = true;`
- Then:
  - If `cfg.autoEquip` is true OR `P.active == s` (the gun in your hand was just swapped out):
    `P.active = s; P.reload = 0;` and message `"Equipped " + l.w.name`.
  - Otherwise: do NOT change `P.active` or `P.reload`. Message
    `"Picked up " + l.w.name + " (slot " + std::to_string(s + 1) + ")"`.
- Keep the same message colour `RARC[l.w.rar]` and the same sfx logic (legendary sound for rar 3).
- Do not touch the melee `else` branch.

## Step 4: Add the Settings row
File: `src/game/45_menus.h`, inside `drawSettings()`, right after the `Film grain` row
- Add:
  `if ((d = row("Auto-equip guns", cfg.autoEquip ? "On" : "Off", "Switch to a gun as soon as you pick it up. Off keeps what you're holding."))) cfg.autoEquip ^= 1;`
- After building, open Settings in-game and check the new row isn't cut off at the bottom of the panel. If it is, STOP and report back; don't resize the panel yourself.

## Step 5: Changelog
File: `CHANGELOG.md`
- Under the top unreleased version heading (create `## v0.8.8` above `## v0.8.7` if there isn't one), add:
  `- Picking up a gun no longer switches to it. Settings: "Auto-equip guns" On/Off (default Off).`

---

## Manual test (Jorma does this)
1. Fists out, pick up a pistol → still fists, message says "Picked up ... (slot N)".
2. Holding gun in slot 1, pick up another slot-1 gun → you're holding the new one (old one drops).
3. Holding slot 1, pick up a slot-2 gun → still holding slot 1.
4. Turn Auto-equip On → picking up any gun switches to it.
5. Quit, relaunch → setting is remembered. Delete the config file → defaults to Off, no crash.
