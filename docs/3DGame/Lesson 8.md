# Lesson 8 — Buffs and Power-ups

## Goal

Add power-ups that temporarily modify player abilities. The system should be data-driven and should not make each weapon or enemy know every buff type.

## 1. Define effects and duration

```cpp
enum class PowerUpType {
    RapidFire,
    SpeedBoost,
    DamageBoost,
    Heal
};

struct ActivePowerUp {
    PowerUpType type;
    float remaining_seconds;
    float strength;
};

struct PlayerModifiers {
    float move_speed_multiplier = 1.0f;
    float fire_rate_multiplier = 1.0f;
    float damage_multiplier = 1.0f;
};
```

A heal is an instant effect; the other three are timed effects. Treat those as different application paths even if they share a pickup representation.

## 2. Recalculate from base values

At each fixed update, build effective modifiers from the active list. For example, the strongest speed boost wins, while rapid-fire multipliers may multiply only if the design deliberately allows stacking. Clamp the final values to safe limits.

Do not change `base_move_speed` when a buff activates. If you repeatedly multiply the current speed, the value can grow permanently after the buff expires.

## 3. Make pickup behaviour explicit

Define whether duplicate buffs refresh duration, extend duration, stack strength or are ignored. Use the same rule for every buff unless there is a deliberate design reason not to. A small `applyPowerUp` function should own these decisions.

## 4. Connect to the wave director

Power-ups can spawn after an enemy dies, between waves or at fixed arena points. Keep drop probability and spawn decisions in a reward/drop system, not inside the enemy's render function. Seed random drops so a play-test can be repeated.

## Checkpoint

Collect each power-up, confirm the affected stat changes, then wait for the timer to expire and check that the original base stat returns. Test collecting the same buff twice.

## Extension

Add a temporary shield with one hit of protection. Make the HUD show a shield status and ensure the shield is removed exactly once when it absorbs damage.
