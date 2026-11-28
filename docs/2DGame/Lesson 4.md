# Lesson 4 — Movement and Tile Collision

## Goal

Make movement independent of frame rate and stop the player from passing through solid tiles. The platformer uses a small custom AABB/grid solver because the audited Moss physics surface is a Jolt-derived 3D system, not a drop-in 2D rigid-body API.

## 1. Use a fixed simulation step

Use a fixed interval such as 1/60 second for gameplay. Accumulate real elapsed time from `std::chrono::steady_clock`, run zero or more fixed updates, and render once per outer frame. Clamp unusually large frame times (for example after a breakpoint) so the application does not try to simulate many seconds in one frame.

```cpp
constexpr double fixed_step = 1.0 / 60.0;
double accumulator = 0.0;
auto previous = std::chrono::steady_clock::now();

while (!Moss_ShouldWindowClose(window)) {
    Moss_PollEvents();
    const auto now = std::chrono::steady_clock::now();
    double frame_time = std::chrono::duration<double>(now - previous).count();
    previous = now;
    frame_time = std::min(frame_time, 0.25);
    accumulator += frame_time;

    while (accumulator >= fixed_step) {
        const PlayerInput input = readPlayerInput();
        updateGame(static_cast<float>(fixed_step), input);
        accumulator -= fixed_step;
    }

    renderGame(renderer);
}
```

Include `<chrono>` and `<algorithm>`. `updateGame` and `renderGame` are functions you write. For games where a jump press must not be lost between fixed ticks, queue the press for a short input buffer rather than relying only on one instantaneous transition.

## 2. Build the player's AABB

```cpp
struct Aabb {
    float left, right, bottom, top;
};

Aabb playerBounds(const Player& p)
{
    return { p.x - p.half_width, p.x + p.half_width, p.y - p.half_height, p.y + p.half_height };
}

bool overlaps(const Aabb& a, const Aabb& b)
{
    return a.left < b.right && a.right > b.left && a.bottom < b.top && a.top > b.bottom;
}
```

A tile at integer `(x, y)` occupies `[x, x+1] × [y, y+1]`. Test only the grid cells touched by the player's bounds rather than scanning the whole level.

## 3. Resolve one axis at a time

Move horizontally first, then resolve horizontal penetrations against nearby solid cells. Move vertically next, then resolve vertical penetrations. Separating the axes makes the simple grid solver predictable and lets you determine whether the player landed on a floor or hit a ceiling.

For each candidate solid tile:

- Horizontal: if the player overlaps the tile after movement, place the player centre just outside the tile edge and set `velocity_x` to zero.
- Vertical: if moving downward, place the player's bottom at the tile top and set `grounded = true`; if moving upward, place the player's top at the tile bottom. In both cases set `velocity_y` to zero.

Apply gravity before vertical movement:

```cpp
player.velocity_y -= gravity * dt;
player.x += player.velocity_x * dt;
resolveHorizontal(player, map);
player.y += player.velocity_y * dt;
player.grounded = false;
resolveVertical(player, map);
```

`resolveHorizontal` and `resolveVertical` are game-owned helpers. Use the player's movement direction to choose the correct side when snapping; do not simply teleport to whichever tile edge is closest, because that can push the player through thin geometry.

## 4. Tune jump behaviour

A useful starting set of values is a horizontal speed around 5 world units/second, gravity around 24 units/second² and jump velocity around 9 units/second. These are not universal values. Tune them against tile size and level scale. Add coyote time (a short grace window after leaving a ledge) and jump buffering only after basic collision is reliable.

## Checkpoint

The player can run and jump over a gap, land on a raised platform and collide with the underside of a platform without passing through it. Reset the player to the start when it falls below a defined kill plane.

## Debugging

Draw the player's AABB and tile bounds with `Moss_RendererDrawBox2D`. Temporarily colour the grounded state differently. If a collision fails, inspect the calculated tile range and the exact AABB edges before changing gravity values.
