# Lesson 9 — Building the 3D World

## Goal

Replace part of the debug arena with imported models and understand where Moss's 3D physics system fits. This lesson is intentionally explicit about the difference between confirmed entry points and the setup that your game must provide.

## 1. Load a model

The renderer header declares model loaders for FBX, OBJ and GLB, as well as a generic `Moss_ModelLoad` entry point. A useful first test is a small GLB model:

```cpp
Moss_Model* arena_model = Moss_ModelLoadGLB("assets/models/arena.glb");
if (arena_model == nullptr) {
    const char* message = Moss_ModelGetLastLoadErrorMessage();
    // Log the message, asset path and current working directory.
}
```

Render it with a column-major 4x4 transform. The renderer header documents the transform arrays as column-major:

```cpp
const float identity_transform[16] = {
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1
};

if (arena_model != nullptr)
    Moss_RendererDrawModel(renderer, arena_model, identity_transform);
```

The exact model importer support and required build dependencies should be tested with the current checkout. On shutdown, release the model with `Moss_ModelRemove(arena_model)` before destroying the renderer. Keep the debug floor until the imported scene has a confirmed scale and orientation.

## 2. Plan world units and collision separately

Choose a scale convention (for example, one world unit equals one metre) and use it consistently for model dimensions, enemy radii, movement speed, projectile speed and camera clipping planes. Do not assume an imported asset is authored at the scale you intended; inspect its bounds and adjust its transform or content pipeline.

## 3. Add a light and shadow plan

The renderer header declares 3D light data structures and shadow-map creation/destruction and pass calls. Use one clear key light first. Add a shadow map only after the scene's materials and depth behaviour are working. Renderer capability and backend support must be verified for the selected configuration; do not assume every lighting feature is active on every backend.

## 4. Understand the 3D physics integration boundary

The public physics system is defined in `Moss/Physics/PhysicsSystem.h` and follows Jolt-style interfaces. The header confirms this API shape:

```cpp
PhysicsSystem physics;
physics.Init(max_bodies, body_mutex_count, max_body_pairs, max_contact_constraints, broad_phase_layer_interface, object_vs_broad_phase_layer_filter, object_layer_pair_filter);

BodyInterface& bodies = physics.GetBodyInterface();

// Each simulation step, with valid long-lived allocator/job-system objects:
physics.Update(fixed_dt, collision_steps, &temp_allocator, &job_system);
```

This is an API contract sketch, not a standalone compiling physics sample. Your project must create suitable broadphase/object-layer filter implementations, keep those referenced objects alive for the physics system's lifetime, construct the job system and temporary allocator, create shapes/body settings and follow the body creation/activation rules in the corresponding Moss physics headers. `PhysicsSystem::Update` divides the requested time into collision steps and uses the supplied job system and temporary allocator.

The arena prototype can remain on simple game-owned collision until you need rigid-body contact, ragdolls, vehicles or more complex physical interactions. Integrate the physics system deliberately rather than adding it just to move a sphere around a flat plane.

## Checkpoint

Load one GLB prop, render it with an identity or scale transform, and preserve the debug geometry for collision tuning. Record the asset scale and the backend used for the test.
