# XR (Virtual/Augmented Reality)
## Overview
Moss XR is built on OpenXR. The public API creates instances, sessions, swapchains, frame loops, layers, actions, and haptics while leaving native graphics binding creation to the selected renderer/platform backend.

> [!NOTE]  
> Ensure you have [OpenXR SDK](https://sourceforge.net/projects/openxr-sdk.mirror/) installed

## Graphics Bindings
`MossXR_InitInfo::graphicsBinding` must point to the OpenXR graphics binding structure for the selected backend:

| VR Vendors | Supported (At Runtime) |
| ------------- | ------------- |
| Meta Quest |✔️ |
| Oculus / Rift |✔️ |
| PICO |✔️ |
| Pimax |✔️ |
| Valve |✔️ |
| Varjo |✔️ |
| Sony |✔️ |
| Microsoft HoloLens |✔️ |
| Microsoft Mixed Reality |✔️ |
| HTC Vive |✔️ |

| Platforms | Supported (At Runtime) |
| ------------- | ------------- |
| Windows |✔️ |
| Linux |✔️ |
| Android |✔️ |

## XR (Virtual/Augmented/Mixed Reality)
- [Overview](/docs/XR.md#overview)
    - [XROrigin](/docs/XR.md), [XRSpace](/docs/XR.md), [XRAnchor](/docs/XR.md), [XRSession](/docs/XR.md)
    - [XRCamera](/docs/XR.md)
    - [XRActionSet](/docs/XR.md), [XRActions](/docs/XR.md)
    - [XRControls](/docs/XR.md), [XRHaptic](/docs/XR.md)
    - [XRFaceModifier](/docs/XR.md),  [XRBodyModifier](/docs/XR.md), [XRHandModifier](/docs/XR.md),
    - [XRCharacterBody](/docs/XR.md)
    
```cpp
MossXR_InitInfo info{};
info.renderer = ERendererBackend::VULKAN;
info.graphicsBinding = &xrGraphicsBindingVulkan;
Moss_XR_Initialize(&info);
```

Backend binding targets:

| Backend | Binding type |
| --- | --- |
| Vulkan | `XrGraphicsBindingVulkanKHR` |
| OpenGL | Platform-specific OpenGL binding such as `XrGraphicsBindingOpenGLWin32KHR` or `XrGraphicsBindingOpenGLXlibKHR` |
| OpenGL ES | `XrGraphicsBindingOpenGLESAndroidKHR` |
| DirectX 12 | `XrGraphicsBindingD3D12KHR` |
| Metal | `XrGraphicsBindingMetalKHR` where supported by the runtime/platform bridge |

A compile-oriented lifecycle example lives at `examples/showcase_xr_graphics_binding/main.cpp`.

## Runtime Status
OpenXR instance/session/action scaffolding exists. Passthrough and foveated rendering are extension-gated capability toggles. Hardware validation still needs to be run on real XR runtimes for each graphics backend.


## XROrigin

## XREvents

## Actions

## XRControls

## XRHand

## XRCamera

## XRCharacterBody
