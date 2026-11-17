# PLAN: more lifelike character movement

Proposed (Nov 2026). Not built yet.

## Where it stands

Every character is posed by code (`src/poses.h`, `buildPose`), not by canned animations. That's good: it means everything below is tunable math, no new assets.

What makes it look robotic today:

1. **Poses snap.** Each frame the pose is computed from scratch. Swing ends, guard drops, crouch, roll, gun up/down, walk to sprint: all of them pop from one pose to the next in a single frame.
2. **Zombies turn instantly.** `z.a = ang` every frame, so they pivot on a dime like turrets.
3. **Zombies freeze mid-stride** when they stop (banging a door, waiting). There is no idle pose for them; their legs hold wherever the walk cycle was.
4. **Legs run at full speed while the body is still accelerating.** The player's walk cycle advances from the target speed, not the real speed, so the feet skate on start and stop.
5. **Walk to sprint is a hard switch** at `move > 1.2` (stride .45 to .8, lean .05 to .22).
6. **Only the legs and arms move.** No hip sway, no shoulders turning against the hips, no breathing, no weight shift when standing.
7. **Death is instant.** A killed zombie becomes a corpse already lying flat.

## The work, in three passes

### Pass 1: smoothness (biggest win, no gameplay change)

- **Pose blending.** Each character keeps its last pose and eases toward the new one (slerp per bone, about 12-18/s; faster for attacks so hits still read instantly). This alone kills every pop listed above.
  - Storage: a small pose buffer on `Zombie` and `Player`, reused by the shadow pass so shadows don't double-step it (`37_shadows.h` currently rebuilds the pose separately).
- **Real-speed walk cycle.** Advance `P.step` from actual velocity (`P.vel`), and blend stride by speed, so the feet plant instead of skating. Zombies already use distance walked, so they're mostly fine.
- **Smooth walk to jog to sprint.** Stride, lean, arm swing and bob all scale continuously with speed instead of two fixed settings.

### Pass 2: body life (no gameplay change)

- **Hips and shoulders:** hips rotate with the stride and shoulders counter-rotate, the way people actually walk. Small side-to-side hip sway.
- **Idle:** breathing (chest rise), slow weight shift between feet, head looking around a little. Zombies get their own idle: swaying, head lolling, arms hanging.
- **Feet:** heel strike and toe-off through the step instead of flat-footed swinging.
- **Arms with guns:** the gun bobs and settles with the stride instead of being locked rigid; it kicks up and recovers on recoil.
- **Lean into turns** when running.
- **Hit reactions** knock the body away from the hit direction (currently it's always backwards).

### Pass 3: zombie character (some gameplay change, flagged)

- **Death falls:** a 0.4-0.6 s topple in the direction of the killing blow, then the corpse stays put. Headshots drop straight down.
- **Shamble:** walkers lurch, with speed surging on each step and a dragged foot on some of them (per-zombie seed, so each one walks a bit differently).
- **Reach when they see you:** arms hang while wandering and come up when they spot you, so you can read who's locked on.
- **Runner flail:** looser, more chaotic arms, a stumble now and then.
- **Brute weight:** slower, heavier steps with a bigger body roll and screen thump near you.
- **Turn rates** (⚠️ this one changes the game): walkers turn slowly (about 2.5 rad/s), runners quicker, brutes slowest. Juking a walker becomes a real move, and a brute that charges past you takes time to come around (fits the miss and stun). Volatiles stay fast.

## Files

- `src/poses.h`: most of the work (gait, idle, hips and feet, zombie styles, death fall).
- `src/game/19_entities.h` / `20_player.h`: pose buffer, fall timer, turn rate fields.
- `src/game/38_render_world.h`, `37_shadows.h`: feed the new inputs; shadows reuse the blended pose.
- `src/game/33_update_player.h`: walk cycle from real velocity.
- `src/game/34_zombie_ai.h`: turn rate (pass 3 only).
- `src/game/27_combat.h`: hit direction into the hurt pose, fall direction into the corpse.

No save format change.

## Cost

Pose blending is ~50 bones x on-screen characters, a few thousand slerps a frame. Cheap, but I'll watch the frame time with a big horde.

## Changelog lines (when built)

- Characters move like people: poses blend smoothly, feet stop skating, hips and shoulders move with the stride, and everyone breathes and shifts their weight when standing still.
- Zombies shamble, lurch and topple when they die, and you can see who's spotted you by the arms coming up.
- Zombies can't turn on a dime anymore. Walkers are slow to come about, brutes even slower.
