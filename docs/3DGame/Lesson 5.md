# Lesson 5 — Multiple Enemies and Combat

## Goal

Scale from one enemy to a group while keeping updates predictable and object removal safe.

## 1. Store enemies in one collection

```cpp
std::vector<Enemy> enemies;
std::vector<Projectile> projectiles;
```

For a small arena, a `std::vector` is enough. Use stable IDs if other systems need to refer to enemies after the vector changes. Do not store long-lived raw pointers to elements that may move during vector growth or compaction.

## 2. Separate update passes

A practical fixed-step order is:

1. Read and apply player movement.
2. Update enemy state and movement.
3. Update weapon cooldown and create projectiles.
4. Move projectiles.
5. Resolve projectile/enemy hits.
6. Resolve enemy/player contact damage.
7. Remove or compact dead entities.
8. Update wave-completion conditions.

The order matters. For example, resolve projectile hits before deciding whether a wave is complete, or a wave may advance one tick earlier than intended.

## 3. Apply safe cleanup

During collision checks, mark objects inactive. After all checks are complete, compact the collections:

```cpp
auto remove_inactive = [](const auto& item) { return !item.active; };

projectiles.erase(std::remove_if(projectiles.begin(), projectiles.end(), remove_inactive), projectiles.end());
```

`Enemy` can use `state == EnemyState::Dead` instead of an `active` field. Decide whether dead enemies remain for a short death animation; if so, add a `Dying` state and timer before removal.

## 4. Add enemy contact damage

Use the same distance-squared test as projectile collision, but compare the player and enemy radii. Add a per-enemy contact cooldown or maintain a single player damage cooldown. Apply knockback as a separate response so it does not become entangled with the damage timer.

## 5. Render from the simulation state

Clear the debug draw list, add the floor, player, enemies and active projectiles, then submit it once. Moss's debug list API supports boxes and spheres; it is suitable for visualising this prototype. Do not create a new debug list every frame.

## Checkpoint

Spawn at least five enemies. Confirm that each can be hit once per projectile, that dead enemies no longer pursue the player, and that cleanup does not skip adjacent vector elements.

## Extension

Add two enemy archetypes: a slow durable enemy and a fast fragile enemy. Put tuning values in data/configuration rather than scattering `if` statements across the update loop.
