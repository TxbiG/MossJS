# Lesson 5 — Collectibles and Scoring

## Goal

Add collectible objects and a score counter without coupling the pickup rules to the renderer.

## 1. Store collectible data

```cpp
struct Collectible {
    float x = 0.0f;
    float y = 0.0f;
    float radius = 0.2f;
    int score_value = 10;
    bool active = true;
};

struct GameProgress {
    int score = 0;
    int collected_count = 0;
};
```

Store collectibles in a `std::vector<Collectible>`. Give each pickup a stable identifier if you later add save files, networking or persistent level state. For a first prototype, a simple active flag is enough.

## 2. Test player-to-coin overlap

The closest point on an AABB to a circle centre gives a cheap circle/AABB overlap test:

```cpp
float clampFloat(float value, float low, float high)
{
    return std::max(low, std::min(value, high));
}

bool circleTouchesAabb(float cx, float cy, float radius, const Aabb& box)
{
    const float nearest_x = clampFloat(cx, box.left, box.right);
    const float nearest_y = clampFloat(cy, box.bottom, box.top);
    const float dx = cx - nearest_x;
    const float dy = cy - nearest_y;
    return dx * dx + dy * dy <= radius * radius;
}
```

Include `<algorithm>`. The test works for circles touching a player's axis-aligned collision box, which is a good fit for this prototype.

## 3. Collect first, remove later

Do not erase elements from a vector in the middle of a range-based loop. Mark items inactive while testing, then remove or compact them after the pass.

```cpp
void collectNearby(Player& player, std::vector<Collectible>& items, GameProgress& progress) {
    const Aabb bounds = playerBounds(player);
    for (Collectible& item : items) {
        if (!item.active)
            continue;
        if (circleTouchesAabb(item.x, item.y, item.radius, bounds)) {
            item.active = false;
            progress.score += item.score_value;
            ++progress.collected_count;
        }
    }
}
```

After the loop, use the erase-remove idiom or keep inactive objects until the level is unloaded. The latter can be useful if you want stable IDs or pickup respawn rules.

## 4. Render and display the result

Draw active pickups as small 2D debug circles using `Moss_RendererDrawCircle2D(renderer, &centre, radius, colour)`; `centre` is a `Vec2`. This is an outline/debug primitive, not a textured coin. The score is game data; the HUD reads it later rather than the pickup changing GUI widgets directly.

## Checkpoint

Place five pickups across the level. Each should increase the score exactly once, and inactive pickups should no longer render or collide. Resetting the scene should reset the score and restore the collectibles.

## Extension

Add a rare pickup with a different score value. Then add a “perfect run” bonus that is awarded only when all required pickups are collected without touching a hazard. Keep bonus calculation in a `GameProgress` or level-results system, not in the renderer.
