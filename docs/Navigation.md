# Navigation

## Overview
[Recast Navigation 1.6.0](https://github.com/recastnavigation/recastnavigation)

Moss navigation has two public layers:

- `Moss_NavigationAPI.h` is the small C-like API for applications. It exposes opaque nav mesh and crowd handles, triangle baking, 2D helpers, dynamic obstacles, serialization hooks, debug draw, and error strings.
- `Moss_Navigation.h` remains the legacy compatibility header that includes the modified Recast/Detour declarations used by backend code and older projects.

New application code should include `Moss/Moss_NavigationAPI.h` unless it needs direct Recast/Detour compatibility types.

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

## Core API
```cpp
Moss_NavMesh* Moss_NavMeshCreate(const Moss_NavMeshDesc* desc);
bool Moss_NavMeshBakeFromTriangles(Moss_NavMesh* mesh, const Moss_NavBakeDesc* desc);
bool Moss_NavMeshBakeFromTriangles2D(Moss_NavMesh* mesh, const Moss_NavBake2DDesc* desc);
int Moss_NavMeshFindPath(Moss_NavMesh* mesh, const Moss_NavVec3* start, const Moss_NavVec3* end, Moss_NavVec3* outPoints, int maxPoints);
int Moss_NavMeshFindPath2D(Moss_NavMesh* mesh, const Moss_NavVec2* start, const Moss_NavVec2* end, float planeHeight, Moss_NavVec2* outPoints, int maxPoints);
Moss_NavCrowd* Moss_NavCrowdCreate(Moss_NavMesh* mesh, const Moss_NavCrowdDesc* desc);
void Moss_NavCrowdUpdate(Moss_NavCrowd* crowd, float dt);
```

## Compatibility
The Recast/Detour surface is still available through `Moss_Navigation.h`, but it should be treated as backend/internal compatibility. The cleanup direction is to keep examples and application code on `Moss_NavigationAPI.h`.
