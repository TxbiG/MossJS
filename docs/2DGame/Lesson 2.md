# Lesson 2 — Building a Tilemap

## Goal

Represent a level as data rather than hard-coded draw calls. The first level is a simple tile grid; each cell has a small semantic value that controls drawing and collision.

## 1. Choose a tile vocabulary

Keep the first version intentionally small:

```cpp
enum class Tile : unsigned char {
    Empty,
    Solid,
    Hazard,
    Start,
    Exit
};
```

A level stores a fixed width, height and row-major array. For this prototype, row zero is the bottom row and the player moves in a world where positive Y points upward.

```cpp
#include <vector>
#include <cstddef>

struct TileMap {
    int width = 0;
    int height = 0;
    std::vector<Tile> cells;

    Tile at(int x, int y) const {
        if (x < 0 || y < 0 || x >= width || y >= height)
            return Tile::Solid; // Treat outside the map as a wall.
        return cells[static_cast<std::size_t>(y * width + x)];
    }

    bool isSolid(int x, int y) const {
        const Tile tile = at(x, y);
        return tile == Tile::Solid || tile == Tile::Hazard;
    }
};
```

Treating out-of-bounds cells as solid prevents the player from walking through the edge while you are still building the level. You can later make exits or open boundaries explicit.

## 2. Define a small test level

A readable text layout is convenient while learning. Convert symbols to `Tile` values once during loading; do not parse strings every frame.

```text
....................
....................
.............E......
.........###........
....C...............
..###.......H.......
S###################
```

Suggested symbols: `.` empty, `#` solid, `H` hazard, `C` collectible, `S` start and `E` exit. The parser is application code; ensure every row has exactly the same length and report a useful error for invalid symbols.

## 3. Draw the grid using Moss debug geometry

Moss's public renderer header declares `Moss_RendererDrawBox2D`. It queues an outline box, making it useful as a temporary level visualisation. It does not fill a tile or load an image.

```cpp
void drawTileMap(Moss_Renderer* renderer, const TileMap& map) {
    for (int y = 0; y < map.height; ++y) {
        for (int x = 0; x < map.width; ++x) {
            if (map.at(x, y) == Tile::Empty)
                continue;

            Vec2 centre(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f);
            Vec2 size(1.0f, 1.0f);

            Color colour = Color(0.35f, 0.65f, 0.85f, 1.0f);
            if (map.at(x, y) == Tile::Hazard)
                colour = Color(1.0f, 0.25f, 0.15f, 1.0f);

            Moss_RendererDrawBox2D(
                renderer, &centre, &size, 0.0f, colour);
        }
    }
}
```

Call `drawTileMap` between `Moss_RendererBeginFrame(renderer)` and `Moss_RendererEndFrame(renderer)`. Keep one world-space tile equal to one game unit initially. The current camera viewport is in pixels, so decide a world-to-screen scale before making a large map; do not mix tile counts and screen pixels throughout gameplay code.

## 4. Keep level data separate from presentation

The tilemap decides whether a location is solid or hazardous. The renderer only visualises that decision. This separation lets you change the art later without rewriting collision, and it also makes the level easy to validate with unit tests.

## Checkpoint

Draw a floor, at least two floating platforms, one hazard and a start/exit marker. Confirm that changing a cell in the data changes the drawing without changing the rendering loop.

## Common mistakes

- Reversing the row order between parsing and drawing.
- Using a signed index without checking bounds.
- Treating a hazard as a platform without defining the intended interaction.
- Expecting the debug box to be a filled sprite.

## Next

Lesson 3 introduces the player as game-owned data and samples Moss keyboard state after the event poll.
