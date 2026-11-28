# Lesson 6 — Player States and Buffs

## Goal

Introduce explicit player states and temporary buffs without turning movement code into a chain of unrelated booleans.

## 1. Model mutually exclusive states

```cpp
enum class PlayerState {
    Grounded,
    Rising,
    Falling,
    Dead
};
```

Derive the state from simulation facts after collision resolution. A simple rule is: `Dead` takes priority; otherwise `Grounded` when a floor contact was registered; otherwise `Rising` when vertical velocity is positive; otherwise `Falling`. If you add dash, wall-slide or climb, make the state transitions explicit and define which states may interrupt one another.

## 2. Represent temporary buffs as data

```cpp
enum class BuffType {
    SpeedBoost,
    LowGravity,
    Shield
};

struct TimedBuff {
    BuffType type;
    float remaining_seconds;
    float magnitude;
};

struct PlayerStats {
    float base_move_speed = 5.0f;
    float base_gravity = 24.0f;
    bool shield_active = false;
};
```

Store active buffs in a vector. Decrease each timer by the fixed simulation delta and remove expired effects after iteration. Recalculate effective stats from base values and active modifiers each tick; do not repeatedly multiply the already-modified value, or it will grow exponentially.

## 3. Define stacking rules up front

For example:

- Speed Boost: highest active magnitude wins; it does not stack additively.
- Low Gravity: strongest reduction wins, but gravity never falls below a safe minimum.
- Shield: one active shield absorbs one hazard hit; a second shield extends duration only if you explicitly choose that design.

These are game design rules, not Moss behaviours. Put them in a buff system that can be unit-tested without a renderer.

## 4. Add jump grace and buffering

Coyote time stores a short countdown after the player leaves a platform. Jump buffering stores a short countdown when the player presses jump. A jump is allowed when grounded or within the coyote window, provided a buffered press exists. Decrement the timers on fixed ticks so the same simulation produces the same result at different render rates.

## Checkpoint

Show a visible state indicator in your debug HUD. Test that a speed pickup expires, the base speed is restored and the low-gravity effect does not accumulate across multiple pickups.

## Extension

Add a dash state with a fixed duration and a cooldown. Decide whether the dash can pass through hazards, cancel vertical velocity or be refreshed by landing. Document the rule before coding it.
