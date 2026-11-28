# Renderer
## Overview
The Moss renderer is the higher-level drawing layer built on top of `Moss_GPU.h`. New renderer APIs use C-like descriptors, opaque handles, explicit create/destroy calls, and backend-independent draw descriptions.
The Moss Renderer supports a wide range of graphics APIs such as ```OpenGL```, ```Vulkan```, ```Metal```, and ```DirectX12```. The Renderer is written in a C like style but is completely made in C++.

Renderer supports ```Windows```, ```Linux```, ```MacOS```, ```FreeBSD```, ```IOS``` and ```Android```.

> [!NOTE]  
> When working with Vulkan, ensure your graphics card supports it and you've installed [Vulkan SDK](https://vulkan.lunarg.com/sdk/home)
> All Features used for OpenGL are from [LearnOpenGL](https://learnopengl.com)

> [!TIP]
> For noise textures I recommend using this website [opengameart.org/content/700-noise-textures](https://opengameart.org/content/700-noise-textures)
> 
> Moss does support making noise texureing using FastNoiseLite

## Renderer
- [Overview](/docs/Renderer.md#overview)
    - [Introduction](/docs/Renderer.md#introduction)
    - [Choosing a Renderer](/docs/Renderer.md#choosing-a-renderer)
    - [Renderer Life-cycle](/docs/Renderer.md#renderer-life-cycles)
    - [Textures](/docs/Renderer.md#), [Shaders](/docs/Renderer.md#shaders), [Surface](/docs/Renderer.md#surface), [Mesh](/docs/Renderer.md#mesh), [Model](/docs/Renderer.md#Model)
    - [ParallaxLayer], [Line2D], [FogVolume], [Decal](/docs/3D.md#decal), [Sprite2D](/docs/2D.md#sprite2d), [Sprite3D]()
    - [Viewport & SubViewports](/docs/Renderer.md#), [Cameras](/docs/Renderer.md#)
    - [Lighting](/docs/Renderer.md#)
    - [Postprocessing](/docs/Renderer.md#post-processing), [Compositor]()
    - [Graphics API Specific](/docs/Renderer.md#graphics-api-specific)

## Choosing A Renderer
- ```Compatibility```: Web, Older / low-end. Desktop, Consoles, Mobile.
- ```Mobile```: Newer / high-end Mobile devices.
- ```Forward+```: Newer / high-end Desktop, Consoles.

## Comparison
| Features | Compatibility | Mobile | Forward+ |
| ------------- | ------------- | ------------- | ------------- |
| XR Support |✔️ | ✔️ | ✔️ |
| Web Support | ✔️ | ❌ | ❌ |
| Driver | OpenGL | Vulkan, Metal | Vulkan, DirectX 12, Metal |
| 2D Games |✔️ | ✔️ | ✔️ |
| 3D Games |✔️ | ✔️ | ✔️ |

### Lighting and shadows
| Features | Compatibility | Mobile | Forward+ |
| ------------- | ------------- | ------------- | ------------- |
| Approach | Forward | Forward | Forward+ | Forward or Forward+ |
| Content Cell | Content Cell | Content Cell | Content Cell | Content Cell |

### Global Illumination
| Features | Compatibility | Mobile | Forward+ |
| ------------- | ------------- | ------------- | ------------- |
| Content Cell | Content Cell | Content Cell | Content Cell |
| Content Cell | Content Cell | Content Cell | Content Cell |

### Environment and post-processing
| Features | Compatibility | Mobile | Forward+ |
| ------------- | ------------- | ------------- | ------------- |
| Fog (Depth and Height) | ✔️ | ✔️ | ✔️ |
| Volumetric Fog | ✔️ (Local Fog Volumes)| ✔️ | ✔️ |
| Tonemapping | ✔️ | ✔️ | ✔️ |
| Screen-Space Reflections | ✔️ | ✔️ | ✔️ |
| Screen-Space Ambient Occlusion (SSAO) | ✔️ | ✔️ | ✔️ |
| Glow | ✔️ | ✔️ | ✔️ |
| Adjustments | ✔️ | ✔️ | ✔️ |

### Antialiasing
| Features | Compatibility | Mobile | Forward+ |
| ------------- | ------------- | ------------- | ------------- |
| MSAA 2D | ❌ | ✔️ | ✔️ | ✔️ |
| MSAA 3D | ✔️ | ✔️ | ✔️ |
| TAA | ❌ | ✔️ | ✔️ | ❌ |
| FXAA | ❌ | ✔️ | ✔️ | ✔️ |
| SMAA | ❌ | ✔️ | ✔️ | ✔️ |
| SSAA | ✔️ | ✔️ | ✔️ | ✔️ |


### Matrix views
| Types | Description |
| ------------- | ------------- |
| u_proj | Projection matrix (orthographic / perspective) |
| u_local | Object/model transform (local space to world space) |
| u_world | World/scene transform (optional, often combined with model) |
| u_view | View/camera transform (world space → camera space) |
| u_mvp | Combined matrix: Projection X View X Model |
| u_normal | Normal matrix (inverse-transpose of model or model-view) |


## Defines
```cpp
// Provided by Moss
#define MOSS_GRAPHICS_OPENGL
#define MOSS_GRAPHICS_OPENGLES
#define MOSS_GRAPHICS_VULKAN
#define MOSS_GRAPHICS_DIRECTX
#define MOSS_GRAPHICS_METAL
```

## Renderer Life-cycles
[Add Text here]
```cpp
Moss_Renderer* Moss_CreateRenderer(Moss_Window& window);
```
```cpp
void Moss_RendererBeginFrame(Moss_Renderer& renderer);
```
```cpp
void Moss_PresentRenderer(Moss_Renderer& renderer);
```
```cpp
void Moss_TerminateRenderer(Moss_Renderer& renderer);
```

// Add Clear colour

Exmaple
```cpp
...

renderer = Moss_CreateRenderer(window);

while (!Moss_ShouldWindowClose(window))
    {

        ...

        Moss_RendererBeginFrame(renderer);

        ...

        Moss_PresentRenderer(renderer);

        ...
    };


Moss_TerminateRenderer(renderer);

...
```

## Camera 2D & 3D
[Add Text here]
```cpp
```
## Viewport & SubViewports
[Add Text here]
```cpp
```
## Skybox
This is used for 3D
[Add Text here]
```cpp
```
## Textures
[Add Text here]
```cpp
```

## Shaders
[Add Text here]
```cpp
```

## Mesh
[Add Text here]
```cpp
```
## Particle Effects

### Particle Effect 2D
```cpp
```

#### Example
```cpp
ParticleSystem2 g_particleSystem2D;

uint32_t explosionID = g_particleSystem2D.CreateParticleEffect(explosionTransform);
g_particleSystem2D.CreateParticleEmitter(); // attaches emitters to effect


Transform2 t;
t.position = pos;

uint32_t effectID = g_particleSystem2D.CreateParticleEffect(t);
ParticleEmitterID emitter = g_particleSystem2D.CreateParticleEmitter();

// Configure emitter
ParticleEmitter2 explosionEmitter;
explosionEmitter.m_transform = t.ToMat44();
explosionEmitter.amount = 100;      // 100 sparks
explosionEmitter.spawnTimer = 0;
g_particleSystem2D.m_activeEmitterBuckets[emitter] = ParticleEmitterBucket2(explosionEmitter);


g_particleSystem2D.Update(currentTime, deltaTime);

for (auto& [id, emitter] : g_particleSystem2D.m_activeEmitterBuckets) {
    emitter.draw(); // 2D = no view
}

```
### Particle Effect 3D
```cpp
```

#### Example
```cpp
ParticleSystem3 g_particleSystem3D;


Transform2 t;
t.position = pos;

uint32_t effectID = g_particleSystem3D.CreateParticleEffect(t);
ParticleEmitterID emitter = g_particleSystem3D.CreateParticleEmitter();

// Configure emitter
ParticleEmitter2 explosionEmitter;
explosionEmitter.m_transform = t.ToMat44();
explosionEmitter.amount = 100;      // 100 sparks
explosionEmitter.spawnTimer = 0;
g_particleSystem2D.m_activeEmitterBuckets[emitter] = ParticleEmitterBucket2(explosionEmitter);

g_particleSystem3D.Update(currentTime, deltaTime);

for (auto& [id, emitter] : g_particleSystem3D.m_activeEmitterBuckets) {
    emitter.draw();
}

```
## Post-processing
[Add Text here]
```cpp
```

## OpenGL and OpenGLES Specific
[Add Text here]

## Vulkan Spesific
[Add Text here]

## DirectX 12 Spesific
[Add Text here]

## Metal Spesific
[Add Text here]



## Renderer Lifecycle
```cpp
Moss_RendererDesc desc{};
desc.window = window;
desc.gpu_device = gpu_device;
Moss_Renderer* renderer = Moss_RendererCreate(&desc);

Moss_RenderFrameDesc frame{};
Moss_RendererBeginFrameEx(renderer, &frame);
Moss_PresentRenderer(renderer);

Moss_RendererDestroy(renderer);
```

## Renderer Resources
| Resource | Create | Destroy |
| --- | --- | --- |
| Material | `Moss_RendererCreateMaterial` | `Moss_RendererDestroyMaterial` |
| Mesh | `Moss_RendererCreateMesh` | `Moss_RendererDestroyMesh` |
| Model | `Moss_RendererLoadModel` | `Moss_ModelRemove` |
| Sprite batch | `Moss_RendererCreateSpriteBatch` | `Moss_RendererDestroySpriteBatch` |
| Debug draw list | `Moss_RendererCreateDebugDrawList` | `Moss_RendererDestroyDebugDrawList` |
| Camera | `Moss_CameraCreate2D`, `Moss_CameraCreate3D`, `Moss_CameraCreateEditor` | `Moss_CameraDestroy` |
| Shadow map | `Moss_RendererCreateShadowMap` | `Moss_RendererDestroyShadowMap` |

## Materials
Materials collect textures, uniforms, shader variants, pipeline state, and resource sets.

```cpp
Moss_MaterialDesc material_desc{};
material_desc.name = "terrain";
material_desc.pipeline = pipeline;
material_desc.resource_set = resource_set;
Moss_Material* material = Moss_RendererCreateMaterial(renderer, &material_desc);
```

## Meshes And Models
Meshes are submitted through `Moss_DrawMeshDesc`; models are submitted through `Moss_DrawModelDesc`.

```cpp
Moss_DrawMeshDesc draw{};
draw.mesh = mesh;
draw.pipeline = pipeline;
draw.resource_set = resource_set;
draw.transform = transform;
Moss_RendererDrawMesh(renderer, &draw);
```

## Sprites And 2D Drawing
Sprite batches collect `Moss_SpriteDesc` entries and submit them together.

```cpp
Moss_SpriteBatch* batch = Moss_RendererCreateSpriteBatch(renderer, 1024);
Moss_SpriteBatchAdd(batch, &sprite);
Moss_RendererDrawSpriteBatch(renderer, batch);
```

## Debug Draw
Debug lists are shared by renderer, physics, and navigation helpers.

```cpp
Moss_DebugDrawList* list = Moss_RendererCreateDebugDrawList(renderer, 256);
Moss_DebugDrawLine(list, &line);
Moss_DebugDrawBox(list, &box);
Moss_RendererDrawDebugList(renderer, list);
```

## Cameras
Use `Moss_CameraCreate2D` for orthographic 2D camera data, `Moss_CameraCreate3D` for perspective scene cameras, and `Moss_CameraCreateEditor` for editor-style movement settings.

## Shaders And GPU Resources
Shader modules, GPU buffers, textures, samplers, resource sets, barriers, and command buffers live in `Moss_GPU.h`. Renderer objects should reference those resources rather than owning backend-specific objects directly.

## Post-Processing And Shadows
Framebuffer, post-processing, and shadow-map APIs are present at the renderer layer. Backend render pass and shadow implementation details are still being filled in per graphics backend.

## Backend Notes
- OpenGL/OpenGL ES: compatibility and mobile paths.
- Vulkan: modern explicit backend path.
- DirectX 12: Windows explicit backend path.
- Metal: Apple explicit backend path.

Backend modules register native function tables with `Moss_RegisterNativeGPUBackend(...)` before device creation. If no native table is registered, Moss reports the backend as compiled/supported but using the fallback function table through `Moss_GetGPUBackendInfo(...)`.