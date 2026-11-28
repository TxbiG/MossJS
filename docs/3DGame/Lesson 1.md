# Lesson 1 — Getting Started

## Goal

Create a Moss window, renderer, 3D camera and a debug-draw scene. The first result is a visible arena made from simple 3D bounds, not a finished modelled level.

## 1. Build against Moss

From a workspace containing sibling `Moss/` and `ArenaSurvivor/` directories, use the following project CMake pattern:

```cmake
cmake_minimum_required(VERSION 3.20)
project(ArenaSurvivor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory("../Moss" "${CMAKE_BINARY_DIR}/Moss")
add_executable(ArenaSurvivor src/main.cpp)
target_link_libraries(ArenaSurvivor PRIVATE Moss::Moss)
```

Moss's own CMake script defines the `Moss::Moss` build-tree alias. Configure the framework with the backend and SDK options required by your platform before treating the game as runnable on that machine.

## 2. Create a 3D camera

The public renderer header declares `Moss_Camera3DDesc` with position, target, up vector, field of view, aspect ratio and clipping planes.

```cpp
Moss_Camera3DDesc camera_desc{};
camera_desc.position = Float3(0.0f, 12.0f, 18.0f);
camera_desc.target = Float3(0.0f, 0.0f, 0.0f);
camera_desc.up = Float3(0.0f, 1.0f, 0.0f);
camera_desc.fov_degrees = 50.0f;
camera_desc.aspect_ratio = 1280.0f / 720.0f;
camera_desc.near_plane = 0.1f;
camera_desc.far_plane = 250.0f;

Moss_Camera* camera = Moss_CameraCreate3D(&camera_desc);
Moss_RendererSetCamera(renderer, camera);
```

Moss uses Y-up in the declared 3D camera structure. The view looks from the camera position toward its target. The sample uses a fixed elevated camera so it can be implemented using the public camera creation and selection calls without assuming an undocumented per-frame camera mutator.

## 3. Draw the arena with a debug list

Moss exposes a renderer-owned debug draw list and descriptors for boxes and spheres.

```cpp
Moss_DebugDrawList* debug_list = Moss_RendererCreateDebugDrawList(renderer, 256);

Moss_DebugDrawListClear(debug_list);

Moss_DebugBoxDesc floor{};
floor.center = Float3(0.0f, -0.5f, 0.0f);
floor.size = Float3(20.0f, 1.0f, 20.0f);
floor.color = Color(0.25f, 0.35f, 0.42f, 1.0f);
Moss_DebugDrawBox(debug_list, &floor);

Moss_DebugSphereDesc player_marker{};
player_marker.center = Float3(0.0f, 0.6f, 0.0f);
player_marker.radius = 0.55f;
player_marker.color = Color(0.2f, 0.85f, 1.0f, 1.0f);
Moss_DebugDrawSphere(debug_list, &player_marker);

Moss_RendererBeginFrame(renderer);
Moss_RendererDrawDebugList(renderer, debug_list);
Moss_RendererEndFrame(renderer);
```

Create the debug list once and reuse it. Clear and refill it each frame rather than allocating a new list every tick. The descriptor fields and function names are taken from the current `Moss_Renderer.h`.

## 4. Clean up in reverse ownership order

Destroy the debug list, then the camera, then the renderer, then the window:

```cpp
Moss_RendererDestroyDebugDrawList(renderer, debug_list);
Moss_CameraDestroy(camera);
Moss_RendererDestroy(renderer);
Moss_TerminateWindow(window);
```

Check every factory for `nullptr` and unwind objects already created if a later creation fails.

## Checkpoint

Run the game and see a floor outline/box and a player marker from an elevated perspective. Move the marker by changing its debug descriptor centre. The actual simulation and entity logic arrive in the next lessons.
