# Your First 2D Game — Rift Runner

Build a small 2D platformer using Moss as the platform, input and rendering foundation. The tutorial starts with a visible debug-geometry prototype and gradually adds game-owned systems. The debug rectangles are intentionally temporary: they let you test movement, level scale and collision before introducing a verified texture/material pipeline.

## What you will build

- A C++17 desktop application using the Moss platform and renderer.
- A tile-grid level with solid platforms, hazards and collectibles.
- A player controller with acceleration, gravity, jumping and tile collision.
- Collectibles, score, temporary buffs and clear game states.
- HUD text, audio lifecycle integration, debug tools and post-processing architecture.

## Lesson sequence

| Lesson | Topic | Result |
|---:|---|---|
| 1 | [Getting Started](Lesson%201.md) | Window, renderer, 2D camera and frame loop |
| 2 | [Building a Tilemap](Lesson%202.md) | Data-driven level grid and debug visualisation |
| 3 | [Creating the Player](Lesson%203.md) | Player data, input intent and movement state |
| 4 | [Movement and Tile Collision](Lesson%204.md) | Fixed-step gravity, jumping and AABB resolution |
| 5 | [Collectibles and Scoring](Lesson%205.md) | Pickup tests, score and safe entity removal |
| 6 | [Player States and Buffs](Lesson%206.md) | Timed effects and player-state rules |
| 7 | [Scene Flow and Game States](Lesson%207.md) | Title, playing, pause, death and completion flow |
| 8 | [HUD and Feedback](Lesson%208.md) | Font rendering, HUD values and feedback design |
| 9 | [Polish, Testing and Performance](Lesson%209.md) | Audio integration notes, testing and optimisation |
| 10 | [Post-processing](Lesson%2010.md) | Framebuffer, pipeline and final-render workflow |

## Controls used by the prototype

- **A / D** or **Left / Right**: move.
- **Space**: jump.
- **Escape**: request window close.

These are tutorial defaults, not hard-coded Moss input mappings. They are read through `Moss_IsKeyPressed` and `Moss_IsKeyJustPressed` after `Moss_PollEvents`.
