# Lesson 3 — Creating the Player

## Goal

Create a player model that stores gameplay state separately from input and rendering. A player is not a Moss scene node; it is a C++ type owned by your game.

## 1. Define player state

```cpp
struct Player {
    float x = 2.0f;
    float y = 2.0f;
    float velocity_x = 0.0f;
    float velocity_y = 0.0f;
    float half_width = 0.35f;
    float half_height = 0.45f;
    bool grounded = false;
    bool alive = true;
};
```

Use the player's centre as its position. Width and height are stored as half-extents so collision tests can build an AABB without allocating a rectangle object every frame.

## 2. Read input through Moss

The platform header declares `Moss_IsKeyPressed` for held input and `Moss_IsKeyJustPressed` for a press transition. Always call `Moss_PollEvents()` before reading them in the main loop.

```cpp
struct PlayerInput {
    float horizontal = 0.0f;
    bool jump_pressed = false;
};

PlayerInput readPlayerInput() {
    PlayerInput input{};
    if (Moss_IsKeyPressed(Keyboard::KEY_A) ||
        Moss_IsKeyPressed(Keyboard::KEY_LEFT))
        input.horizontal -= 1.0f;

    if (Moss_IsKeyPressed(Keyboard::KEY_D) ||
        Moss_IsKeyPressed(Keyboard::KEY_RIGHT))
        input.horizontal += 1.0f;

    input.jump_pressed = Moss_IsKeyJustPressed(Keyboard::KEY_SPACE);
    return input;
}
```

This deliberately returns an intent structure rather than changing the player directly. That makes it easier to add gamepad input, replay input, AI possession or input remapping later.

## 3. Apply intent in game code

For the first pass, set horizontal velocity from input. Acceleration and friction are introduced in the next lesson.

```cpp
void updatePlayerIntent(Player& player, const PlayerInput& input) {
    constexpr float move_speed = 5.0f;
    player.velocity_x = input.horizontal * move_speed;

    if (input.jump_pressed && player.grounded) {
        player.velocity_y = 9.0f;
        player.grounded = false;
    }
}
```

This code is ordinary game logic. The constants are tuning values, not Moss configuration values. Do not poll keyboard state from inside rendering code: sample once per simulation tick and pass the result into the controller.

## 4. Render the player as a debug outline

```cpp
void drawPlayer(Moss_Renderer* renderer, const Player& player) {
    Vec2 centre(player.x, player.y);
    Vec2 size(player.half_width * 2.0f, player.half_height * 2.0f);
    Moss_RendererDrawBox2D(renderer, &centre, &size, 0.0f, Color(0.95f, 0.9f, 0.35f, 1.0f));
}
```

The call uses the framework's confirmed 2D debug drawing function. It is intentionally a placeholder for a future sprite/material path.

## 5. Keep input, update and draw separate

A simple application structure is:

```text
poll platform events
sample PlayerInput
run fixed simulation steps
begin renderer frame
    draw level
    draw player
end renderer frame
```

The player update should never draw; the draw function should never change velocity or position. This makes bugs reproducible and lets you test movement without a window.

## Checkpoint

The player outline should move left and right and start a jump when Space is pressed while grounded. At this point it may fall through the floor; collision is the purpose of Lesson 4.

## Extension

Add a `facing` value of -1 or +1. Update it only when horizontal input is non-zero, then use it later for sprite orientation.
