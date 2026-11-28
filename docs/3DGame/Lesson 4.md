# Lesson 4 — Weapons and Projectiles

## Goal

Create projectiles with velocity, a lifetime, a target test and a weapon fire cooldown. This version uses automatic firing in the direction of the nearest enemy to keep the first combat loop simple.

## 1. Data model

```cpp
struct Projectile {
    Vec3 position{};
    Vec3 velocity{};
    float radius = 0.12f;
    float remaining_seconds = 2.0f;
    int damage = 1;
    bool active = true;
};

struct Weapon {
    float seconds_between_shots = 0.35f;
    float cooldown = 0.0f;
    float projectile_speed = 12.0f;
};
```

Projectiles are separate from enemies so they can be updated, tested and removed independently.

## 2. Fire on a cooldown

Decrease the cooldown by fixed delta time. When it reaches zero, choose a target, calculate a normalised direction on the XZ plane, create a projectile at the player's position and reset the cooldown. If there is no living enemy in range, do not spawn a projectile.

```cpp
if (weapon.cooldown > 0.0f)
    weapon.cooldown -= dt;

if (weapon.cooldown <= 0.0f && has_target) {
    projectiles.push_back(makeProjectile(player.position, target.position,
                                         weapon.projectile_speed));
    weapon.cooldown = weapon.seconds_between_shots;
}
```

`makeProjectile` is a game helper: subtract source from target, set Y velocity to zero, normalise the XZ direction and multiply it by the projectile speed.

## 3. Update projectile motion

```cpp
void updateProjectile(Projectile& shot, float dt) {
    if (!shot.active)
        return;
    shot.position.x += shot.velocity.x * dt;
    shot.position.y += shot.velocity.y * dt;
    shot.position.z += shot.velocity.z * dt;
    shot.remaining_seconds -= dt;
    if (shot.remaining_seconds <= 0.0f)
        shot.active = false;
}
```

## 4. Test hits with squared distance

For a projectile and enemy, calculate the squared 3D distance between their centres. A hit occurs when it is less than or equal to `(projectile.radius + enemy.radius)²`. Squared distance avoids a square root for every collision candidate.

On a hit, subtract projectile damage from enemy health, deactivate the projectile, and set the enemy to `Dead` when health reaches zero. Ensure one projectile can only hit once.

## Checkpoint

The player automatically fires at a target, projectiles travel at a consistent world speed, enemy health decreases, and expired or hit projectiles stop colliding.

## Extension

Add aim direction from the mouse or a gamepad stick only after the current target-selection rules work. Input-to-world conversion depends on the camera and viewport mapping; confirm the current Moss mouse-coordinate and camera-projection APIs before writing that conversion.
