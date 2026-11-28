# Your First 3D Game — Arena Survivor

Build a compact top-down/angled 3D arena-survival game. The prototype uses Moss's 3D camera, renderer lifecycle, debug draw list, model importer and font renderer, while the tutorial project owns its entities, combat rules, projectiles, wave spawner and buffs.

## What you will build

- A C++17 application with a Moss window, renderer and 3D camera.
- A player that moves around a bounded arena.
- Enemies that pursue the player and can be defeated by projectiles.
- A weapon cooldown, hit handling, score and wave progression.
- Multiple enemies, timed power-ups, imported world models, HUD and post-processing structure.

## Lesson sequence

| Lesson | Topic | Result |
|---:|---|---|
| 1 | [Getting Started](Lesson%201.md) | Moss window, renderer, 3D camera and debug scene |
| 2 | [Player Movement and Camera Space](Lesson%202.md) | World-space movement and arena boundaries |
| 3 | [The First Enemy](Lesson%203.md) | Enemy state and pursuit steering |
| 4 | [Weapons and Projectiles](Lesson%204.md) | Cooldown, projectile motion and hit tests |
| 5 | [Multiple Enemies and Combat](Lesson%205.md) | Entity collections, damage and cleanup |
| 6 | [Wave Spawning](Lesson%206.md) | Spawn budgets, wave timing and difficulty |
| 7 | [HUD and Game Feedback](Lesson%207.md) | Font rendering, health, score and wave display |
| 8 | [Buffs and Power-ups](Lesson%208.md) | Timed modifiers and stacking rules |
| 9 | [Building the 3D World](Lesson%209.md) | GLB/OBJ/FBX model loading, lighting concepts and physics integration path |
| 10 | [Post-processing and Final Build](Lesson%2010.md) | Framebuffer passes, profiling, cleanup and packaging |

## Controls used by the prototype

- **W / A / S / D**: move on the ground plane.
- **Escape**: request window close.
- The tutorial starts with automatic fire to keep input scope small; aim/fire controls are an extension exercise.

## Physics scope

Moss's `PhysicsSystem` is Jolt-derived and exposes 3D rigid-body simulation through `PhysicsSystem::Init`, `GetBodyInterface` and `Update`. Its initialization requires broadphase and object-layer filter instances plus job-system and temporary-allocation support at update time. The core arena prototype uses simple game-owned sphere/circle distance tests first; Lesson 9 explains the safe integration boundary without pretending physics setup is a one-line call.
