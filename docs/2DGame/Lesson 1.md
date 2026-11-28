# Lesson 1 — Getting Started

## Goal

Create the smallest useful Moss application: a window, a renderer, a 2D camera and a frame loop. This lesson is about getting the framework and your application boundary correct before adding gameplay.

## 1. Build Moss first

Clone the repository with its submodules where the project setup requires them:

```bash
git clone --recurse-submodules https://github.com/TxbiG/Moss.git
cmake -S Moss -B Moss/build -DCMAKE_BUILD_TYPE=Debug
cmake --build Moss/build --config Debug
```

Use the backend options and platform dependencies documented by the checkout you actually cloned. Do not add an unverified option copied from an older branch. The tutorial's CMake integration uses the build-tree target exposed by Moss:

```cmake
cmake_minimum_required(VERSION 3.20)
project(RiftRunner LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory("../Moss" "${CMAKE_BINARY_DIR}/Moss")
add_executable(RiftRunner src/main.cpp)
target_link_libraries(RiftRunner PRIVATE Moss::Moss)
```

The path to `../Moss` is relative to this example's `CMakeLists.txt`. Moss creates `Moss` and the alias `Moss::Moss` in its CMake build script.

## 2. Create the application shell

The following is a complete minimal example of the framework calls used in this lesson. It deliberately uses the simple renderer factory; more advanced GPU/backend configuration belongs in a separate setup pass.

```cpp
#include <Moss/Moss.h>

int main()
{
    Moss_Window* window = Moss_CreateWindow("Rift Runner", 1280, 720, nullptr, nullptr);

    if (window == nullptr)
        return 1;

    Moss_Renderer* renderer = Moss_CreateRenderer(window);
    if (renderer == nullptr) {
    Moss_TerminateWindow(window);
        return 1;
    }

    Moss_Camera2DDesc camera_desc{};
    camera_desc.viewport_width = 1280.0f;
    camera_desc.viewport_height = 720.0f;

    Moss_Camera* camera = Moss_CameraCreate2D(&camera_desc);
    if (camera == nullptr) {
        Moss_RendererDestroy(renderer);
        Moss_TerminateWindow(window);
        return 1;
    }

    Moss_RendererSetCamera(renderer, camera);

    while (!Moss_ShouldWindowClose(window)) {
        Moss_PollEvents();

        if (Moss_IsKeyJustPressed(Keyboard::KEY_ESCAPE))
            Moss_CloseWindow(window);

        Moss_RendererBeginFrame(renderer);

        // A debug outline proves that the camera and render loop are active.
        Vec2 centre(640.0f, 360.0f);
        Vec2 size(160.0f, 100.0f);
        Moss_RendererDrawBox2D(renderer, &centre, &size, 0.0f, Color(0.2f, 0.8f, 1.0f, 1.0f));

        Moss_RendererEndFrame(renderer);
    }

    Moss_CameraDestroy(camera);
    Moss_RendererDestroy(renderer);
    Moss_TerminateWindow(window);
    return 0;
}
```

The call names and descriptor fields above are present in the current public headers. Build against the same Moss checkout you inspected; a backend can still fail at runtime if its SDK, driver or compile configuration is missing.

## 3. Understand the frame

The application owns the outer loop. `Moss_PollEvents()` refreshes platform and input state. `Moss_RendererBeginFrame()` starts the frame, draw calls queue their work, and `Moss_RendererEndFrame()` completes it. Game simulation will be inserted between event polling and rendering in later lessons.

The camera descriptor sets an initial viewport size. This first project keeps the camera static so the sample uses only the declared camera creation and selection calls. Do not assume a camera setter or follow method that is not present in your checked-out header.

## Checkpoint

You should see a window with a cyan rectangle outline. Press Escape to close it. If the window appears but no outline is visible, verify that the renderer was created successfully, the selected backend is supported, and your frame calls are reached.

## Exercise

Change the window title and rectangle size. Then create a second `Vec2` centre/size pair and draw a second box. Keep the game loop independent of platform-specific rendering code.

## Next

In Lesson 2, convert the level into a data grid and draw its solid cells with the same debug-box call.
