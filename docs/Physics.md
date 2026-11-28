## Overview
Moss uses Jolt Physics to provide fast, stable, and modern rigid-body dynamics for games and simulations.
It is designed to take full advantage of hardware acceleration and vectorized instructions across multiple CPU architectures.
[Jolt Physics 5.6.0](https://github.com/jrouwe/JoltPhysics)

Supported Architectures:
- ```X86/x64``` - SSE2, SSE4.1, SSE4.2, AVX, AVX2, or AVX512.
- ```ARM32/ARM64``` - NEON, FP16
- ```RISC-V64```
- ```LoongArch64```
- ```PowerPC64LE```

> [!NOTE]  
> Moss automatically selects the best available SIMD path at runtime, ensuring optimal performance without manual configuration.
> You can optionally force a specific instruction path for testing, benchmarking, or for low-end CPUs.


## Physics
- [Overview](/docs/Physics.md#overview)
### Physics 2D
- [Collision detection](/docs/), [Vehicles 2D](/docs/), [Softbody simulation](/docs/), [Animated ragdolls](/docs/), [Game character simulation](/docs/), [Water buoyancy calculations](/docs/), [Motors](/docs/)
- DampedSpringJoint2D, GrooveJoint2D, PinJoint2D, Path2D, PathFollow
### Physics 3D
- [Collision detection](/docs/), [Vehicles 3D](/docs/), [Softbody simulation](/docs/) [Animated ragdolls](/docs/), [Game character simulation](/docs/), [Water buoyancy calculations](/docs/), [Motors](/docs/)
-  VehicleWheel3D, SpringArm3D, DampedSpringJoint3D, GrooveJoint3D, PinJoint3D

## Macros
```cpp
```
## Enums
```cpp
```
## Structs
```cpp
```
## Classes
```cpp
```
## Functions
```cpp
```

## Moss-Level Debug Draw
Physics debug visualization now feeds the renderer debug list through descriptor helpers:

```cpp
Moss_DebugDrawList* list = Moss_RendererCreateDebugDrawList(renderer, 256);
Moss_PhysicsDebugDrawBodies(list, bodies, body_count, MOSS_PHYSICS_DEBUG_DRAW_BODIES);
Moss_PhysicsDebugDrawContacts(list, contacts, contact_count);
Moss_RendererDrawDebugList(renderer, list);
```

The helper layer is intentionally C-like: callers pass plain descriptor arrays, and the renderer owns the actual drawing path.

## Serialization
`Moss_PhysicsShapeSerialize` and `Moss_PhysicsSceneSerialize` write a compact JSON-like description for Moss-level shapes and contacts. This is meant for editor save data, fixtures, and smoke tests. Full Jolt scene binary serialization is still a backend-level task and should be added after the public Moss scene format is stable.

## Current Limitations
- Soft body vs soft body support is partial and needs per-backend validation before being treated as production-ready.
- Ragdoll support exposes low-level constraints, but high-level ragdoll presets still have type gaps.
- Collision mass and inertia are reliable for simple primitives; compound, mesh, and custom-center-of-mass cases may use approximations.
- Sensors can be marked in descriptors and debug output, but trigger/sensor callback behavior still needs consistent high-level wrappers across update paths.

## GPU Cloth And Hair
`Moss_GPUClothSolver` is a small compute helper for cloth and hair simulation. It can wrap an existing `Moss_ComputePipelineState` or create one from a compute shader descriptor, then bind particle/previous-position/velocity/constraint buffers and dispatch work groups. A starter GLSL kernel lives at `src/Renderer/Shaders/Compute/cloth_dynamics.glsl`.