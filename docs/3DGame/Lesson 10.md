# Lesson 10 — Post-processing and Final Build

## Goal

Add an optional final-render effect, complete cleanup and create a repeatable build. The post-process pass requires a backend-valid shader pipeline and resource set; the sample shows Moss's confirmed pass execution shape without inventing shader descriptor fields.

## 1. Understand the pass descriptor

Moss's renderer header declares:

```cpp
struct Moss_PostProcessPass {
    Moss_PipelineState* pipeline;
    Moss_ResourceSet* resource_set;
    Moss_Framebuffer* target;
};
```

It also declares `Moss_PostProcessExecute(renderer, &pass)`. After your project has correctly created a pipeline, resource set and target framebuffer for its graphics backend, execute the effect with:

```cpp
Moss_PostProcessPass pass{};
pass.pipeline = post_process_pipeline;
pass.resource_set = post_process_resources;
pass.target = output_framebuffer;
Moss_PostProcessExecute(renderer, &pass);
```

Do not copy shader code from another backend and assume it will work unchanged. Check the current `Moss_PipelineDesc`, shader, resource-layout and framebuffer declarations and the backend's supported formats.

## 2. Choose restrained effects

A wave-survival game benefits more from readable silhouettes and clear damage feedback than from strong distortion. Consider subtle colour grading, a low-health vignette or a short hit flash. Make the effect optional so you can compare the base output and isolate shader issues.

## 3. Complete ownership and teardown

Use a clear reverse-order cleanup policy:

1. Stop gameplay and audio updates.
2. Release active power-ups and game-owned entities.
3. Destroy models, fonts, debug draw lists, framebuffers, resource sets and pipelines using the appropriate owning API.
4. Destroy cameras.
5. Destroy the renderer.
6. Terminate the window.
7. Terminate the audio subsystem after its audio objects have been released.

The exact order between renderer resources should respect dependencies: a pipeline or resource set must not outlive objects it references, and the renderer must outlive renderer-owned resources. Keep an explicit ownership table in the project.

## 4. Final test pass

- Run several complete waves.
- Test every power-up and expiry path.
- Test player death and restart.
- Test an empty enemy list and a projectile that expires without hitting.
- Test missing model and font files.
- Test the window close path from title, active play and pause.
- Test at more than one frame rate; gameplay timers must use simulation delta.
- Record backend, compiler, CMake generator and Moss revision with the build.

## 5. Build reproducibly

For the sibling-folder layout shown in the README:

```bash
cmake -S ArenaSurvivor -B ArenaSurvivor/build -DCMAKE_BUILD_TYPE=Debug
cmake --build ArenaSurvivor/build --config Debug
```

Use `Release` for performance testing after the Debug configuration is stable. Keep the Moss source revision pinned for releases so an API change cannot silently alter the build.

## Completion

You now have a compact 3D arena-survival game architecture using Moss's public platform, renderer, camera, debug draw, model and text APIs. Physics, materials, textures, input-to-world aiming and post-process shader setup are natural advanced follow-ons, but each should be added against the exact current public declarations and backend support rather than guessed from another engine.
