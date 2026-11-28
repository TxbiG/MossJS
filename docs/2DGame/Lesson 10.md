# Lesson 10 — Post-processing

## Goal

Understand where a post-processing stage belongs in the render graph and how Moss exposes the objects needed to execute one. This is an advanced lesson: a real effect also requires a valid backend-compatible shader, pipeline, resource bindings and framebuffer configuration.

## 1. Think in render stages

A simple 2D post-process pipeline looks like this:

```text
world draw calls
      |
      v
scene framebuffer
      |
      v
post-process pipeline (custom shader)
      |
      v
final output / backbuffer
```

The effect might apply colour grading, a vignette, scanlines, a blur or a subtle chromatic shift. Keep the base game readable; the effect should support the art direction rather than hide important gameplay information.

## 2. Moss declarations used by the pass

The current renderer header declares `Moss_RendererCreateFramebuffer`, `Moss_RendererCreatePipeline`, `Moss_RendererCreateResourceSet`, `Moss_PostProcessExecute`, `Moss_FramebufferBegin` and `Moss_FramebufferEnd`. It also declares this pass descriptor:

```cpp
struct Moss_PostProcessPass {
    Moss_PipelineState* pipeline;
    Moss_ResourceSet* resource_set;
    Moss_Framebuffer* target;
};
```

Once you have created a valid pipeline, resource set and destination framebuffer for your selected backend, the execution call is:

```cpp
Moss_PostProcessPass pass{};
pass.pipeline = post_process_pipeline;
pass.resource_set = post_process_resources;
pass.target = output_framebuffer;

Moss_PostProcessExecute(renderer, &pass);
```

This is the verified shape of the execution descriptor, not a full shader setup recipe. The descriptor types and shader language details are backend-sensitive; build them using the exact `Moss_PipelineDesc`, resource-layout and shader declarations in the current public headers.

## 3. Resize and lifetime

When the window/backbuffer changes size, resize or recreate off-screen targets to match the intended render resolution. Avoid using a framebuffer after it has been destroyed. Release the resource set, pipeline and framebuffer in an order that respects their dependencies, then destroy the renderer last.

## 4. Evaluate the effect

Compare the same scene with post-processing enabled and disabled. Check:

- Text and HUD legibility.
- High-contrast edges and pixel-art sharpness.
- Motion clarity during a jump.
- GPU cost at target resolution.
- Behaviour when the window is resized.

## Final challenge

Add one subtle full-screen effect and make it toggleable for development. Keep a clean non-post-processed path so visual bugs can be isolated from shader bugs.

## Completion

You now have the architecture of a small 2D platformer built around Moss's public framework surface. The next step is to replace the outline geometry with a verified texture/sprite asset workflow, then add levels, animation and accessibility settings.
