# Lesson 3 — The First Enemy

## Goal

Add one enemy that follows the player. Keep the enemy as ordinary game data and use the same debug draw path to visualise it.

## 1. Define enemy behaviour

```cpp
enum class EnemyState {
    Chasing,
    Stunned,
    Dead
};

struct Enemy {
    Vec3f position{4.0f, 0.6f, 3.0f};
    float speed = 2.5f;
    float radius = 0.5f;
    int health = 2;
    EnemyState state = EnemyState::Chasing;
};
```

Do not put a pointer to the player inside every enemy unless you need it. Pass the player position into the update function. This keeps enemies easier to test and avoids dangling references when scenes restart.

## 2. Pursuit steering

```cpp
void updateEnemy(Enemy& enemy, const Player3D& player, float dt)
{
    if (enemy.state != EnemyState::Chasing)
        return;

    float dx = player.position.x - enemy.position.x;
    float dz = player.position.z - enemy.position.z;
    const float distance_sq = dx * dx + dz * dz;
    if (distance_sq < 0.0001f)
        return;

    const float inverse_distance = 1.0f / std::sqrt(distance_sq);
    dx *= inverse_distance;
    dz *= inverse_distance;

    enemy.position.x += dx * enemy.speed * dt;
    enemy.position.z += dz * enemy.speed * dt;
}
```

The enemy moves only on the ground plane. In a more advanced version, separate desired velocity from actual velocity and add acceleration, avoidance, attack range and pathfinding.

## 3. Draw the enemy marker

Add a `Moss_DebugSphereDesc` to the debug list at the enemy's position, with a different colour from the player. If you have more than one shape, create and refill the list once per frame and submit it once with `Moss_RendererDrawDebugList`.

## 4. Avoid overlapping the player

When the enemy gets close enough, calculate a damage timer or enter an attack state. Do not subtract health every rendered frame. Use a cooldown so one continuous overlap produces controlled damage rather than instantly removing all player health.

## Checkpoint

The enemy consistently closes the distance to the player without moving faster when the frame rate increases. It should stop changing position when marked `Dead` or `Stunned`.

## Extension

Add a wind-up state before an attack. Show the state through a colour change in the debug sphere. That gives the player readable feedback and introduces an enemy state machine before you add several enemy types.
