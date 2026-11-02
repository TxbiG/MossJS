# MossJS Cheatsheet

> Quick reference for the idiomatic TypeScript public API. The companion [API Reference](API_Reference.md) defines the complete feature map and all exported native-to-language projections. The public calls below are the target design, not a claim that the current binding has implemented every member yet.

## Language design at a glance

The public surface is an ES module with first-class TypeScript declarations. Use lowerCamelCase functions, PascalCase resource classes, discriminated unions, typed arrays and browser-native objects. WebAssembly startup and browser asset I/O are asynchronous; ordinary in-memory operations remain synchronous.

## Subsystem map

| Moss subsystem | Public module / facade | Responsibility |
|---|---|---|
| Core | `core` | Foundational value types, logging, memory, profiler, jobs, streams and utilities. |
| Platform | `platform` | Windows/surfaces, monitors, events, input, gamepads, sensors, cameras, storage and dialogs. |
| GPU | `gpu` | Devices, buffers, textures, samplers, shaders, pipelines, command recording, swapchains and synchronization. |
| Renderer | `renderer` | Frame lifecycle, materials, resource sets, framebuffers, meshes, models, fonts, sprites, cameras and debug draw. |
| Audio | `audio` | Audio engine, sources, streams, channels, effects, listeners, acoustics, speaker and microphone capture. |
| GUI | `gui` | Contexts, windows, containers, widgets, text/scalar input and drawing. |
| Physics | `physics` | Bodies, shapes, filters, collisions, constraints, characters, vehicles, soft bodies and navigation integration. |
| Network | `network` | Hosts, peers, packets, transports, HTTP/TLS, WebSocket and WebRTC where enabled. |
| XR | `xr` | Runtime/session lifecycle, views, actions, tracking, haptics, hand/body/face tracking and composition. |

## 1. Import Moss

```typescript
import { Moss, GpuDeviceDesc, GpuBufferDesc, GpuBufferUsage, type WindowOptions } from "moss";
```

## 2. Create a window and pump events

```typescript
const canvas = document.querySelector<HTMLCanvasElement>("#moss")!;
const app = await Moss.create({ canvas });
const window = app.platform.attachCanvas(canvas, {
  title: "Moss application",
  width: 1280,
  height: 720,
});

try {
  while (!window.shouldClose) {
    app.platform.pollEvents();
    // Update application state.
  }
} finally {
  await app.dispose();
}
```

## 3. Create and upload a GPU buffer

```typescript
const device = await app.gpu.createDevice(new GpuDeviceDesc());
const buffer = device.createBuffer({
  size: vertices.byteLength,
  usage: GpuBufferUsage.Vertex,
});
buffer.upload(vertices); // Accepts ArrayBufferView / TypedArray.
// Dispose resources before disposing the owning device.
```

## 4. Render a frame

```typescript
const frame = app.renderer.beginFrame();
frame.setCamera(camera);
frame.drawSprite(sprite);
frame.drawText("Hello Moss", position, textStyle);
frame.finish();
```

## 5. Play audio

```typescript
const source = await app.audio.loadWav("/assets/theme.wav");
source.play();
app.audio.update(deltaTime);
source.dispose();
```

## 6. Build a GUI

```typescript
const ui = app.gui.createContext();
ui.panel("Inspector", panel => {
  panel.text("Player");
  const enabled = panel.checkbox("Enabled", currentValue);
});
```

## 7. Simulate physics

```typescript
const physics = app.physics.createSystem(settings);
const body = physics.bodies.create({
  shape: Shape.box(size),
  motionType: MotionType.Dynamic,
});
physics.step(deltaTime);
const contacts = physics.query.contacts(body);
```

## 8. Networking

```typescript
const host = await app.network.createHost(hostOptions);
for await (const event of host.events()) {
  switch (event.type) {
    case "connected": break;
    case "received": break;
    case "disconnected": break;
  }
}
```

## 9. XR session

```typescript
const session = await app.xr.createSession({ renderer: app.renderer });
for await (const event of session.events()) {
  if (event.type === "frame") {
    const frame = session.beginFrame(event.predictedDisplayTime);
    const views = frame.locateViews();
    // Render each active view.
    frame.finish();
  }
}
```

## 10. Common type families

| Category | Public types |
|---|---|
| Vectors | `Vec2`, `Vec3`, `Vec4`, float/double/integer variants |
| Matrices and transforms | `Mat22`, `Mat33`, `Mat44`, `DMat44`, `Quat`, `Basis` |
| Geometry | `AABB2`, `AABB3`, `OBB2`, `OBB3`, `Plane`, `Rect`, `Triangle` |
| Colour | `Color` with explicit component range and storage precision |
| Containers | language-native lists/maps/sets or typed Moss containers when allocator semantics matter |
| Results and references | target-language result/error types and explicit owned/borrowed resource wrappers |
| Jobs and synchronization | job system, thread pool, barrier, mutex, semaphore and scoped locks |

## 11. Resource-management rules

Every native-backed object exposes `dispose()` and optionally `Symbol.dispose`/`Symbol.asyncDispose` where supported. A resource retains its owning device/context and must be disposed before that owner. Do not expose numeric WASM pointers. Typed arrays are copied for uploads by default; an explicit mapped-buffer API documents when a view aliases WASM memory. Callbacks are detached before their JS closures are released.

## 12. Error-handling rules

Throw `MossError` for failed operations; use `null`/`undefined` only for expected absence. `Moss.create()` and browser/network asset loading return `Promise`. Keep render, input and in-memory resource calls synchronous unless the underlying browser API requires asynchronous work. Use `bigint` for exact 64-bit native integers.

## 13. Feature and platform checks

- A declaration in the reference does not mean every backend implements it. Check the selected GPU backend, platform, XR runtime and optional networking/codec features.
- Keep GPU resources associated with the device that created them; release child resources before the device.
- Pair renderer frame begin/end, GUI begin/end, render-pass begin/end and XR frame begin/end operations. Prefer scoped/structured helpers so early returns cannot leave a subsystem unbalanced.
- Poll platform and network events at the cadence required by the subsystem. Do not block a browser/UI thread waiting for asynchronous I/O.
- Use the API Reference's subsystem and header inventory when looking for advanced Navigation, Geometry, Physics, Resources, Skeleton or Variants APIs.

## 14. Where to look next

See [API_Reference.md](API_Reference.md) for the full native symbol crosswalk, target-language signatures, resource models, threading rules, feature coverage and the complete 270-header / 19-inline-header inventory.
