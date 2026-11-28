# Lesson 6 — Wave Spawning

## Goal

Create a wave director that decides when to spawn enemies, how many to spawn and when the current wave is complete.

## 1. Define wave state

```cpp
struct WaveState {
    int wave_number = 0;
    int enemies_remaining_to_spawn = 0;
    float spawn_interval = 1.0f;
    float spawn_timer = 0.0f;
    float break_seconds = 3.0f;
    bool wave_active = false;
};
```

Keep the director separate from individual enemies. It should request spawns; it should not own enemy movement, damage or rendering.

## 2. Start a wave

A simple difficulty curve could start with five enemies and add two per wave:

```cpp
void startNextWave(WaveState& wave) {
    ++wave.wave_number;
    wave.enemies_remaining_to_spawn = 5 + (wave.wave_number - 1) * 2;
    wave.spawn_interval = std::max(0.25f, 1.0f - 0.05f * wave.wave_number);
    wave.spawn_timer = 0.0f;
    wave.wave_active = true;
}
```

Use a cap on spawn interval so the game does not become impossible to read. Balance health and enemy speed separately from spawn count.

## 3. Spawn at controlled locations

Choose a set of spawn anchors near the arena edges. Reject an anchor if it is too close to the player or overlaps an existing enemy. Use a seeded random generator for reproducible tests:

```cpp
std::mt19937 rng{ level_seed };
```

Do not use global `rand()` throughout the project; injecting a generator makes tests and bug reproduction easier.

## 4. Complete a wave

A wave is complete only when no further enemies remain to spawn **and** no living enemies remain. Then start a short break timer, show a clear transition message and call `startNextWave` when the break ends. Do not test `enemies.empty()` alone if dying enemies remain in the vector for animation.

## Checkpoint

Wave 1 starts automatically, enemies spawn over time, the game waits until all living enemies are defeated, shows a short break and starts Wave 2 with a higher budget.

## Extension

Add a boss every fifth wave. Give the boss a different spawn delay and explicit phase state. Avoid scaling every parameter at once; one readable difficulty change per wave is easier to balance.
