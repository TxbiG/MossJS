# Lesson 2 — Player Movement and Camera Space

## Goal

Move a player over the arena floor using a clear world-space coordinate convention. The prototype treats the player as a simple sphere/circle marker on the XZ plane.

## 1. Define a small game vector

The game can use a tiny local math type for learning, keeping the Moss API surface separate from gameplay calculations:

```cpp
struct Vec3f {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct Player3D {
    Vec3f position{0.0f, 0.6f, 0.0f};
    float move_speed = 7.0f;
    float radius = 0.55f;
    int health = 100;
};
```

In this track, positive Y is up. The ground plane is XZ. W/S move along the Z axis and A/D move along X. Choose the sign convention once and apply it to enemies, projectiles, arena bounds and model transforms.

## 2. Read input intent from Moss

```cpp
Vec3f readMoveIntent() {
    Vec3f direction{};
    if (Moss_IsKeyPressed(Keyboard::KEY_A)) direction.x -= 1.0f;
    if (Moss_IsKeyPressed(Keyboard::KEY_D)) direction.x += 1.0f;
    if (Moss_IsKeyPressed(Keyboard::KEY_W)) direction.z -= 1.0f;
    if (Moss_IsKeyPressed(Keyboard::KEY_S)) direction.z += 1.0f;
    return direction;
}
```

Call this after `Moss_PollEvents()` and once for each fixed simulation update. Normalise the vector if its length is greater than zero so diagonal movement is not faster than straight movement.

## 3. Integrate movement and clamp the arena

```cpp
void updatePlayer(Player3D& player, Vec3f direction, float dt) {
    const float length_sq = direction.x * direction.x + direction.z * direction.z;
    if (length_sq > 0.0f) {
        const float inverse_length = 1.0f / std::sqrt(length_sq);
        direction.x *= inverse_length;
        direction.z *= inverse_length;
    }

    player.position.x += direction.x * player.move_speed * dt;
    player.position.z += direction.z * player.move_speed * dt;

    constexpr float arena_half_extent = 9.0f;
    player.position.x = std::clamp(player.position.x, -arena_half_extent + player.radius, arena_half_extent - player.radius);
    player.position.z = std::clamp(player.position.z, -arena_half_extent + player.radius, arena_half_extent - player.radius);
}
```

Include `<cmath>` and `<algorithm>`. The clamp is a simple boundary response, not a full collision system. It is appropriate while the arena has only four walls.

## 4. Update the debug marker

Set `player_marker.center` to `Float3(player.position.x, player.position.y, player.position.z)` before calling `Moss_DebugDrawSphere`. Rebuild the debug list inside the render frame, not from the input function.

## Camera note

The camera remains fixed for this prototype. The current public header documents camera construction and selection, but this tutorial does not assume an unverified `SetPosition`/`LookAt` mutator. A true follow camera should be added only after the current camera update mechanism has been confirmed in the checked-out API.

## Checkpoint

The player moves in four directions, diagonal movement is normalised, and the player cannot leave the arena. Check that movement speed is measured in world units per second and is multiplied by delta time.
