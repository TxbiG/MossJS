# MossJS API Reference

> **Complete-parity public API design for JavaScript / TypeScript.** This document specifies the intended idiomatic API when the binding exposes the full Moss feature set. It is a language-facing design contract, not a statement that every projected member is implemented in the current wrapper.

## 1. API design principles

Moss C++/CMoss is the canonical feature and behaviour baseline. The language binding is a **native TypeScript library**, not a mechanical translation of C++ identifiers. The FFI may use C-compatible handles and exported `Moss_*` symbols internally, but ordinary users should not see those details.

The public surface is an ES module with first-class TypeScript declarations. Use lowerCamelCase functions, PascalCase resource classes, discriminated unions, typed arrays and browser-native objects. WebAssembly startup and browser asset I/O are asynchronous; ordinary in-memory operations remain synchronous.

The binding must preserve feature coverage and semantics, not the number or spelling of native functions. Several native calls may become one managed method, a property, an iterator, a scoped object or a language protocol. Conversely, an overloaded native function may become distinct named methods when the language does not support overloads cleanly.

### Non-negotiable rules

- Keep the complete native feature set: Core, Platform, GPU, Renderer, Audio, GUI, Physics, Networking, XR, Navigation, Geometry, Resources, Skeleton, TriangleSplitter and Variants.
- Expose resource objects rather than asking users to pass opaque pointers repeatedly.
- Hide allocator plumbing, user-data pointers, C calling conventions, struct packing and raw WASM/JNI/P/Invoke handles behind the binding layer.
- Preserve status-query meaning. `false`, `0`, an empty collection and an absent optional value are not interchangeable with an error.
- Use asynchronous operations only when the target runtime requires them. WebAssembly startup and browser fetches are asynchronous; ordinary native API calls are not made asynchronous merely for stylistic consistency.
- Document platform/backend feature gates. A declared API is not proof that a backend or optional transport is available in a particular build.

## 2. Package and module layout

```typescript
import { Moss, GpuDeviceDesc, GpuBufferDesc, GpuBufferUsage, type WindowOptions } from "moss";
```

### Subsystem map

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

The public package/module root should follow the repository's established package metadata. The names in this document are the recommended public design; implementation namespaces can be adjusted to the actual package identifier without changing the member design.

## 3. Core language type system

| Native Moss concept | TypeScript public representation | Design requirement |
|---|---|---|
| `bool` | `boolean` | Keep boolean status and query values boolean. |
| Fixed-width signed/unsigned integers | `number (bigint for exact 64-bit values)` | Preserve width and unsigned semantics at the ABI boundary. |
| `size_t` | `number` | Must be large enough for the host architecture. |
| `float` / `double` | `number` | Do not narrow double precision silently. |
| UTF-8 text | `string` | Copy or borrow according to documented native lifetime; validate encoding. |
| Opaque native resource | `typed class with dispose()` | Never expose raw pointers as normal user-facing values. |
| Pointer + element count | `TypedArray / ArrayBufferView` | Validate length, element type and retention duration. |
| Function pointer + user data | `function closure retained until unregister` | Unregister before callback state is released. |

### Value types and descriptors

Vectors, matrices, quaternions, colours, bounds, planes, rectangles and descriptor types should be real TypeScript values. Use value structs/records where copy semantics are cheap and safe; use resource classes only for objects with identity or native lifetime. Descriptor objects should have named fields/options instead of long sequences of positional primitives. Validate invalid enum combinations before crossing FFI.

## 4. Public resource model

| Moss native resource family | Recommended public type | Ownership model |
|---|---|---|
| Window / monitor / gamepad / camera / capture / storage | `Window`, `Monitor`, `Gamepad`, `Camera`, `Capture`, `Storage` | The application owns platform state; each opened resource has explicit close/dispose semantics. |
| GPU device / buffer / texture / sampler / shader / pipeline / command buffer / fence / semaphore | `GpuDevice`, `GpuBuffer`, `GpuTexture`, `GpuSampler`, `GpuShader`, `GpuPipeline`, `GpuCommandBuffer`, `GpuFence`, `GpuSemaphore` | Child objects retain or borrow their creating device; destroy children before device shutdown. |
| Renderer / material / framebuffer / mesh / model / font / sprite / batch | `Renderer`, `Material`, `Framebuffer`, `Mesh`, `Model`, `Font`, `Sprite`, `SpriteBatch` | Renderer-owned resources cannot outlive the renderer unless explicitly reference-counted. |
| Audio source / stream / listener / effect / microphone | `AudioSource`, `AudioStream`, `AudioListener`, `AudioEffect`, `Microphone` | Release sources, streams and listeners before audio shutdown. |
| GUI context | `GuiContext` | Context scopes widget calls; begin/end pairs are balanced by scoped helpers or callbacks. |
| Physics system / body / shape / constraint / character / vehicle / soft body | `PhysicsSystem`, `Body`, `Shape`, `Constraint`, `Character`, `Vehicle`, `SoftBody` | System and job/allocator dependencies outlive bodies and simulation work. |
| Network host / peer / packet / HTTP request / WebSocket / WebRTC | `NetworkHost`, `Peer`, `Packet`, `HttpRequest`, `WebSocket`, `WebRtcConnection` | Explicit ownership of hosts, packets, requests and connections. |
| XR instance / session / action set / action / hand / body / face | `XrRuntime`, `XrSession`, `XrActionSet`, `XrAction`, `HandTracker`, `BodyTracker`, `FaceTracker` | Session-bound objects cannot outlive the session/runtime they depend on. |

The exact implementation may consolidate thin native objects where that is safer (for example, a renderer may own framebuffers or a device may own command pools). The public API must still make ownership unambiguous.

## 5. Lifecycle, errors, callbacks and concurrency

### Resource lifetime

Every native-backed object exposes `dispose()` and optionally `Symbol.dispose`/`Symbol.asyncDispose` where supported. A resource retains its owning device/context and must be disposed before that owner. Do not expose numeric WASM pointers. Typed arrays are copied for uploads by default; an explicit mapped-buffer API documents when a view aliases WASM memory. Callbacks are detached before their JS closures are released.

### Errors and results

Throw `MossError` for failed operations; use `null`/`undefined` only for expected absence. `Moss.create()` and browser/network asset loading return `Promise`. Keep render, input and in-memory resource calls synchronous unless the underlying browser API requires asynchronous work. Use `bigint` for exact 64-bit native integers.

### Threading and callbacks

- State which thread owns window/event pumping, renderer submission, audio callbacks, physics updates and XR frame operations.
- Do not promise thread safety unless the native subsystem guarantees it.
- Keep callback state alive until native unregistration completes; no callback may run after disposal/close returns.
- Never allow a managed exception, panic or language-runtime longjmp to unwind through a C ABI frame.
- For asynchronous APIs, define cancellation, completion and disposal behaviour. Do not return a view over memory that is freed when the Promise/callback completes.

## 6. Idiomatic quick-start patterns

### Platform and event loop

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

### GPU resource creation

```typescript
const device = await app.gpu.createDevice(new GpuDeviceDesc());
const buffer = device.createBuffer({
  size: vertices.byteLength,
  usage: GpuBufferUsage.Vertex,
});
buffer.upload(vertices); // Accepts ArrayBufferView / TypedArray.
// Dispose resources before disposing the owning device.
```

### Renderer frame

```typescript
const frame = app.renderer.beginFrame();
frame.setCamera(camera);
frame.drawSprite(sprite);
frame.drawText("Hello Moss", position, textStyle);
frame.finish();
```

### Audio playback

```typescript
const source = await app.audio.loadWav("/assets/theme.wav");
source.play();
app.audio.update(deltaTime);
source.dispose();
```

### GUI context and widgets

```typescript
const ui = app.gui.createContext();
ui.panel("Inspector", panel => {
  panel.text("Player");
  const enabled = panel.checkbox("Enabled", currentValue);
});
```

### Physics world

```typescript
const physics = app.physics.createSystem(settings);
const body = physics.bodies.create({
  shape: Shape.box(size),
  motionType: MotionType.Dynamic,
});
physics.step(deltaTime);
const contacts = physics.query.contacts(body);
```

### Networking

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

### XR session

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

The examples show the intended public shape and lifecycle rules. Names and overloads in the index below are the parity mapping target; implementation-specific initialization parameters can be added to the relevant options objects without exposing the FFI.

## 7. Complete public API projection index

The following table maps every exported callable/allocator callback in the supplied CMoss declaration catalogue to a proposed TypeScript public entry point. The native name is included **only for traceability**. It is not the public name that developers should call. Target signatures use idiomatic types and hide receiver/context handles where those belong to an object.

- **1063 indexed entries:** 1,058 callable/constructor declarations and five allocator callback values.
- Duplicate native overloads are retained as separate rows where their parameters differ; the public API should resolve each to a distinct overload or a disambiguated method name.
- Native pointer/out-parameter patterns should become owned resource values, spans/slices, tuples/records or callback objects as appropriate to the language.

### `Moss_Audio.h` → `audio`

| Native symbol (traceability) | Public API projection | Target-language signature | Kind |
|---|---|---|---|
| `Moss_Init_Audio` | `audio.initialize(...)` | `audio.initialize(): void` | function |
| `Moss_Terminate_Audio` | `audio.shutdown(...)` | `audio.shutdown(): void` | function |
| `Moss_AudioUpdate` | `audio.update(...)` | `audio.update(deltaTime: number): void` | function |
| `Moss_AudioLoadWavFile` | `audio.loadWavFile(...)` | `audio.loadWavFile(url: string): Promise<AudioSource>` | function |
| `Moss_AudioLoadWav` | `audio.loadWav(...)` | `audio.loadWav(): AudioSource` | function |
| `Moss_AudioLoadOgg` | `audio.loadOgg(...)` | `audio.loadOgg(filename: string, type: AudioLoadType): AudioSource` | function |
| `Moss_AudioLoadMP3` | `audio.loadMp3(...)` | `audio.loadMp3(filename: string): AudioSource` | function |
| `Moss_AudioSourceDestroy` | `AudioSource.dispose(...)` | `AudioSource.dispose(): void` | function |
| `Moss_AudioCaptureMicrophone` | `audio.captureMicrophone(...)` | `audio.captureMicrophone(microphone: Microphone): AudioSource` | function |
| `Moss_AudioCreateEffect` (overload 1) | `audio.createEffect(...)` | `audio.createEffect(type: AudioEffectType): AudioEffect` | function |
| `Moss_AudioCreateEffect` (overload 2) | `AudioEffect.setParameter(...)` | `AudioEffect.setParameter(paramName: string, value: number): void` | function |
| `Moss_AudioRemoveEffect` | `AudioEffect.dispose(...)` | `AudioEffect.dispose(): void` | function |
| `Moss_AudioCreateChannel` | `audio.createChannel(...)` | `audio.createChannel(channel: ChannelId): ChannelId` | function |
| `Audio_RemoveChannel` | `audio.removeChannel(...)` | `audio.removeChannel(channel: ChannelId): void` | function |
| `Moss_AudioGetMasterChannel` | `audio.getMasterChannel(...)` | `audio.getMasterChannel(): ChannelId` | function |
| `Moss_AudioSetChannelVolume` | `audio.setChannelVolume(...)` | `audio.setChannelVolume(channel: ChannelId, volume: number): void` | function |
| `Moss_AudioSetChannelMute` | `audio.setChannelMute(...)` | `audio.setChannelMute(channel: ChannelId, mute: boolean): void` | function |
| `Moss_AudioAddChannelEffect` | `audio.addEffectToChannel(...)` | `audio.addEffectToChannel(channel: ChannelId, effect: AudioEffect): void` | function |
| `Moss_AudioRemoveChannelEffect` | `audio.removeEffectFromChannel(...)` | `audio.removeEffectFromChannel(channel: ChannelId, effect: AudioEffect): void` | function |
| `Moss_AudioRemoveAllChannelEffects` | `audio.clearChannelEffects(...)` | `audio.clearChannelEffects(channel: ChannelId): void` | function |
| `Moss_AudioStreamCreate` | `audio.createStream(...)` | `audio.createStream(): AudioStream` | function |
| `Moss_AudioStreamPlay` | `AudioStream.play(...)` | `AudioStream.play(): void` | function |
| `Moss_AudioStreamStop` | `AudioStream.stop(...)` | `AudioStream.stop(): void` | function |
| `Moss_AudioStreamSetVolume` | `AudioStream.setVolume(...)` | `AudioStream.setVolume(volume: number): void` | function |
| `Moss_AudioStreamSetPitch` | `AudioStream.setPitch(...)` | `AudioStream.setPitch(pitch: number): void` | function |
| `Moss_AudioStreamSetPlaybackRate` | `AudioStream.setPlaybackRate(...)` | `AudioStream.setPlaybackRate(rate: number): void` | function |
| `Moss_AudioStreamSetPan` | `AudioStream.setPan(...)` | `AudioStream.setPan(pan: number): void` | function |
| `Moss_AudioStreamSetLoop` | `AudioStream.setLoop(...)` | `AudioStream.setLoop(loop: boolean): void` | function |
| `Moss_AudioStreamRemove` | `AudioStream.dispose(...)` | `AudioStream.dispose(): void` | function |
| `Moss_AudioStream2DCreate` | `audio.createStreamDimension2D(...)` | `audio.createStreamDimension2D(): AudioStreamDIMENSION2D` | function |
| `Moss_AudioStream2DPlay` | `AudioStreamDIMENSION2D.play(...)` | `AudioStreamDIMENSION2D.play(): void` | function |
| `Moss_AudioStream2DStop` | `AudioStreamDIMENSION2D.stop(...)` | `AudioStreamDIMENSION2D.stop(): void` | function |
| `Moss_AudioStream2DSetVolume` | `AudioStreamDIMENSION2D.setVolume(...)` | `AudioStreamDIMENSION2D.setVolume(volume: number): void` | function |
| `Moss_AudioStream2DSetPitch` | `AudioStreamDIMENSION2D.setPitch(...)` | `AudioStreamDIMENSION2D.setPitch(pitch: number): void` | function |
| `Moss_AudioStream2DSetPan` | `AudioStreamDIMENSION2D.setPan(...)` | `AudioStreamDIMENSION2D.setPan(pan: number): void` | function |
| `Moss_AudioStream2DSetLoop` | `AudioStreamDIMENSION2D.setLoop(...)` | `AudioStreamDIMENSION2D.setLoop(loop: boolean): void` | function |
| `Moss_AudioStream3DSetPosition` (overload 1) | `AudioStreamDIMENSION2D.streamDimension3SetPosition(...)` | `AudioStreamDIMENSION2D.streamDimension3SetPosition(): void` | function |
| `Moss_AudioStream3DSetVelocity` (overload 1) | `AudioStreamDIMENSION2D.streamDimension3SetVelocity(...)` | `AudioStreamDIMENSION2D.streamDimension3SetVelocity(): void` | function |
| `Moss_AudioStream3DSetMaxDistance` (overload 1) | `AudioStreamDIMENSION2D.streamDimension3SetMaxDistance(...)` | `AudioStreamDIMENSION2D.streamDimension3SetMaxDistance(): void` | function |
| `Moss_AudioStream2DRemove` | `AudioStreamDIMENSION2D.dispose(...)` | `AudioStreamDIMENSION2D.dispose(): void` | function |
| `Moss_AudioStream3DCreate` | `audio.createStreamDimension3D(...)` | `audio.createStreamDimension3D(): AudioStreamDIMENSION3D` | function |
| `Moss_AudioStream3DPlay` | `AudioStreamDIMENSION3D.play(...)` | `AudioStreamDIMENSION3D.play(): void` | function |
| `Moss_AudioStream3DStop` | `AudioStreamDIMENSION3D.stop(...)` | `AudioStreamDIMENSION3D.stop(): void` | function |
| `Moss_AudioStream3DSetVolume` | `AudioStreamDIMENSION3D.setVolume(...)` | `AudioStreamDIMENSION3D.setVolume(volume: number): void` | function |
| `Moss_AudioStream3DSetPitch` | `AudioStreamDIMENSION3D.setPitch(...)` | `AudioStreamDIMENSION3D.setPitch(pitch: number): void` | function |
| `Moss_AudioStream3DSetPlaybackRate` | `AudioStreamDIMENSION3D.setPlaybackRate(...)` | `AudioStreamDIMENSION3D.setPlaybackRate(rate: number): void` | function |
| `Moss_AudioStream3DSetPan` | `AudioStreamDIMENSION3D.setPan(...)` | `AudioStreamDIMENSION3D.setPan(pan: number): void` | function |
| `Moss_AudioStream3DSetLoop` | `AudioStreamDIMENSION3D.setLoop(...)` | `AudioStreamDIMENSION3D.setLoop(loop: boolean): void` | function |
| `Moss_AudioStream3DSetPosition` (overload 2) | `AudioStreamDIMENSION3D.setPosition(...)` | `AudioStreamDIMENSION3D.setPosition(): void` | function |
| `Moss_AudioStream3DSetVelocity` (overload 2) | `AudioStreamDIMENSION3D.setVelocity(...)` | `AudioStreamDIMENSION3D.setVelocity(): void` | function |
| `Moss_AudioStream3DSetMaxDistance` (overload 2) | `AudioStreamDIMENSION3D.setMaxDistance(...)` | `AudioStreamDIMENSION3D.setMaxDistance(): void` | function |
| `Moss_AudioStream3DSetDistanceModel` | `AudioStreamDIMENSION3D.setDistanceModel(...)` | `AudioStreamDIMENSION3D.setDistanceModel(model: DistanceModel): void` | function |
| `Moss_AudioStream3DRemove` | `AudioStreamDIMENSION3D.dispose(...)` | `AudioStreamDIMENSION3D.dispose(): void` | function |
| `Moss_AudioCreateAudioListener2D` | `audio.createAudioListenerDimension2D(...)` | `audio.createAudioListenerDimension2D(position: Float2): AudioListenerDIMENSION2D` | function |
| `Moss_AudioCreateAudioListener3D` | `audio.createAudioListenerDimension3D(...)` | `audio.createAudioListenerDimension3D(position: Float3): AudioListenerDIMENSION3D` | function |
| `Moss_AudioCreateRayAudioListener2D` | `audio.createRayAudioListenerDimension2D(...)` | `audio.createRayAudioListenerDimension2D(position: Float2): RayAudioListenerDIMENSION2D` | function |
| `Moss_AudioCreateRayAudioListener3D` | `audio.createRayAudioListenerDimension3D(...)` | `audio.createRayAudioListenerDimension3D(position: Float3): RayAudioListenerDIMENSION3D` | function |
| `Moss_AudioRemoveAudioListener2D` | `AudioListenerDIMENSION2D.dispose(...)` | `AudioListenerDIMENSION2D.dispose(): void` | function |
| `Moss_AudioRemoveAudioListener3D` | `AudioListenerDIMENSION3D.dispose(...)` | `AudioListenerDIMENSION3D.dispose(): void` | function |
| `Moss_AudioRemoveRayAudioListener2D` | `RayAudioListenerDIMENSION2D.dispose(...)` | `RayAudioListenerDIMENSION2D.dispose(): void` | function |
| `Moss_AudioRemoveRayAudioListener3D` | `RayAudioListenerDIMENSION3D.dispose(...)` | `RayAudioListenerDIMENSION3D.dispose(): void` | function |
| `Moss_AudioActivateAudioListener2D` | `AudioListenerDIMENSION2D.activate(...)` | `AudioListenerDIMENSION2D.activate(activate: boolean): void` | function |
| `Moss_AudioActivateAudioListener3D` | `AudioListenerDIMENSION3D.activate(...)` | `AudioListenerDIMENSION3D.activate(activate: boolean): void` | function |
| `Moss_AudioActivateRayAudioListener2D` | `RayAudioListenerDIMENSION2D.activate(...)` | `RayAudioListenerDIMENSION2D.activate(activate: boolean): void` | function |
| `Moss_AudioActivateRayAudioListener3D` | `RayAudioListenerDIMENSION3D.activate(...)` | `RayAudioListenerDIMENSION3D.activate(activate: boolean): void` | function |
| `Moss_AudioListenerSetOrientation` | `AudioListenerDIMENSION3D.listenerSetOrientation(...)` | `AudioListenerDIMENSION3D.listenerSetOrientation(forward: Vec3, up: Vec3): void` | function |
| `Moss_AudioRayListener2DSetPhysics` | `RayAudioListenerDIMENSION2D.raySetPhysics(...)` | `RayAudioListenerDIMENSION2D.raySetPhysics(physics: PhysicsSystem, raycast: AudioRaycastDIMENSION2DCallback): void` | function |
| `Moss_AudioRayListener3DSetPhysics` | `RayAudioListenerDIMENSION3D.raySetPhysics(...)` | `RayAudioListenerDIMENSION3D.raySetPhysics(physics: PhysicsSystem, raycast: AudioRaycastDIMENSION3DCallback): void` | function |
| `Moss_AudioRayListener2DSetTraceSettings` | `RayAudioListenerDIMENSION2D.raySetTraceSettings(...)` | `RayAudioListenerDIMENSION2D.raySetTraceSettings(maxDistance: number, reflectionRays: number): void` | function |
| `Moss_AudioRayListener3DSetTraceSettings` | `RayAudioListenerDIMENSION3D.raySetTraceSettings(...)` | `RayAudioListenerDIMENSION3D.raySetTraceSettings(maxDistance: number, reflectionRays: number): void` | function |
| `Moss_AudioRayTrace2D` | `audio.rayTraceDimension2D(...)` | `audio.rayTraceDimension2D(desc: Readonly<AudioRayTraceDIMENSION2DDesc>, outResult: AudioRayTraceResult): boolean` | function |
| `Moss_AudioRayTrace3D` | `audio.rayTraceDimension3D(...)` | `audio.rayTraceDimension3D(desc: Readonly<AudioRayTraceDIMENSION3DDesc>, outResult: AudioRayTraceResult): boolean` | function |
| `Moss_AudioRayTraceFromListener2D` | `RayAudioListenerDIMENSION2D.rayTraceFrom(...)` | `RayAudioListenerDIMENSION2D.rayTraceFrom(sourcePosition: Readonly<Vec2>, outResult: AudioRayTraceResult): boolean` | function |
| `Moss_AudioRayTraceFromListener3D` | `RayAudioListenerDIMENSION3D.rayTraceFrom(...)` | `RayAudioListenerDIMENSION3D.rayTraceFrom(sourcePosition: Readonly<Vec3>, outResult: AudioRayTraceResult): boolean` | function |
| `Moss_AudioRayTraceComputeGain` | `AudioRayTraceResult.rayTraceComputeGain(...)` | `AudioRayTraceResult.rayTraceComputeGain(): number` | function |
| `Moss_IsSpeakerDeviceReady` | `audio.isSpeakerDeviceReady(...)` | `audio.isSpeakerDeviceReady(): boolean` | function |
| `Moss_AudioSpeakerOpen` | `audio.speakerOpen(...)` | `audio.speakerOpen(): void` | function |
| `Moss_AudioSpeakerPause` | `audio.speakerPause(...)` | `audio.speakerPause(): void` | function |
| `Moss_AudioSpeakerResume` | `audio.speakerResume(...)` | `audio.speakerResume(): void` | function |
| `Moss_AudioSpeakerIsPaused` | `audio.speakerIsPaused(...)` | `audio.speakerIsPaused(): boolean` | function |
| `Moss_AudioSelectSpeakerDevice` | `audio.selectSpeakerDevice(...)` | `audio.selectSpeakerDevice(id: number): boolean` | function |
| `Moss_GetCurrentSpeakerDeviceID` | `audio.getCurrentSpeakerDeviceId(...)` | `audio.getCurrentSpeakerDeviceId(): number` | function |
| `Moss_GetSpeakerDeviceName` | `audio.getSpeakerDeviceName(...)` | `audio.getSpeakerDeviceName(id: number): string \| null` | function |
| `Moss_ListSpeakerDevices` | `audio.listSpeakerDevices(...)` | `audio.listSpeakerDevices(): number` | function |
| `Moss_MicrophoneGetDeviceCount` | `audio.microphoneGetDeviceCount(...)` | `audio.microphoneGetDeviceCount(): number` | function |
| `Moss_MicrophoneGetDeviceName` | `audio.microphoneGetDeviceName(...)` | `audio.microphoneGetDeviceName(deviceIndex: number): string \| null` | function |
| `Moss_MicrophoneOpen` | `audio.microphoneOpen(...)` | `audio.microphoneOpen(desc: Readonly<MicrophoneDesc>): Microphone` | function |
| `Moss_MicrophoneClose` | `Microphone.dispose(...)` | `Microphone.dispose(): void` | function |
| `Moss_MicrophoneStart` | `Microphone.start(...)` | `Microphone.start(): boolean` | function |
| `Moss_MicrophoneStop` | `Microphone.stop(...)` | `Microphone.stop(): void` | function |
| `Moss_MicrophoneRead` | `Microphone.read(...)` | `Microphone.read(outSamples: number[], maxFrames: number): number` | function |
| `Moss_MicrophoneSetGain` | `Microphone.setGain(...)` | `Microphone.setGain(gain: number): void` | function |
| `Moss_MicrophoneGetSampleRate` | `Microphone.getSampleRate(...)` | `Microphone.getSampleRate(): number` | function |
| `Moss_MicrophoneGetChannels` | `Microphone.getChannels(...)` | `Microphone.getChannels(): number` | function |
| `Moss_MicrophoneSetCallback` | `Microphone.setCallback(...)` | `Microphone.setCallback(callback: MicrophoneCallback): void` | function |
| `Moss_MicrophoneGetLevels` | `Microphone.getLevels(...)` | `Microphone.getLevels(): MicrophoneLevels` | function |
| `Moss_MicrophoneGetLevelRMS` | `Microphone.getLevelRms(...)` | `Microphone.getLevelRms(): number` | function |
| `Moss_MicrophoneGetLevelPeak` | `Microphone.getLevelPeak(...)` | `Microphone.getLevelPeak(): number` | function |
| `Moss_MicrophoneGetSmoothedVolume` | `Microphone.getSmoothedVolume(...)` | `Microphone.getSmoothedVolume(): number` | function |
| `Moss_MicrophoneGetVoiceActivity` | `Microphone.getVoiceActivity(...)` | `Microphone.getVoiceActivity(): number` | function |
| `Moss_IsMicrophoneDeviceReady` | `audio.isMicrophoneDeviceReady(...)` | `audio.isMicrophoneDeviceReady(): boolean` | function |
| `Moss_AudioMicrophoneOpen` | `audio.microphoneOpen(...)` | `audio.microphoneOpen(): number` | function |
| `Moss_AudioMicrophoneClose` | `audio.microphoneClose(...)` | `audio.microphoneClose(): void` | function |
| `Moss_AudioMicrophonePlay` | `audio.microphonePlay(...)` | `audio.microphonePlay(): void` | function |
| `Moss_AudioMicrophoneStop` | `audio.microphoneStop(...)` | `audio.microphoneStop(): void` | function |
| `Moss_AudioMicrophoneID` | `audio.microphoneId(...)` | `audio.microphoneId(): number` | function |
| `Moss_AudioSelectMicrophoneDevice` | `audio.selectMicrophoneDevice(...)` | `audio.selectMicrophoneDevice(id: number): boolean` | function |
| `Moss_GetMicrophoneDeviceName` | `audio.getMicrophoneDeviceName(...)` | `audio.getMicrophoneDeviceName(index: number): string \| null` | function |
| `Moss_ListMicrophoneDevices` | `audio.listMicrophoneDevices(...)` | `audio.listMicrophoneDevices(): number` | function |
| `Moss_AudioMicrophoneSetGain` | `Microphone.setGain(...)` | `Microphone.setGain(gain: number): void` | function |
| `Moss_AudioMicrophoneGetSampleRate` | `Microphone.getSampleRate(...)` | `Microphone.getSampleRate(): number` | function |
| `Moss_AudioMicrophoneGetChannels` | `Microphone.getChannels(...)` | `Microphone.getChannels(): number` | function |
| `Moss_AudioStreamSetCallback` | `AudioStream.setCallback(...)` | `AudioStream.setCallback(callback: AudioStreamCallback): void` | function |

### `Moss_GPU.h` → `gpu`

| Native symbol (traceability) | Public API projection | Target-language signature | Kind |
|---|---|---|---|
| `Moss_AcquireGPUCommandBuffer` | `GpuDevice.acquireCommandBuffer(...)` | `GpuDevice.acquireCommandBuffer(): GpuCommandBuffer` | function |
| `Moss_BeginGPUComputePass` | `GpuCommandBuffer.beginComputePass(...)` | `GpuCommandBuffer.beginComputePass(): void` | function |
| `Moss_BeginGPUCopyPass` | `GpuCommandBuffer.beginCopyPass(...)` | `GpuCommandBuffer.beginCopyPass(): void` | function |
| `Moss_CreateGPUBuffer` | `GpuDevice.createBuffer(...)` | `GpuDevice.createBuffer(desc: Readonly<GpuBufferDesc>): GpuBuffer` | function |
| `Moss_DestroyGPUDevice` | `GpuDevice.dispose(...)` | `GpuDevice.dispose(): void` | function |
| `Moss_BeginGPURenderPass` | `GpuCommandBuffer.beginRenderPass(...)` | `GpuCommandBuffer.beginRenderPass(framebuffer: Framebuffer): void` | function |
| `Moss_BindGPUComputeSamplers` | `GpuCommandBuffer.bindComputeSamplers(...)` | `GpuCommandBuffer.bindComputeSamplers(firstSlot: number, samplers: ReadonlyArray<GpuSampler*>): void` | function |
| `Moss_BindGPUComputeTextures` | `GpuCommandBuffer.bindComputeTextures(...)` | `GpuCommandBuffer.bindComputeTextures(firstSlot: number, textures: ReadonlyArray<GpuTexture*>): void` | function |
| `Moss_BindGPUComputeUniformBuffers` | `GpuCommandBuffer.bindComputeUniformBuffers(...)` | `GpuCommandBuffer.bindComputeUniformBuffers(firstSlot: number, bindings: ReadonlyArray<GpuBufferBinding>): void` | function |
| `Moss_BindGPUFragmentSamplers` | `GpuCommandBuffer.bindFragmentSamplers(...)` | `GpuCommandBuffer.bindFragmentSamplers(firstSlot: number, samplers: ReadonlyArray<GpuSampler*>): void` | function |
| `Moss_BindGPUFragmentTextures` | `GpuCommandBuffer.bindFragmentTextures(...)` | `GpuCommandBuffer.bindFragmentTextures(firstSlot: number, textures: ReadonlyArray<GpuTexture*>): void` | function |
| `Moss_BindGPUFragmentUniformBuffers` | `GpuCommandBuffer.bindFragmentUniformBuffers(...)` | `GpuCommandBuffer.bindFragmentUniformBuffers(firstSlot: number, bindings: ReadonlyArray<GpuBufferBinding>): void` | function |
| `Moss_BindGPUIndexBuffer` | `GpuCommandBuffer.bindIndex(...)` | `GpuCommandBuffer.bindIndex(buffer: GpuBuffer, offset: bigint, indexType: GpuIndexType): void` | function |
| `Moss_BindGPUPipeline` | `GpuCommandBuffer.bindPipeline(...)` | `GpuCommandBuffer.bindPipeline(pipeline: PipelineState): void` | function |
| `Moss_BindGPUResourceSet` | `GpuCommandBuffer.bindResourceSet(...)` | `GpuCommandBuffer.bindResourceSet(setIndex: number, set: ResourceSet): void` | function |
| `Moss_BindGPUVertexBuffers` | `GpuCommandBuffer.bindVertexBuffers(...)` | `GpuCommandBuffer.bindVertexBuffers(firstBinding: number, bindings: ReadonlyArray<GpuBufferBinding>): void` | function |
| `Moss_BindGPUVertexSamplers` | `GpuCommandBuffer.bindVertexSamplers(...)` | `GpuCommandBuffer.bindVertexSamplers(firstSlot: number, samplers: ReadonlyArray<GpuSampler*>): void` | function |
| `Moss_BindGPUVertexTextures` | `GpuCommandBuffer.bindVertexTextures(...)` | `GpuCommandBuffer.bindVertexTextures(firstSlot: number, textures: ReadonlyArray<GpuTexture*>): void` | function |
| `Moss_BindGPUVertexUniformBuffers` | `GpuCommandBuffer.bindVertexUniformBuffers(...)` | `GpuCommandBuffer.bindVertexUniformBuffers(firstSlot: number, bindings: ReadonlyArray<GpuBufferBinding>): void` | function |
| `Moss_BlitGPUTexture` | `GpuCommandBuffer.blitTexture(...)` | `GpuCommandBuffer.blitTexture(src: Texture, dst: Texture, region: Readonly<GpuBlitRegion>): void` | function |
| `Moss_CalculateGPUTextureFormatSize` | `gpu.calculateTextureFormatSize(...)` | `gpu.calculateTextureFormatSize(format: TextureFormat, width: number, height: number, depth: number): number` | function |
| `Moss_CancelGPUCommandBuffer` | `GpuCommandBuffer.cancel(...)` | `GpuCommandBuffer.cancel(): void` | function |
| `Moss_CopyGPUBufferToBuffer` | `GpuCommandBuffer.copyToBuffer(...)` | `GpuCommandBuffer.copyToBuffer(src: GpuBuffer, dst: GpuBuffer, srcRegion: Readonly<GpuBufferRegion>, dstRegion: Readonly<GpuBufferRegion>): void` | function |
| `Moss_CopyGPUBufferToTexture` | `GpuCommandBuffer.copyToTexture(...)` | `GpuCommandBuffer.copyToTexture(src: GpuBuffer, dst: GpuTexture, srcRegion: Readonly<GpuBufferRegion>, dstRegion: Readonly<GpuTextureRegion>): void` | function |
| `Moss_CopyGPUTextureToBuffer` | `GpuCommandBuffer.copyTextureTo(...)` | `GpuCommandBuffer.copyTextureTo(src: GpuTexture, dst: GpuBuffer, srcRegion: Readonly<GpuTextureRegion>, dstRegion: Readonly<GpuBufferRegion>): void` | function |
| `Moss_CopyGPUTextureToTexture` | `GpuCommandBuffer.copyTextureToTexture(...)` | `GpuCommandBuffer.copyTextureToTexture(src: GpuTexture, dst: GpuTexture, srcRegion: Readonly<GpuTextureRegion>, dstRegion: Readonly<GpuTextureRegion>): void` | function |
| `Moss_CreateGPUSampler` | `GpuDevice.createSampler(...)` | `GpuDevice.createSampler(createInfo: Readonly<GpuSamplerCreateInfo>): GpuSampler` | function |
| `Moss_CreateGPUShader` | `GpuDevice.createShader(...)` | `GpuDevice.createShader(createInfo: Readonly<GpuShaderCreateInfo>): GpuShader` | function |
| `Moss_CreateGPUComputePipeline` | `GpuDevice.createComputePipeline(...)` | `GpuDevice.createComputePipeline(createInfo: Readonly<GpuComputePipelineCreateInfo>): ComputePipelineState` | function |
| `Moss_CreateGPUTexture` | `GpuDevice.createTexture(...)` | `GpuDevice.createTexture(createInfo: Readonly<GpuTextureCreateInfo>): GpuTexture` | function |
| `Moss_CreateGPUTextureView` | `GpuDevice.createTextureView(...)` | `GpuDevice.createTextureView(createInfo: Readonly<GpuTextureViewCreateInfo>): GpuTextureView` | function |
| `Moss_CreateGPUTransferBuffer` | `GpuDevice.createTransferBuffer(...)` | `GpuDevice.createTransferBuffer(createInfo: Readonly<GpuTransferBufferCreateInfo>): GpuTransferBuffer` | function |
| `Moss_CreateGPUFramebuffer` | `GpuDevice.createFramebuffer(...)` | `GpuDevice.createFramebuffer(createInfo: Readonly<GpuFramebufferCreateInfo>): Framebuffer` | function |
| `Moss_CreateGPUResourceSetLayout` | `GpuDevice.createResourceSetLayout(...)` | `GpuDevice.createResourceSetLayout(createInfo: Readonly<GpuResourceSetLayoutCreateInfo>): ResourceSetLayout` | function |
| `Moss_CreateGPUResourceSet` | `GpuDevice.createResourceSet(...)` | `GpuDevice.createResourceSet(createInfo: Readonly<GpuResourceSetCreateInfo>): ResourceSet` | function |
| `Moss_CreateGPUQueryPool` | `GpuDevice.createQueryPool(...)` | `GpuDevice.createQueryPool(createInfo: Readonly<GpuQueryPoolCreateInfo>): GpuQueryPool` | function |
| `Moss_DispatchGPUCompute` | `GpuCommandBuffer.dispatchCompute(...)` | `GpuCommandBuffer.dispatchCompute(groupCountX: number, groupCountY: number, groupCountZ: number): void` | function |
| `Moss_DispatchGPUComputeIndirect` | `GpuCommandBuffer.dispatchComputeIndirect(...)` | `GpuCommandBuffer.dispatchComputeIndirect(indirectBuffer: GpuBuffer, offset: bigint): void` | function |
| `Moss_DownloadFromGPUBuffer` | `GpuBuffer.downloadFrom(...)` | `GpuBuffer.downloadFrom(dstData: Uint8Array, size: bigint, srcOffset: bigint): void` | function |
| `Moss_DownloadFromGPUTexture` | `GpuTexture.downloadFrom(...)` | `GpuTexture.downloadFrom(srcRegion: Readonly<GpuTextureRegion>, dstData: Uint8Array, dstRowPitch: number, dstSlicePitch: number): void` | function |
| `Moss_DrawGPUIndexedPrimitives` | `GpuCommandBuffer.drawIndexedPrimitives(...)` | `GpuCommandBuffer.drawIndexedPrimitives(indexCount: number, instanceCount: number, firstIndex: number, vertexOffset: number, firstInstance: number): void` | function |
| `Moss_DrawGPUIndexedPrimitivesIndirect` | `GpuCommandBuffer.drawIndexedPrimitivesIndirect(...)` | `GpuCommandBuffer.drawIndexedPrimitivesIndirect(indirectBuffer: GpuBuffer, offset: bigint, drawCount: number, stride: number): void` | function |
| `Moss_DrawGPUPrimitives` | `GpuCommandBuffer.drawPrimitives(...)` | `GpuCommandBuffer.drawPrimitives(vertexCount: number, instanceCount: number, firstVertex: number, firstInstance: number): void` | function |
| `Moss_DrawGPUPrimitivesIndirect` | `GpuCommandBuffer.drawPrimitivesIndirect(...)` | `GpuCommandBuffer.drawPrimitivesIndirect(indirectBuffer: GpuBuffer, offset: bigint, drawCount: number, stride: number): void` | function |
| `Moss_EndGPUComputePass` | `GpuCommandBuffer.endComputePass(...)` | `GpuCommandBuffer.endComputePass(): void` | function |
| `Moss_EndGPUCopyPass` | `GpuCommandBuffer.endCopyPass(...)` | `GpuCommandBuffer.endCopyPass(): void` | function |
| `Moss_EndGPURenderPass` | `GpuCommandBuffer.endRenderPass(...)` | `GpuCommandBuffer.endRenderPass(): void` | function |
| `Moss_GDKResumeGPU` | `GpuDevice.gdkResume(...)` | `GpuDevice.gdkResume(): void` | function |
| `Moss_GDKSuspendGPU` | `GpuDevice.gdkSuspend(...)` | `GpuDevice.gdkSuspend(): void` | function |
| `Moss_GenerateMipmapsForGPUTexture` | `GpuCommandBuffer.generateMipmapsForTexture(...)` | `GpuCommandBuffer.generateMipmapsForTexture(texture: GpuTexture): void` | function |
| `Moss_GetGPUDriver` | `gpu.getDriver(...)` | `gpu.getDriver(driverIndex: number): string \| null` | function |
| `Moss_GetCompiledGPUBackendType` | `gpu.getCompiledBackendType(...)` | `gpu.getCompiledBackendType(): GpuBackendType` | function |
| `Moss_GetGPUDeviceBackendType` | `GpuDevice.getBackendType(...)` | `GpuDevice.getBackendType(): GpuBackendType` | function |
| `Moss_GPUBackendGetName` | `gpu.backendGetName(...)` | `gpu.backendGetName(backend: GpuBackendType): string \| null` | function |
| `Moss_GPUBackendGetShaderFormats` | `gpu.backendGetShaderFormats(...)` | `gpu.backendGetShaderFormats(backend: GpuBackendType): number` | function |
| `Moss_GPUBackendIsCompiled` | `gpu.backendIsCompiled(...)` | `gpu.backendIsCompiled(backend: GpuBackendType): boolean` | function |
| `Moss_GPUBackendIsSupportedOnPlatform` | `gpu.backendIsSupportedOnPlatform(...)` | `gpu.backendIsSupportedOnPlatform(backend: GpuBackendType): boolean` | function |
| `Moss_GetGPUBackendInfo` | `gpu.getBackendInfo(...)` | `gpu.getBackendInfo(backend: GpuBackendType, outInfo: GpuBackendInfo): boolean` | function |
| `Moss_RegisterNativeGPUBackend` | `gpu.registerNativeBackend(...)` | `gpu.registerNativeBackend(backend: GpuBackendType, nativeTable: Uint8Array): boolean` | function |
| `Moss_HasNativeGPUBackend` | `gpu.hasNativeBackend(...)` | `gpu.hasNativeBackend(backend: GpuBackendType): boolean` | function |
| `Moss_GetGPUShaderFormats` | `GpuDevice.getShaderFormats(...)` | `GpuDevice.getShaderFormats(): number` | function |
| `Moss_GetGPUTextureFormatFromPixelFormat` | `gpu.getTextureFormatFromPixelFormat(...)` | `gpu.getTextureFormatFromPixelFormat(pixelFormat: PixelFormat): TextureFormat` | function |
| `Moss_GetNumGPUDrivers` | `gpu.getNumDrivers(...)` | `gpu.getNumDrivers(): number` | function |
| `Moss_GetPixelFormatFromGPUTextureFormat` | `gpu.getPixelFormatFromTextureFormat(...)` | `gpu.getPixelFormatFromTextureFormat(textureFormat: TextureFormat): PixelFormat` | function |
| `Moss_GPUSupportsProperties` | `GpuDevice.supportsProperties(...)` | `GpuDevice.supportsProperties(): boolean` | function |
| `Moss_GPUSupportsShaderFormats` | `GpuDevice.supportsShaderFormats(...)` | `GpuDevice.supportsShaderFormats(shaderFormatMask: number): boolean` | function |
| `Moss_GPUTextureFormatTexelBlockSize` | `gpu.textureFormatTexelBlockSize(...)` | `gpu.textureFormatTexelBlockSize(format: TextureFormat): number` | function |
| `Moss_GPUTextureSupportsFormat` | `GpuDevice.textureSupportsFormat(...)` | `GpuDevice.textureSupportsFormat(format: TextureFormat, usage: TextureUsage): boolean` | function |
| `Moss_GPUTextureSupportsSampleCount` | `GpuDevice.textureSupportsSampleCount(...)` | `GpuDevice.textureSupportsSampleCount(format: TextureFormat, sampleCount: number): boolean` | function |
| `Moss_InsertGPUDebugLabel` | `GpuCommandBuffer.insertDebugLabel(...)` | `GpuCommandBuffer.insertDebugLabel(label: string): void` | function |
| `Moss_MapGPUTransferBuffer` | `GpuTransferBuffer.map(...)` | `GpuTransferBuffer.map(): Uint8Array` | function |
| `Moss_PopGPUDebugGroup` | `GpuCommandBuffer.popDebugGroup(...)` | `GpuCommandBuffer.popDebugGroup(): void` | function |
| `Moss_PushGPUComputeUniformData` | `GpuCommandBuffer.pushComputeUniformData(...)` | `GpuCommandBuffer.pushComputeUniformData(slot: number, data: Uint8Array, size: number): void` | function |
| `Moss_PushGPUDebugGroup` | `GpuCommandBuffer.pushDebugGroup(...)` | `GpuCommandBuffer.pushDebugGroup(label: string): void` | function |
| `Moss_PushGPUFragmentUniformData` | `GpuCommandBuffer.pushFragmentUniformData(...)` | `GpuCommandBuffer.pushFragmentUniformData(slot: number, data: Uint8Array, size: number): void` | function |
| `Moss_PushGPUVertexUniformData` | `GpuCommandBuffer.pushVertexUniformData(...)` | `GpuCommandBuffer.pushVertexUniformData(slot: number, data: Uint8Array, size: number): void` | function |
| `Moss_QueryGPUFence` | `GpuFence.query(...)` | `GpuFence.query(): boolean` | function |
| `Moss_ReleaseGPUBuffer` | `GpuBuffer.release(...)` | `GpuBuffer.release(): void` | function |
| `Moss_ReleaseGPUFence` | `GpuFence.release(...)` | `GpuFence.release(): void` | function |
| `Moss_ReleaseGPUSampler` | `GpuSampler.release(...)` | `GpuSampler.release(): void` | function |
| `Moss_ReleaseGPUShader` | `GpuShader.release(...)` | `GpuShader.release(): void` | function |
| `Moss_ReleaseGPUComputePipeline` | `GpuDevice.releaseComputePipeline(...)` | `GpuDevice.releaseComputePipeline(pipeline: ComputePipelineState): void` | function |
| `Moss_ReleaseGPUTexture` | `GpuTexture.release(...)` | `GpuTexture.release(): void` | function |
| `Moss_ReleaseGPUTextureView` | `GpuTextureView.release(...)` | `GpuTextureView.release(): void` | function |
| `Moss_ReleaseGPUTransferBuffer` | `GpuTransferBuffer.release(...)` | `GpuTransferBuffer.release(): void` | function |
| `Moss_ReleaseGPUFramebuffer` | `Framebuffer.release(...)` | `Framebuffer.release(): void` | function |
| `Moss_ReleaseGPUResourceSetLayout` | `ResourceSetLayout.release(...)` | `ResourceSetLayout.release(): void` | function |
| `Moss_ReleaseGPUResourceSet` | `ResourceSet.release(...)` | `ResourceSet.release(): void` | function |
| `Moss_ReleaseGPUQueryPool` | `GpuQueryPool.release(...)` | `GpuQueryPool.release(): void` | function |
| `Moss_SetGPUAllowedFramesInFlight` | `GpuDevice.setAllowedFramesInFlight(...)` | `GpuDevice.setAllowedFramesInFlight(framesInFlight: number): void` | function |
| `Moss_SetGPUBlendConstants` | `GpuCommandBuffer.setBlendConstants(...)` | `GpuCommandBuffer.setBlendConstants(r: number, g: number, b: number, a: number): void` | function |
| `Moss_SetGPUBufferName` | `GpuBuffer.setName(...)` | `GpuBuffer.setName(name: string): void` | function |
| `Moss_SetGPUScissor` (overload 1) | `GpuCommandBuffer.setScissor(...)` | `GpuCommandBuffer.setScissor(scissor: Readonly<Rect>): void` | function |
| `Moss_SetGPUStencilReference` | `GpuCommandBuffer.setStencilReference(...)` | `GpuCommandBuffer.setStencilReference(reference: number): void` | function |
| `Moss_SetGPUTextureName` | `GpuTexture.setName(...)` | `GpuTexture.setName(name: string): void` | function |
| `Moss_SetGPUViewport` | `GpuCommandBuffer.setViewport(...)` | `GpuCommandBuffer.setViewport(viewport: Readonly<GpuViewport>): void` | function |
| `Moss_SetGPUScissor` (overload 2) | `GpuCommandBuffer.setScissor(...)` | `GpuCommandBuffer.setScissor(scissors: Readonly<Rect>): void` | function |
| `Moss_SubmitGPUCommandBuffer` | `GpuCommandBuffer.submit(...)` | `GpuCommandBuffer.submit(): void` | function |
| `Moss_SubmitGPUCommandBufferAndAcquireFence` | `GpuCommandBuffer.submitAndAcquireFence(...)` | `GpuCommandBuffer.submitAndAcquireFence(): GpuFence` | function |
| `Moss_BindGPUComputePipeline` | `GpuCommandBuffer.bindComputePipeline(...)` | `GpuCommandBuffer.bindComputePipeline(pipeline: ComputePipelineState): void` | function |
| `Moss_BarrierGPUResources` | `GpuCommandBuffer.barrierResources(...)` | `GpuCommandBuffer.barrierResources(barrierInfo: Readonly<GpuBarrierInfo>): void` | function |
| `Moss_ResetGPUQueryPool` | `GpuCommandBuffer.resetQueryPool(...)` | `GpuCommandBuffer.resetQueryPool(queryPool: GpuQueryPool, firstQuery: number, queryCount: number): void` | function |
| `Moss_WriteGPUTimestamp` | `GpuCommandBuffer.writeTimestamp(...)` | `GpuCommandBuffer.writeTimestamp(queryPool: GpuQueryPool, queryIndex: number): void` | function |
| `Moss_BeginGPUQuery` | `GpuCommandBuffer.beginQuery(...)` | `GpuCommandBuffer.beginQuery(queryPool: GpuQueryPool, queryIndex: number): void` | function |
| `Moss_EndGPUQuery` | `GpuCommandBuffer.endQuery(...)` | `GpuCommandBuffer.endQuery(queryPool: GpuQueryPool, queryIndex: number): void` | function |
| `Moss_GetGPUQueryResults` | `GpuQueryPool.getQueryResults(...)` | `GpuQueryPool.getQueryResults(firstQuery: number, queryCount: number, data: Uint8Array, dataSize: bigint, stride: bigint, flags: GpuQueryResultFlags): boolean` | function |
| `Moss_TransitionGPUBuffer` | `GpuCommandBuffer.transition(...)` | `GpuCommandBuffer.transition(buffer: GpuBuffer, oldState: ResourceState, newState: ResourceState): void` | function |
| `Moss_TransitionGPUTexture` | `GpuCommandBuffer.transitionTexture(...)` | `GpuCommandBuffer.transitionTexture(texture: GpuTexture, oldState: ResourceState, newState: ResourceState): void` | function |
| `Moss_UnmapGPUTransferBuffer` | `GpuTransferBuffer.unmap(...)` | `GpuTransferBuffer.unmap(): void` | function |
| `Moss_UploadToGPUBuffer` | `GpuBuffer.uploadTo(...)` | `GpuBuffer.uploadTo(srcData: Uint8Array, size: bigint, dstOffset: bigint): void` | function |
| `Moss_UploadToGPUTexture` | `GpuTexture.uploadTo(...)` | `GpuTexture.uploadTo(transferInfo: Readonly<GpuTextureTransferInfo>): void` | function |
| `Moss_WaitForGPUFences` | `GpuDevice.waitForFences(...)` | `GpuDevice.waitForFences(fences: ReadonlyArray<GpuFence*>, waitAll: boolean, timeoutNs: bigint): void` | function |
| `Moss_WaitForGPUIdle` | `GpuDevice.waitForIdle(...)` | `GpuDevice.waitForIdle(): void` | function |
| `Moss_WindowSupportsGPUPresentMode` | `Window.supportsPresentMode(...)` | `Window.supportsPresentMode(presentMode: GpuPresentMode): boolean` | function |
| `Moss_GPUBlitInfo` | `new GpuBlitInfo(...)` | `new GpuBlitInfo()` | constructor |
| `Moss_GPUBlitRegion` | `new GpuBlitRegion(...)` | `new GpuBlitRegion()` | constructor |
| `Moss_GPUBufferBinding` | `new GpuBufferBinding(...)` | `new GpuBufferBinding()` | constructor |
| `Moss_GPUBufferCreateInfo` | `new GpuBufferCreateInfo(...)` | `new GpuBufferCreateInfo()` | constructor |
| `Moss_GPUBufferLocation` | `new GpuBufferLocation(...)` | `new GpuBufferLocation()` | constructor |
| `Moss_GPUBufferRegion` | `new GpuBufferRegion(...)` | `new GpuBufferRegion()` | constructor |
| `Moss_GPUColorTargetBlendState` | `new GpuColorTargetBlendState(...)` | `new GpuColorTargetBlendState()` | constructor |
| `Moss_GPUColorTargetDescription` | `new GpuColorTargetDescription(...)` | `new GpuColorTargetDescription()` | constructor |
| `Moss_GPUColorTargetInfo` | `new GpuColorTargetInfo(...)` | `new GpuColorTargetInfo()` | constructor |
| `Moss_GPUDepthStencilState` | `new GpuDepthStencilState(...)` | `new GpuDepthStencilState()` | constructor |
| `Moss_GPUDepthStencilTargetInfo` | `new GpuDepthStencilTargetInfo(...)` | `new GpuDepthStencilTargetInfo()` | constructor |
| `Moss_GPUIndexedIndirectDrawCommand` | `new GpuIndexedIndirectDrawCommand(...)` | `new GpuIndexedIndirectDrawCommand()` | constructor |
| `Moss_GPUIndirectDispatchCommand` | `new GpuIndirectDispatchCommand(...)` | `new GpuIndirectDispatchCommand()` | constructor |
| `Moss_GPUIndirectDrawCommand` | `new GpuIndirectDrawCommand(...)` | `new GpuIndirectDrawCommand()` | constructor |
| `Moss_GPUMultisampleState` | `new GpuMultisampleState(...)` | `new GpuMultisampleState()` | constructor |
| `Moss_GPURasterizerState` | `new GpuRasterizerState(...)` | `new GpuRasterizerState()` | constructor |
| `Moss_GPUSamplerCreateInfo` | `new GpuSamplerCreateInfo(...)` | `new GpuSamplerCreateInfo()` | constructor |
| `Moss_GPUShaderCreateInfo` | `new GpuShaderCreateInfo(...)` | `new GpuShaderCreateInfo()` | constructor |
| `Moss_GPUStencilOpState` | `new GpuStencilOpState(...)` | `new GpuStencilOpState()` | constructor |
| `Moss_GPUTextureCreateInfo` | `new GpuTextureCreateInfo(...)` | `new GpuTextureCreateInfo()` | constructor |
| `Moss_GPUTextureLocation` | `new GpuTextureLocation(...)` | `new GpuTextureLocation()` | constructor |
| `Moss_GPUTextureRegion` | `new GpuTextureRegion(...)` | `new GpuTextureRegion()` | constructor |
| `Moss_GPUTextureSamplerBinding` | `new GpuTextureSamplerBinding(...)` | `new GpuTextureSamplerBinding()` | constructor |
| `Moss_GPUTextureTransferInfo` | `new GpuTextureTransferInfo(...)` | `new GpuTextureTransferInfo()` | constructor |
| `Moss_GPUTransferBufferCreateInfo` | `new GpuTransferBufferCreateInfo(...)` | `new GpuTransferBufferCreateInfo()` | constructor |
| `Moss_GPUTransferBufferLocation` | `new GpuTransferBufferLocation(...)` | `new GpuTransferBufferLocation()` | constructor |
| `Moss_GPUVertexAttribute` | `new GpuVertexAttribute(...)` | `new GpuVertexAttribute()` | constructor |
| `Moss_GPUVertexBufferDescription` | `new GpuVertexBufferDescription(...)` | `new GpuVertexBufferDescription()` | constructor |
| `Moss_GPUVertexInputState` | `new GpuVertexInputState(...)` | `new GpuVertexInputState()` | constructor |
| `Moss_GPUViewport` | `new GpuViewport(...)` | `new GpuViewport()` | constructor |
| `Moss_BindGPUComputeStorageBuffers` | `GpuCommandBuffer.bindComputeStorageBuffers(...)` | `GpuCommandBuffer.bindComputeStorageBuffers(firstSlot: number, bindings: ReadonlyArray<GpuStorageBufferReadWriteBinding>): void` | function |
| `Moss_BindGPUComputeStorageTextures` | `GpuCommandBuffer.bindComputeStorageTextures(...)` | `GpuCommandBuffer.bindComputeStorageTextures(firstSlot: number, bindings: ReadonlyArray<GpuStorageTextureReadWriteBinding>): void` | function |
| `Moss_BindGPUFragmentStorageBuffers` | `GpuCommandBuffer.bindFragmentStorageBuffers(...)` | `GpuCommandBuffer.bindFragmentStorageBuffers(firstSlot: number, bindings: ReadonlyArray<GpuStorageBufferReadWriteBinding>): void` | function |
| `Moss_BindGPUFragmentStorageTextures` | `GpuCommandBuffer.bindFragmentStorageTextures(...)` | `GpuCommandBuffer.bindFragmentStorageTextures(firstSlot: number, bindings: ReadonlyArray<GpuStorageTextureReadWriteBinding>): void` | function |
| `Moss_BindGPUVertexStorageBuffers` | `GpuCommandBuffer.bindVertexStorageBuffers(...)` | `GpuCommandBuffer.bindVertexStorageBuffers(firstSlot: number, bindings: ReadonlyArray<GpuStorageBufferReadWriteBinding>): void` | function |
| `Moss_BindGPUVertexStorageTextures` | `GpuCommandBuffer.bindVertexStorageTextures(...)` | `GpuCommandBuffer.bindVertexStorageTextures(firstSlot: number, bindings: ReadonlyArray<GpuStorageTextureReadWriteBinding>): void` | function |
| `Moss_GetGPUDeviceDriver` | `GpuDevice.getDriver(...)` | `GpuDevice.getDriver(): string \| null` | function |
| `Moss_GetGPUDeviceProperties` | `GpuDevice.getProperties(...)` | `GpuDevice.getProperties(): GpuDeviceProperties` | function |
| `Moss_ReleaseWindowFromGPUDevice` | `GpuDevice.releaseWindowFrom(...)` | `GpuDevice.releaseWindowFrom(window: Window): void` | function |
| `Moss_ClaimWindowForGPUDevice` | `GpuDevice.claimWindowFor(...)` | `GpuDevice.claimWindowFor(window: Window): boolean` | function |
| `Moss_UpdateGPUResourceSet` | `ResourceSet.update(...)` | `ResourceSet.update(bindings: ReadonlyArray<GpuResourceBinding>): void` | function |
| `Moss_CreateGPUDevice` | `gpu.createDevice(...)` | `gpu.createDevice(descriptor: Readonly<GpuDeviceDesc>): Promise<GpuDevice>` | function |
| `Moss_CreateGPUDeviceWithProperties` | `gpu.createDeviceWithProperties(...)` | `gpu.createDeviceWithProperties(descriptor: Readonly<GpuDeviceDesc>, requestedProperties: Readonly<GpuDeviceProperties>): Promise<GpuDevice>` | function |
| `Moss_GPUBufferCreate` | `GpuDevice.createBuffer(...)` | `GpuDevice.createBuffer(descriptor: GpuBufferDescriptor): GpuBuffer` | function |
| `Moss_GPUBufferUpload` | `GpuBuffer.upload(...)` | `GpuBuffer.upload(data: ArrayBufferView, offset?: bigint): void` | function |
| `Moss_GPUBufferMap` | `GpuBuffer.map(...)` | `GpuBuffer.map(): Uint8Array` | function |
| `Moss_GPUBufferUnmap` | `GpuBuffer.unmap(...)` | `GpuBuffer.unmap(): void` | function |
| `Moss_GPUBufferDestroy` | `GpuBuffer.dispose(...)` | `GpuBuffer.dispose(): void` | function |
| `Moss_AcquireGPUSwapchainTexture` | `GpuTexture.acquireSwapchain(...)` | `GpuTexture.acquireSwapchain(window: Window): boolean` | function |
| `Moss_AcquireGPUSwapchainTextureStatus` | `GpuTexture.acquireSwapchainStatus(...)` | `GpuTexture.acquireSwapchainStatus(window: Window): GpuSwapchainResult` | function |
| `Moss_GetGPUSwapchainTextureFormat` | `GpuDevice.getSwapchainTextureFormat(...)` | `GpuDevice.getSwapchainTextureFormat(window: Window): TextureFormat` | function |
| `Moss_SetGPUSwapchainParameters` | `GpuDevice.setSwapchainParameters(...)` | `GpuDevice.setSwapchainParameters(window: Window, width: number, height: number, presentMode: GpuPresentMode): boolean` | function |
| `Moss_ResizeGPUSwapchain` | `GpuDevice.resizeSwapchain(...)` | `GpuDevice.resizeSwapchain(window: Window, width: number, height: number): GpuSwapchainResult` | function |
| `Moss_WaitAndAcquireGPUSwapchainTexture` | `GpuTexture.waitAndAcquireSwapchain(...)` | `GpuTexture.waitAndAcquireSwapchain(window: Window, timeoutNs: bigint): boolean` | function |
| `Moss_WaitAndAcquireGPUSwapchainTextureStatus` | `GpuTexture.waitAndAcquireSwapchainStatus(...)` | `GpuTexture.waitAndAcquireSwapchainStatus(window: Window, timeoutNs: bigint): GpuSwapchainResult` | function |
| `Moss_WaitForGPUSwapchain` | `GpuDevice.waitForSwapchain(...)` | `GpuDevice.waitForSwapchain(window: Window, timeoutNs: bigint): boolean` | function |
| `Moss_WindowSupportsGPUSwapchainComposition` | `Window.supportsSwapchainComposition(...)` | `Window.supportsSwapchainComposition(): boolean` | function |
| `Moss_PresentGPUSwapchain` | `GpuDevice.presentSwapchain(...)` | `GpuDevice.presentSwapchain(window: Window, texture: GpuTexture): boolean` | function |
| `Moss_PresentGPUSwapchainStatus` | `GpuDevice.presentSwapchainStatus(...)` | `GpuDevice.presentSwapchainStatus(window: Window, texture: GpuTexture): GpuSwapchainResult` | function |
| `Moss_GPUClear` | `GpuCommandBuffer.clear(...)` | `GpuCommandBuffer.clear(r: number, g: number, b: number, a: number): void` | function |
| `Moss_GPUSamplerCreate` | `GpuDevice.createSampler(...)` | `GpuDevice.createSampler(desc: Readonly<GpuSamplerDesc>): GpuSampler` | function |
| `Moss_GPUSamplerDestroy` | `GpuSampler.dispose(...)` | `GpuSampler.dispose(): void` | function |
| `Moss_TextureCreate` | `GpuDevice.createTexture(...)` | `GpuDevice.createTexture(desc: Readonly<TextureDesc>): Texture` | function |
| `Moss_TextureDestroy` | `Texture.dispose(...)` | `Texture.dispose(): void` | function |
| `Moss_TextureUpload` | `Texture.upload(...)` | `Texture.upload(desc: Readonly<TextureUploadDesc>): void` | function |
| `Moss_TextureGenerateMips` | `Texture.generateMips(...)` | `Texture.generateMips(): void` | function |
| `Moss_GPUGetExternalFeatureMask` | `gpu.getExternalFeatureMask(...)` | `gpu.getExternalFeatureMask(): number` | function |
| `Moss_GPUExternalFeatureAvailable` | `gpu.externalFeatureAvailable(...)` | `gpu.externalFeatureAvailable(feature: GpuExternalFeature): boolean` | function |
| `Moss_GPUGetExternalFeatures` | `gpu.getExternalFeatures(...)` | `gpu.getExternalFeatures(outFeatures: GpuExternalFeatures): void` | function |
| `Moss_GPUExternalFeatureName` | `gpu.externalFeatureName(...)` | `gpu.externalFeatureName(feature: GpuExternalFeature): string \| null` | function |
| `Moss_FSR1BuildEasuConstants` | `gpu.fsr1BuildEasuConstants(...)` | `gpu.fsr1BuildEasuConstants(outConstants16: number, inputViewportWidth: number, inputViewportHeight: number, inputWidth: number, inputHeight: number, outputWidth: number, outputHeight: number): void` | function |
| `Moss_FSR2GetUpscaleRatio` | `gpu.fsr2GetUpscaleRatio(...)` | `gpu.fsr2GetUpscaleRatio(quality: FSR2QualityMode): number` | function |
| `Moss_FSR2GetRenderResolution` | `gpu.fsr2GetRenderResolution(...)` | `gpu.fsr2GetRenderResolution(displayWidth: number, displayHeight: number, quality: FSR2QualityMode, outResolution: FSR2RenderResolution): boolean` | function |
| `Moss_FSR2GetJitterPhaseCount` | `gpu.fsr2GetJitterPhaseCount(...)` | `gpu.fsr2GetJitterPhaseCount(renderWidth: number, displayWidth: number): number` | function |
| `Moss_FSR2GetJitterOffset` | `gpu.fsr2GetJitterOffset(...)` | `gpu.fsr2GetJitterOffset(outX: number[], outY: number[], index: number, phaseCount: number): boolean` | function |
| `Moss_MeshGenerateVertexRemap` | `gpu.meshGenerateVertexRemap(...)` | `gpu.meshGenerateVertexRemap(destination: number[], indices: ReadonlyArray<Uint32T>, vertices: Uint8Array, vertexCount: number, vertexSize: number): number` | function |
| `Moss_MeshOptimizeVertexCache` | `gpu.meshOptimizeVertexCache(...)` | `gpu.meshOptimizeVertexCache(destination: number[], indices: ReadonlyArray<Uint32T>, vertexCount: number): boolean` | function |
| `Moss_MeshOptimizeOverdraw` | `gpu.meshOptimizeOverdraw(...)` | `gpu.meshOptimizeOverdraw(destination: number[], indices: ReadonlyArray<Uint32T>, vertexPositions: ReadonlyArray<Float>, vertexPositionsStride: number, threshold: number): boolean` | function |
| `Moss_MeshOptimizeVertexFetch` | `gpu.meshOptimizeVertexFetch(...)` | `gpu.meshOptimizeVertexFetch(destination: Uint8Array, indices: number[], indexCount: number, vertices: Uint8Array, vertexCount: number, vertexSize: number): number` | function |
| `Moss_SMOLVEncode` | `gpu.smolvEncode(...)` | `gpu.smolvEncode(spirvData: Uint8Array, spirvSize: number, outSmolvData: Uint8Array, inoutSmolvSize: number[], flags: number): boolean` | function |
| `Moss_SMOLVGetDecodedSize` | `gpu.smolvGetDecodedSize(...)` | `gpu.smolvGetDecodedSize(smolvData: Uint8Array, smolvSize: number): number` | function |
| `Moss_SMOLVDecode` | `gpu.smolvDecode(...)` | `gpu.smolvDecode(smolvData: Uint8Array, smolvSize: number, outSpirvData: Uint8Array, spirvSize: number, flags: number): boolean` | function |
| `Moss_GPUVulkanMemoryAllocatorAvailable` | `gpu.memoryAllocatorAvailable(...)` | `gpu.memoryAllocatorAvailable(): boolean` | function |
| `Moss_GPUClothSolverCreate` | `GpuDevice.createClothSolver(...)` | `GpuDevice.createClothSolver(desc: Readonly<GpuClothSolverDesc>): GpuClothSolver` | function |
| `Moss_GPUClothSolverDestroy` | `GpuClothSolver.dispose(...)` | `GpuClothSolver.dispose(): void` | function |
| `Moss_GPUClothSolverDispatch` | `GpuCommandBuffer.clothSolverDispatch(...)` | `GpuCommandBuffer.clothSolverDispatch(solver: GpuClothSolver, desc: Readonly<GpuClothDispatchDesc>): boolean` | function |
| `Moss_GPUCommandBufferBegin` | `GpuDevice.createCommandBuffer(...)` | `GpuDevice.createCommandBuffer(queue: CommandQueue): GpuCommandBuffer` | function |
| `Moss_GPUCommandBufferEnd` | `GpuCommandBuffer.end(...)` | `GpuCommandBuffer.end(): void` | function |
| `Moss_GPUCommandBufferSubmit` | `GpuCommandBuffer.submit(...)` | `GpuCommandBuffer.submit(): void` | function |
| `Moss_CmdBeginRenderPass` | `GpuCommandBuffer.cmdBeginRenderPass(...)` | `GpuCommandBuffer.cmdBeginRenderPass(fb: Framebuffer): void` | function |
| `Moss_CmdEndRenderPass` | `GpuCommandBuffer.cmdEndRenderPass(...)` | `GpuCommandBuffer.cmdEndRenderPass(): void` | function |
| `Moss_CmdBindPipeline` | `GpuCommandBuffer.cmdBindPipeline(...)` | `GpuCommandBuffer.cmdBindPipeline(pipeline: PipelineState): void` | function |
| `Moss_CmdBindResourceSet` | `GpuCommandBuffer.cmdBindResourceSet(...)` | `GpuCommandBuffer.cmdBindResourceSet(setIndex: number, set: ResourceSet): void` | function |
| `Moss_CmdBindVertexBuffer` | `GpuCommandBuffer.cmdBindVertex(...)` | `GpuCommandBuffer.cmdBindVertex(buffer: GpuBuffer, offset: bigint): void` | function |
| `Moss_CmdBindIndexBuffer` | `GpuCommandBuffer.cmdBindIndex(...)` | `GpuCommandBuffer.cmdBindIndex(buffer: GpuBuffer, offset: bigint): void` | function |
| `Moss_CmdDraw` | `GpuCommandBuffer.cmdDraw(...)` | `GpuCommandBuffer.cmdDraw(vertexCount: number, firstVertex: number): void` | function |
| `Moss_CmdDrawIndexed` | `GpuCommandBuffer.cmdDrawIndexed(...)` | `GpuCommandBuffer.cmdDrawIndexed(indexCount: number, firstIndex: number, vertexOffset: number): void` | function |
| `Moss_CmdDispatch` | `GpuCommandBuffer.cmdDispatch(...)` | `GpuCommandBuffer.cmdDispatch(x: number, y: number, z: number): void` | function |
| `Moss_RGPassReadTexture` | `RGPass.readTexture(...)` | `RGPass.readTexture(texture: RGTexture, state: ResourceState): RGTexture` | function |
| `Moss_RGPassWriteTexture` | `RGPass.writeTexture(...)` | `RGPass.writeTexture(texture: RGTexture, state: ResourceState): RGTexture` | function |
| `Moss_GPUDeviceSupportsAsyncCompute` | `GpuDevice.supportsAsyncCompute(...)` | `GpuDevice.supportsAsyncCompute(): number` | function |
| `Moss_TextureSetResidency` | `Texture.setResidency(...)` | `Texture.setResidency(state: ResidencyState): void` | function |
| `Moss_ShaderCreate` | `GpuDevice.createShader(...)` | `GpuDevice.createShader(desc: Readonly<ShaderDesc>): Shader` | function |
| `Moss_ShaderDestroy` | `Shader.dispose(...)` | `Shader.dispose(): void` | function |
| `Moss_BindlessRegisterTexture` | `GpuDevice.bindlessRegisterTexture(...)` | `GpuDevice.bindlessRegisterTexture(texture: Uint8Array): BindlessHandle` | function |
| `Moss_BindlessRegisterBuffer` | `GpuBuffer.bindlessRegister(...)` | `GpuBuffer.bindlessRegister(): BindlessHandle` | function |
| `Moss_BindlessUnregister` | `GpuDevice.bindlessUnregister(...)` | `GpuDevice.bindlessUnregister(handle: BindlessHandle): void` | function |
| `Moss_PipelineCreate` | `GpuDevice.createPipeline(...)` | `GpuDevice.createPipeline(desc: Readonly<PipelineDesc>): PipelineState` | function |
| `Moss_PipelineDestroy` | `GpuDevice.pipelineDestroy(...)` | `GpuDevice.pipelineDestroy(pipeline: PipelineState): void` | function |
| `Moss_WindowResizeVK` | `gpu.windowResizeVk(...)` | `gpu.windowResizeVk(physicalDevice: VkPhysicalDevice, device: VkDevice, swapchain: VkSwapchainKHR, swapchainExtent: VkExtentDIMENSION2D, swapchainFramebuffers: StdVector<VkFramebuffer>, swapchainImageViews: StdVector<VkImageView>, depthImage: VkImage, depthImageMemory: VkDeviceMemory, depthImageView: VkImageView): void` | function |
| `Moss_GPUFatalError` | `gpu.fatalError(...)` | `gpu.fatalError(result: MossResult): void` | function |
| `Moss_SetGPUViewports` | `GpuCommandBuffer.setViewports(...)` | `GpuCommandBuffer.setViewports(viewports: ReadonlyArray<GpuViewport>): void` | function |
| `Moss_SetGPUScissors` | `GpuCommandBuffer.setScissors(...)` | `GpuCommandBuffer.setScissors(scissors: ReadonlyArray<Rect>): void` | function |
| `Moss_ClearGPUTexture` | `GpuCommandBuffer.clearTexture(...)` | `GpuCommandBuffer.clearTexture(texture: GpuTexture, color: number): void` | function |
| `Moss_ClearGPUDepthStencilTexture` | `GpuCommandBuffer.clearDepthStencilTexture(...)` | `GpuCommandBuffer.clearDepthStencilTexture(texture: GpuTexture, depth: number, stencil: number): void` | function |
| `Moss_DrawGPUIndexedPrimitivesIndirectCount` | `GpuCommandBuffer.drawIndexedPrimitivesIndirectCount(...)` | `GpuCommandBuffer.drawIndexedPrimitivesIndirectCount(indirectBuffer: GpuBuffer, offset: bigint, countBuffer: GpuBuffer, countOffset: bigint, maxDrawCount: number, stride: number): void` | function |
| `Moss_DrawGPUPrimitivesIndirectCount` | `GpuCommandBuffer.drawPrimitivesIndirectCount(...)` | `GpuCommandBuffer.drawPrimitivesIndirectCount(indirectBuffer: GpuBuffer, offset: bigint, countBuffer: GpuBuffer, countOffset: bigint, maxDrawCount: number, stride: number): void` | function |
| `Moss_SetGPUDepthBias` | `GpuCommandBuffer.setDepthBias(...)` | `GpuCommandBuffer.setDepthBias(constantFactor: number, clamp: number, slopeFactor: number): void` | function |
| `Moss_CreateGPUSemaphore` | `GpuDevice.createSemaphore(...)` | `GpuDevice.createSemaphore(): GpuSemaphore` | function |
| `Moss_ReleaseGPUSemaphore` | `GpuSemaphore.release(...)` | `GpuSemaphore.release(): void` | function |
| `Moss_SignalGPUSemaphoreValue` | `GpuSemaphore.signalValue(...)` | `GpuSemaphore.signalValue(value: bigint): void` | function |
| `Moss_WaitGPUSemaphoreValue` | `GpuSemaphore.waitValue(...)` | `GpuSemaphore.waitValue(value: bigint, timeoutNs: bigint): boolean` | function |
| `Moss_SubmitGPUCommandBufferWithSemaphores` | `GpuCommandBuffer.submitWithSemaphores(...)` | `GpuCommandBuffer.submitWithSemaphores(waitSemaphores: Readonly<GpuSemaphore>, waitValues: ReadonlyArray<Uint64T>, signalSemaphores: Readonly<GpuSemaphore>, signalValues: ReadonlyArray<Uint64T>): void` | function |
| `Moss_GetGPUBindlessCapacity` | `GpuDevice.getBindlessCapacity(...)` | `GpuDevice.getBindlessCapacity(type: ShaderResourceType): number` | function |
| `Moss_CreateGPUPipelineCache` | `GpuDevice.createPipelineCache(...)` | `GpuDevice.createPipelineCache(initialData: Uint8Array, initialSize: number): GpuPipelineCache` | function |
| `Moss_GetGPUPipelineCacheData` | `GpuPipelineCache.getData(...)` | `GpuPipelineCache.getData(outData: Uint8Array, outSize: number): number` | function |
| `Moss_ReleaseGPUPipelineCache` | `GpuPipelineCache.release(...)` | `GpuPipelineCache.release(): void` | function |

### `Moss_GUI.h` → `gui`

| Native symbol (traceability) | Public API projection | Target-language signature | Kind |
|---|---|---|---|
| `CreateContext` | `gui.createContext(...)` | `gui.createContext(sharedFontAtlas?: FontAtlas): GuiContext` | function |
| `DestroyContext` | `GuiContext.dispose(...)` | `GuiContext.dispose(): void` | function |
| `GetCurrentContext` | `gui.getCurrentContext(...)` | `gui.getCurrentContext(): GuiContext` | function |
| `SetCurrentContext` | `GuiContext.setCurrent(...)` | `GuiContext.setCurrent(): void` | function |
| `BeginWindow` | `GuiContext.beginWindow(...)` | `GuiContext.beginWindow(name: string, pOpen?: boolean[], flags?: GuiWindowFlags): boolean` | function |
| `EndWindow` | `GuiContext.endWindow(...)` | `GuiContext.endWindow(): void` | function |
| `BeginWindowChild` (overload 1) | `GuiContext.beginWindowChild(...)` | `GuiContext.beginWindowChild(strId: string, size?: Float2, childFlags?: GuiChildFlags, windowFlags?: GuiWindowFlags): boolean` | function |
| `BeginWindowChild` (overload 2) | `GuiContext.beginWindowChild(...)` | `GuiContext.beginWindowChild(id: number, size?: Float2, childFlags?: GuiChildFlags, windowFlags?: GuiWindowFlags): boolean` | function |
| `EndWindowChild` | `GuiContext.endWindowChild(...)` | `GuiContext.endWindowChild(): void` | function |
| `BeginContainer` | `GuiContext.beginContainer(...)` | `GuiContext.beginContainer(rect: Rect): void` | function |
| `EndContainer` | `GuiContext.endContainer(...)` | `GuiContext.endContainer(): void` | function |
| `BeginVContainer` | `GuiContext.beginVContainer(...)` | `GuiContext.beginVContainer(rect: Rect, spacing?: number): void` | function |
| `EndVContainer` | `GuiContext.endVContainer(...)` | `GuiContext.endVContainer(): void` | function |
| `BeginHContainer` | `GuiContext.beginHContainer(...)` | `GuiContext.beginHContainer(rect: Rect, spacing?: number): void` | function |
| `EndHContainer` | `GuiContext.endHContainer(...)` | `GuiContext.endHContainer(): void` | function |
| `BeginCenterContainer` | `GuiContext.beginCenterContainer(...)` | `GuiContext.beginCenterContainer(rect: Rect): void` | function |
| `EndCenterContainer` | `GuiContext.endCenterContainer(...)` | `GuiContext.endCenterContainer(): void` | function |
| `BeginMarginContainer` | `GuiContext.beginMarginContainer(...)` | `GuiContext.beginMarginContainer(leftMargin?: number, rightMargin?: number, topMargin?: number, bottomMargin?: number): void` | function |
| `EndMarginContainer` | `GuiContext.endMarginContainer(...)` | `GuiContext.endMarginContainer(): void` | function |
| `BeginVScrollContainer` | `GuiContext.beginVScrollContainer(...)` | `GuiContext.beginVScrollContainer(): void` | function |
| `EndVScrollContainer` | `GuiContext.endVScrollContainer(...)` | `GuiContext.endVScrollContainer(): void` | function |
| `BeginHScrollContainer` | `GuiContext.beginHScrollContainer(...)` | `GuiContext.beginHScrollContainer(): void` | function |
| `EndHScrollContainer` | `GuiContext.endHScrollContainer(...)` | `GuiContext.endHScrollContainer(): void` | function |
| `BeginGridContainer` | `GuiContext.beginGridContainer(...)` | `GuiContext.beginGridContainer(): void` | function |
| `EndGridContainer` | `GuiContext.endGridContainer(...)` | `GuiContext.endGridContainer(): void` | function |
| `ColorRect` | `GuiContext.colorRect(...)` | `GuiContext.colorRect(color: Color, size: Float2): void` | function |
| `Text` | `GuiContext.text(...)` | `GuiContext.text(fmt: string, args: ...): void` | function |
| `RichText` | `GuiContext.richText(...)` | `GuiContext.richText(label: string): void` | function |
| `ProgressBar` | `GuiContext.progressBar(...)` | `GuiContext.progressBar(): void` | function |
| `TextLink` | `GuiContext.textLink(...)` | `GuiContext.textLink(label: string): boolean` | function |
| `TextLinkOpenURL` | `GuiContext.textLinkOpenUrl(...)` | `GuiContext.textLinkOpenUrl(label: string, url?: string): void` | function |
| `Button` | `GuiContext.button(...)` | `GuiContext.button(label: string, rectPtr: Readonly<Rect>, vpMatrix: Mat44, defaultWidth?: number, defaultHeight?: number): boolean` | function |
| `ToggleSwitch` | `GuiContext.toggleSwitch(...)` | `GuiContext.toggleSwitch(active: boolean): boolean` | function |
| `RadioButton` | `GuiContext.radioButton(...)` | `GuiContext.radioButton(label: string, active: boolean): boolean` | function |
| `Checkbox` | `GuiContext.checkbox(...)` | `GuiContext.checkbox(label: string, value: boolean[], rectPtr: Readonly<Rect>, vpMatrix: Mat44, defaultSize?: number): boolean` | function |
| `SliderFloat` | `GuiContext.sliderFloat(...)` | `GuiContext.sliderFloat(minValue: number, maxValue: number, value: number): void` | function |
| `SliderDouble` | `GuiContext.sliderDouble(...)` | `GuiContext.sliderDouble(minValue: number, maxValue: number, value: number): void` | function |
| `SliderInt` | `GuiContext.sliderInt(...)` | `GuiContext.sliderInt(minValue: number, maxValue: number, value: number): void` | function |
| `SliderFloat2` | `GuiContext.sliderFloat2(...)` | `GuiContext.sliderFloat2(minValue: Float2, maxValue: Float2, value: Float2): void` | function |
| `SliderDouble2` | `GuiContext.sliderDouble2(...)` | `GuiContext.sliderDouble2(minValue: Double2, maxValue: Double2, value: Double2): void` | function |
| `SliderInt2` | `GuiContext.sliderInt2(...)` | `GuiContext.sliderInt2(minValue: Int2, maxValue: Int2, value: Int2): void` | function |
| `SliderFloat3` | `GuiContext.sliderFloat3(...)` | `GuiContext.sliderFloat3(minValue: Float3, maxValue: Float3, value: Float3): void` | function |
| `SliderDouble3` | `GuiContext.sliderDouble3(...)` | `GuiContext.sliderDouble3(minValue: Double3, maxValue: Double3, value: Double3): void` | function |
| `SliderInt3` | `GuiContext.sliderInt3(...)` | `GuiContext.sliderInt3(minValue: Int3, maxValue: Int3, value: Int3): void` | function |
| `SliderFloat4` | `GuiContext.sliderFloat4(...)` | `GuiContext.sliderFloat4(minValue: Float4, maxValue: Float4, value: Float4): void` | function |
| `SliderDouble4` | `GuiContext.sliderDouble4(...)` | `GuiContext.sliderDouble4(minValue: Double4, maxValue: Double4, value: Double4): void` | function |
| `SliderInt4` | `GuiContext.sliderInt4(...)` | `GuiContext.sliderInt4(minValue: Int4, maxValue: Int4, value: Int4): void` | function |
| `DragFloat` | `GuiContext.dragFloat(...)` | `GuiContext.dragFloat(label: string, value: number[], vSpeed?: number, vMin?: number, vMax?: number, format?: string, flags?: GuiSliderFlags): boolean` | function |
| `DragFloat2` | `GuiContext.dragFloat2(...)` | `GuiContext.dragFloat2(label: string, value: Float2, vSpeed?: number, vMin?: number, vMax?: number, format?: string, flags?: GuiSliderFlags): boolean` | function |
| `DragFloat3` | `GuiContext.dragFloat3(...)` | `GuiContext.dragFloat3(label: string, value: Float3, vSpeed?: number, vMin?: number, vMax?: number, format?: string, flags?: GuiSliderFlags): boolean` | function |
| `DragFloat4` | `GuiContext.dragFloat4(...)` | `GuiContext.dragFloat4(label: string, value: Float4, vSpeed?: number, vMin?: number, vMax?: number, format?: string, flags?: GuiSliderFlags): boolean` | function |
| `DragFloatRange2` | `GuiContext.dragFloatRange2(...)` | `GuiContext.dragFloatRange2(label: string, vCurrentMin: number[], vCurrentMax: number[], vSpeed?: number, vMin?: number, vMax?: number, format?: string, formatMax?: string, flags?: GuiSliderFlags): boolean` | function |
| `DragInt` | `GuiContext.dragInt(...)` | `GuiContext.dragInt(label: string, v: number[], vSpeed?: number, vMin?: number, vMax?: number, format?: string, flags?: GuiSliderFlags): boolean` | function |
| `DragInt2` | `GuiContext.dragInt2(...)` | `GuiContext.dragInt2(label: string, v: number, vSpeed?: number, vMin?: number, vMax?: number, format?: string, flags?: GuiSliderFlags): boolean` | function |
| `DragInt3` | `GuiContext.dragInt3(...)` | `GuiContext.dragInt3(label: string, v: number, vSpeed?: number, vMin?: number, vMax?: number, format?: string, flags?: GuiSliderFlags): boolean` | function |
| `DragInt4` | `GuiContext.dragInt4(...)` | `GuiContext.dragInt4(label: string, v: number, vSpeed?: number, vMin?: number, vMax?: number, format?: string, flags?: GuiSliderFlags): boolean` | function |
| `DragIntRange2` | `GuiContext.dragIntRange2(...)` | `GuiContext.dragIntRange2(label: string, vCurrentMin: number[], vCurrentMax: number[], vSpeed?: number, vMin?: number, vMax?: number, format?: string, formatMax?: string, flags?: GuiSliderFlags): boolean` | function |
| `DragScalar` | `GuiContext.dragScalar(...)` | `GuiContext.dragScalar(label: string, dataType: GuiDataType, pData: Uint8Array, vSpeed?: number, pMin?: Uint8Array, pMax?: Uint8Array, format?: string, flags?: GuiSliderFlags): boolean` | function |
| `DragScalarN` | `GuiContext.dragScalarN(...)` | `GuiContext.dragScalarN(label: string, dataType: GuiDataType, pData: Uint8Array, components: number, vSpeed?: number, pMin?: Uint8Array, pMax?: Uint8Array, format?: string, flags?: GuiSliderFlags): boolean` | function |
| `InputText` | `GuiContext.inputText(...)` | `GuiContext.inputText(label: string, buf: Uint8Array, bufSize: number, flags?: GuiInputTextFlags, callback?: GuiInputTextCallback): boolean` | function |
| `InputTextMultiline` | `GuiContext.inputTextMultiline(...)` | `GuiContext.inputTextMultiline(label: string, buf: Uint8Array, bufSize: number, size?: Float2, flags?: GuiInputTextFlags, callback?: GuiInputTextCallback): boolean` | function |
| `InputTextWithHint` | `GuiContext.inputTextWithHint(...)` | `GuiContext.inputTextWithHint(label: string, hint: string, buf: Uint8Array, bufSize: number, flags?: GuiInputTextFlags, callback?: GuiInputTextCallback): boolean` | function |
| `InputFloat` | `GuiContext.inputFloat(...)` | `GuiContext.inputFloat(label: string, v: number[], step?: number, stepFast?: number, format?: string, flags?: GuiInputTextFlags): boolean` | function |
| `InputFloat2` | `GuiContext.inputFloat2(...)` | `GuiContext.inputFloat2(label: string, v: number, format?: string, flags?: GuiInputTextFlags): boolean` | function |
| `InputFloat3` | `GuiContext.inputFloat3(...)` | `GuiContext.inputFloat3(label: string, v: number, format?: string, flags?: GuiInputTextFlags): boolean` | function |
| `InputFloat4` | `GuiContext.inputFloat4(...)` | `GuiContext.inputFloat4(label: string, v: number, format?: string, flags?: GuiInputTextFlags): boolean` | function |
| `InputInt` | `GuiContext.inputInt(...)` | `GuiContext.inputInt(label: string, v: number[], step?: number, stepFast?: number, flags?: GuiInputTextFlags): boolean` | function |
| `InputInt2` | `GuiContext.inputInt2(...)` | `GuiContext.inputInt2(label: string, v: number, flags?: GuiInputTextFlags): boolean` | function |
| `InputInt3` | `GuiContext.inputInt3(...)` | `GuiContext.inputInt3(label: string, v: number, flags?: GuiInputTextFlags): boolean` | function |
| `InputInt4` | `GuiContext.inputInt4(...)` | `GuiContext.inputInt4(label: string, v: number, flags?: GuiInputTextFlags): boolean` | function |
| `InputDouble` | `GuiContext.inputDouble(...)` | `GuiContext.inputDouble(label: string, v: number[], step?: number, stepFast?: number, format?: string, flags?: GuiInputTextFlags): boolean` | function |
| `InputScalar` | `GuiContext.inputScalar(...)` | `GuiContext.inputScalar(label: string, dataType: GuiDataType, pData: Uint8Array, pStep?: Uint8Array, pStepFast?: Uint8Array, format?: string, flags?: GuiInputTextFlags): boolean` | function |
| `InputScalarN` | `GuiContext.inputScalarN(...)` | `GuiContext.inputScalarN(label: string, dataType: GuiDataType, pData: Uint8Array, components: number, pStep?: Uint8Array, pStepFast?: Uint8Array, format?: string, flags?: GuiInputTextFlags): boolean` | function |

### `Moss_Network.h` → `network`

| Native symbol (traceability) | Public API projection | Target-language signature | Kind |
|---|---|---|---|
| `Moss_Init_Network` | `network.initialize(...)` | `network.initialize(): void` | function |
| `enet_initialize_with_callbacks` | `NetworkCallbacks.initializeWith(...)` | `NetworkCallbacks.initializeWith(): number` | function |
| `Moss_TerminateNetwork` | `network.shutdown(...)` | `network.shutdown(): void` | function |
| `enet_list_clear` | `NetworkList.clear(...)` | `NetworkList.clear(): void` | function |
| `enet_list_insert` | `network.listInsert(...)` | `network.listInsert(netListIterator: E, value: Uint8Array): NetworkListIterator` | function |
| `enet_list_remove` | `network.listRemove(...)` | `network.listRemove(netListIterator: E): Uint8Array` | function |
| `enet_list_move` | `network.listMove(...)` | `network.listMove(netListIterator: E, value: Uint8Array, value: Uint8Array): NetworkListIterator` | function |
| `enet_list_size` | `NetworkList.size(...)` | `NetworkList.size(): number` | function |
| `enet_time_get` | `network.timeGet(...)` | `network.timeGet(): number` | function |
| `enet_time_set` | `network.timeSet(...)` | `network.timeSet(int32T: U): void` | function |
| `enet_socket_create` | `network.createSocket(...)` | `network.createSocket(adressType: NetworkAddressType, socketType: NetworkSocketType): NetworkSocket` | function |
| `enet_socket_bind` | `NetworkAddress.socketBind(...)` | `NetworkAddress.socketBind(netSocket: E): number` | function |
| `enet_socket_get_address` | `NetworkAddress.socketGet(...)` | `NetworkAddress.socketGet(netSocket: E): number` | function |
| `enet_socket_listen` | `network.socketListen(...)` | `network.socketListen(netSocket: E, nt: I): number` | function |
| `enet_socket_accept` | `NetworkAddress.socketAccept(...)` | `NetworkAddress.socketAccept(netSocket: E): NetworkSocket` | function |
| `enet_socket_connect` | `NetworkAddress.socketConnect(...)` | `NetworkAddress.socketConnect(netSocket: E): number` | function |
| `enet_socket_send` | `NetworkAddress.socketSend(...)` | `NetworkAddress.socketSend(netSocket: E, value: Readonly<NetworkBuffer>, izeT: S): number` | function |
| `enet_socket_receive` | `NetworkAddress.socketReceive(...)` | `NetworkAddress.socketReceive(netSocket: E, value: NetworkBuffer, izeT: S): number` | function |
| `enet_socket_wait` | `network.socketWait(...)` | `network.socketWait(netSocket: E, value: number[], int32T: U): number` | function |
| `enet_socket_set_option` | `network.socketSetOption(...)` | `network.socketSetOption(netSocket: E, netSocketOption: E, nt: I): number` | function |
| `enet_socket_get_option` | `network.socketGetOption(...)` | `network.socketGetOption(netSocket: E, netSocketOption: E, value: number[]): number` | function |
| `enet_socket_shutdown` | `network.socketShutdown(...)` | `network.socketShutdown(netSocket: E, netSocketShutdown: E): number` | function |
| `enet_socket_destroy` | `network.socketDestroy(...)` | `network.socketDestroy(netSocket: E): void` | function |
| `enet_socketset_select` | `NetworkSocketSet.socketsetSelect(...)` | `NetworkSocketSet.socketsetSelect(netSocket: E, value: NetworkSocketSet, int32T: U): number` | function |
| `enet_address_set_host_ip` | `NetworkAddress.setHostIp(...)` | `NetworkAddress.setHostIp(hostName: string): number` | function |
| `enet_address_set_host` | `NetworkAddress.setHost(...)` | `NetworkAddress.setHost(type: NetworkAddressType, hostName: string): number` | function |
| `enet_address_get_host_ip` | `NetworkAddress.getHostIp(...)` | `NetworkAddress.getHostIp(hostName: Uint8Array, nameLength: number): number` | function |
| `enet_address_get_host` | `NetworkAddress.getHost(...)` | `NetworkAddress.getHost(hostName: Uint8Array, nameLength: number): number` | function |
| `enet_address_equal_host` | `NetworkAddress.equalHost(...)` | `NetworkAddress.equalHost(secondAddress: Readonly<NetworkAddress>): number` | function |
| `enet_address_is_broadcast` | `NetworkAddress.isBroadcast(...)` | `NetworkAddress.isBroadcast(): number` | function |
| `enet_address_build_any` | `NetworkAddress.buildAny(...)` | `NetworkAddress.buildAny(type: NetworkAddressType): void` | function |
| `enet_address_build_loopback` | `NetworkAddress.buildLoopback(...)` | `NetworkAddress.buildLoopback(type: NetworkAddressType): void` | function |
| `enet_address_convert_ipv6` | `NetworkAddress.convertIpv6(...)` | `NetworkAddress.convertIpv6(): void` | function |
| `enet_crc32` | `NetworkBuffer.crc32(...)` | `NetworkBuffer.crc32(izeT: S): number` | function |
| `enet_host_create` | `NetworkAddress.createHost(...)` | `NetworkAddress.createHost(type: NetworkAddressType, izeT: S, izeT: S, int32T: U, int32T: U): NetworkHost` | function |
| `enet_host_destroy` | `NetworkHost.dispose(...)` | `NetworkHost.dispose(): void` | function |
| `enet_host_check_events` | `NetworkHost.checkEvents(...)` | `NetworkHost.checkEvents(value: NetworkEvent): number` | function |
| `enet_host_service` | `NetworkHost.service(...)` | `NetworkHost.service(value: NetworkEvent, int32T: U): number` | function |
| `enet_host_flush` (overload 1) | `NetworkHost.flush(...)` | `NetworkHost.flush(): void` | function |
| `enet_host_broadcast` (overload 1) | `NetworkHost.broadcast(...)` | `NetworkHost.broadcast(int8T: U, value: NetworkPacket): void` | function |
| `enet_host_compress` | `NetworkHost.compress(...)` | `NetworkHost.compress(value: Readonly<NetworkCompressor>): void` | function |
| `enet_host_compress_with_range_coder` | `NetworkHost.compressWithRangeCoder(...)` | `NetworkHost.compressWithRangeCoder(): number` | function |
| `enet_host_channel_limit` | `NetworkHost.channelLimit(...)` | `NetworkHost.channelLimit(izeT: S): void` | function |
| `enet_host_bandwidth_limit` | `NetworkHost.bandwidthLimit(...)` | `NetworkHost.bandwidthLimit(int32T: U, int32T: U): void` | function |
| `enet_host_bandwidth_throttle` | `NetworkHost.bandwidthThrottle(...)` | `NetworkHost.bandwidthThrottle(): void` | function |
| `enet_host_random_seed` | `network.hostRandomSeed(...)` | `network.hostRandomSeed(): number` | function |
| `enet_host_random` | `NetworkHost.random(...)` | `NetworkHost.random(): number` | function |
| `enet_host_connect` (overload 1) | `NetworkHost.connect(...)` | `NetworkHost.connect(value: Readonly<NetworkAddress>, izeT: S, int32T: U): NetworkPeer` | function |
| `enet_peer_send` | `NetworkPeer.send(...)` | `NetworkPeer.send(int8T: U, value: NetworkPacket): number` | function |
| `enet_peer_ping` (overload 1) | `NetworkPeer.ping(...)` | `NetworkPeer.ping(): void` | function |
| `enet_peer_ping_interval` | `NetworkPeer.pingInterval(...)` | `NetworkPeer.pingInterval(int32T: U): void` | function |
| `enet_peer_timeout` (overload 1) | `NetworkPeer.timeout(...)` | `NetworkPeer.timeout(int32T: U, int32T: U, int32T: U): void` | function |
| `enet_peer_reset` (overload 1) | `NetworkPeer.reset(...)` | `NetworkPeer.reset(): void` | function |
| `enet_peer_disconnect` (overload 1) | `NetworkPeer.disconnect(...)` | `NetworkPeer.disconnect(int32T: U): void` | function |
| `enet_peer_disconnect_now` (overload 1) | `NetworkPeer.disconnectNow(...)` | `NetworkPeer.disconnectNow(int32T: U): void` | function |
| `enet_peer_disconnect_later` | `NetworkPeer.disconnectLater(...)` | `NetworkPeer.disconnectLater(int32T: U): void` | function |
| `enet_peer_throttle_configure` | `NetworkPeer.throttleConfigure(...)` | `NetworkPeer.throttleConfigure(int32T: U, int32T: U, int32T: U): void` | function |
| `enet_peer_throttle` | `NetworkPeer.throttle(...)` | `NetworkPeer.throttle(int32T: U): number` | function |
| `enet_peer_reset_queues` | `NetworkPeer.resetQueues(...)` | `NetworkPeer.resetQueues(): void` | function |
| `enet_peer_has_outgoing_commands` | `NetworkPeer.hasOutgoingCommands(...)` | `NetworkPeer.hasOutgoingCommands(): number` | function |
| `enet_peer_setup_outgoing_command` | `NetworkOutgoingCommand.peerSetup(...)` | `NetworkOutgoingCommand.peerSetup(value: NetworkPeer): void` | function |
| `enet_peer_queue_outgoing_command` | `NetworkPeer.queueOutgoingCommand(...)` | `NetworkPeer.queueOutgoingCommand(value: Readonly<NetworkProtocol>, value: NetworkPacket, int32T: U, int16T: U): NetworkOutgoingCommand` | function |
| `enet_peer_queue_incoming_command` | `NetworkPeer.queueIncomingCommand(...)` | `NetworkPeer.queueIncomingCommand(value: Readonly<NetworkProtocol>, value: Uint8Array, izeT: S, int32T: U, int32T: U): NetworkIncomingCommand` | function |
| `enet_peer_queue_acknowledgement` | `NetworkPeer.queueAcknowledgement(...)` | `NetworkPeer.queueAcknowledgement(value: Readonly<NetworkProtocol>, int16T: U): NetworkAcknowledgement` | function |
| `enet_peer_dispatch_incoming_unreliable_commands` | `NetworkIncomingCommand.peerDispatchIncomingUnreliableCommands(...)` | `NetworkIncomingCommand.peerDispatchIncomingUnreliableCommands(value: NetworkPeer, value: NetworkChannel): void` | function |
| `enet_peer_dispatch_incoming_reliable_commands` | `NetworkIncomingCommand.peerDispatchIncomingReliableCommands(...)` | `NetworkIncomingCommand.peerDispatchIncomingReliableCommands(value: NetworkPeer, value: NetworkChannel): void` | function |
| `enet_peer_on_connect` | `NetworkPeer.onConnect(...)` | `NetworkPeer.onConnect(): void` | function |
| `enet_peer_on_disconnect` | `NetworkPeer.onDisconnect(...)` | `NetworkPeer.onDisconnect(): void` | function |
| `enet_packet_create` | `network.createPacket(...)` | `network.createPacket(value: Uint8Array, izeT: S, int32T: U): NetworkPacket` | function |
| `enet_packet_destroy` | `NetworkPacket.dispose(...)` | `NetworkPacket.dispose(): void` | function |
| `enet_packet_resize` | `NetworkPacket.resize(...)` | `NetworkPacket.resize(izeT: S): number` | function |
| `enet_peer_receive` | `NetworkPeer.receive(...)` | `NetworkPeer.receive(channelId: Uint8Array): NetworkPacket` | function |
| `enet_range_coder_create` | `network.createRangeCoder(...)` | `network.createRangeCoder(): Uint8Array` | function |
| `enet_range_coder_destroy` | `network.rangeCoderDestroy(...)` | `network.rangeCoderDestroy(value: Uint8Array): void` | function |
| `enet_range_coder_compress` | `NetworkBuffer.rangeCoderCompress(...)` | `NetworkBuffer.rangeCoderCompress(value: Uint8Array, izeT: S, izeT: S, value: Uint8Array, izeT: S): number` | function |
| `enet_range_coder_decompress` | `network.rangeCoderDecompress(...)` | `network.rangeCoderDecompress(value: Uint8Array, value: Uint8Array, izeT: S, value: Uint8Array, izeT: S): number` | function |
| `enet_protocol_command_size` | `network.protocolCommandSize(...)` | `network.protocolCommandSize(int8T: U): number` | function |
| `enet_packet_get_data` | `NetworkPacket.getData(...)` | `NetworkPacket.getData(): Uint8Array` | function |
| `enet_packet_get_user_data` | `NetworkPacket.getUserData(...)` | `NetworkPacket.getUserData(): Uint8Array` | function |
| `enet_packet_set_user_data` | `NetworkPacket.setUserData(...)` | `NetworkPacket.setUserData(): void` | function |
| `enet_packet_get_length` | `NetworkPacket.getLength(...)` | `NetworkPacket.getLength(): number` | function |
| `enet_packet_set_acknowledge_callback` | `NetworkPacket.setAcknowledgeCallback(...)` | `NetworkPacket.setAcknowledgeCallback(netPacketAcknowledgedCallback: E): void` | function |
| `enet_packet_set_free_callback` | `NetworkPacket.setFreeCallback(...)` | `NetworkPacket.setFreeCallback(netPacketFreeCallback: E): void` | function |
| `enet_packet_check_references` | `NetworkPacket.checkReferences(...)` | `NetworkPacket.checkReferences(): number` | function |
| `enet_packet_dispose` | `NetworkPacket.dispose(...)` | `NetworkPacket.dispose(): void` | function |
| `enet_host_get_peers_count` | `NetworkHost.getPeersCount(...)` | `NetworkHost.getPeersCount(): number` | function |
| `enet_host_get_packets_sent` | `NetworkHost.getPacketsSent(...)` | `NetworkHost.getPacketsSent(): number` | function |
| `enet_host_get_packets_received` | `NetworkHost.getPacketsReceived(...)` | `NetworkHost.getPacketsReceived(): number` | function |
| `enet_host_get_bytes_sent` | `NetworkHost.getBytesSent(...)` | `NetworkHost.getBytesSent(): number` | function |
| `enet_host_get_bytes_received` | `NetworkHost.getBytesReceived(...)` | `NetworkHost.getBytesReceived(): number` | function |
| `enet_host_set_max_duplicate_peers` | `NetworkHost.setMaxDuplicatePeers(...)` | `NetworkHost.setMaxDuplicatePeers(int16T: U): void` | function |
| `enet_host_set_intercept_callback` | `NetworkHost.setInterceptCallback(...)` | `NetworkHost.setInterceptCallback(netInterceptCallback: E): void` | function |
| `enet_host_set_checksum_callback` | `NetworkHost.setChecksumCallback(...)` | `NetworkHost.setChecksumCallback(netChecksumCallback: E): void` | function |
| `enet_peer_get_id` | `NetworkPeer.getId(...)` | `NetworkPeer.getId(): number` | function |
| `enet_peer_get_ip` | `NetworkPeer.getIp(...)` | `NetworkPeer.getIp(value: Uint8Array, izeT: S): number` | function |
| `enet_peer_get_port` | `NetworkPeer.getPort(...)` | `NetworkPeer.getPort(): number` | function |
| `enet_peer_get_mtu` | `NetworkPeer.getMtu(...)` | `NetworkPeer.getMtu(): number` | function |
| `enet_peer_get_state` | `NetworkPeer.getState(...)` | `NetworkPeer.getState(): NetworkPeerState` | function |
| `enet_peer_get_rtt` | `NetworkPeer.getRtt(...)` | `NetworkPeer.getRtt(): number` | function |
| `enet_peer_get_last_rtt` | `NetworkPeer.getLastRtt(...)` | `NetworkPeer.getLastRtt(): number` | function |
| `enet_peer_get_lastsendtime` | `NetworkPeer.getLastsendtime(...)` | `NetworkPeer.getLastsendtime(): number` | function |
| `enet_peer_get_lastreceivetime` | `NetworkPeer.getLastreceivetime(...)` | `NetworkPeer.getLastreceivetime(): number` | function |
| `enet_peer_get_packets_throttle` | `NetworkPeer.getPacketsThrottle(...)` | `NetworkPeer.getPacketsThrottle(): number` | function |
| `enet_peer_get_data` | `NetworkPeer.getData(...)` | `NetworkPeer.getData(): Uint8Array` | function |
| `enet_peer_set_data` | `NetworkPeer.setData(...)` | `NetworkPeer.setData(value: Uint8Array): void` | function |
| `enet_host_create_with_transport` (overload 1) | `NetworkAddress.hostCreateWithTransport(...)` | `NetworkAddress.hostCreateWithTransport(peerCount: number, channelLimit: number, incomingBandwidth: number, outgoingBandwidth: number, transport: NetworkTransportType): NetworkHost` | function |
| `enet_host_connect` (overload 2) | `NetworkHost.connect(...)` | `NetworkHost.connect(address: Readonly<NetworkAddress>, channelCount: number, data: number): NetworkPeer` | function |
| `enet_host_connect_ex` (overload 1) | `NetworkHost.connectEx(...)` | `NetworkHost.connectEx(endpoint: string, channelCount: number, data: number): NetworkPeer` | function |
| `enet_host_transport_caps` (overload 1) | `NetworkHost.transportCaps(...)` | `NetworkHost.transportCaps(): number` | function |
| `enet_transport_handle_event` | `NetworkTransportEvent.transportHandle(...)` | `NetworkTransportEvent.transportHandle(host: NetworkHost): void` | function |
| `enet_peer_reset` (overload 2) | `NetworkPeer.reset(...)` | `NetworkPeer.reset(): void` | function |
| `enet_peer_timeout` (overload 2) | `NetworkPeer.timeout(...)` | `NetworkPeer.timeout(): void` | function |
| `enet_peer_ping` (overload 2) | `NetworkPeer.ping(...)` | `NetworkPeer.ping(): void` | function |
| `enet_packet_encode` | `NetPacket.encode(...)` | `NetPacket.encode(outBuffer: Uint8, bufferSize: number): number` | function |
| `enet_packet_decode` | `Uint8.packetDecode(...)` | `Uint8.packetDecode(outPacket: NetPacket, bufferSize: number): boolean` | function |
| `enet_fragment_send` | `NetworkPeer.fragmentSend(...)` | `NetworkPeer.fragmentSend(channelId: Uint8, data: ReadonlyArray<Uint8>): void` | function |
| `enet_fragment_receive` | `NetworkPeer.fragmentReceive(...)` | `NetworkPeer.fragmentReceive(fragment: Readonly<NetPacket>, outPacket: NetPacket): boolean` | function |
| `enet_bandwidth_update` | `NetworkHost.bandwidthUpdate(...)` | `NetworkHost.bandwidthUpdate(deltaTime: number): void` | function |
| `enet_peer_send_window` | `NetworkPeer.sendWindow(...)` | `NetworkPeer.sendWindow(): number` | function |
| `enet_peer_create_channel` | `NetworkPeer.createChannel(...)` | `NetworkPeer.createChannel(reliable: boolean, ordered: boolean): number` | function |
| `enet_peer_reset_channels` | `NetworkPeer.resetChannels(...)` | `NetworkPeer.resetChannels(): void` | function |
| `enet_peer_disconnect` (overload 2) | `NetworkPeer.disconnect(...)` | `NetworkPeer.disconnect(reason: Uint32TT): void` | function |
| `enet_peer_disconnect_now` (overload 2) | `NetworkPeer.disconnectNow(...)` | `NetworkPeer.disconnectNow(): void` | function |
| `enet_peer_get_stats` | `NetworkPeer.getStats(...)` | `NetworkPeer.getStats(rtt: number[], packetLoss: number[], sent: Uint32TT, received: Uint32TT): void` | function |
| `enet_host_flush` (overload 2) | `NetworkHost.flush(...)` | `NetworkHost.flush(): void` | function |
| `enet_host_broadcast` (overload 2) | `NetworkHost.broadcast(...)` | `NetworkHost.broadcast(channelId: Uint8, data: ReadonlyArray<Uint8>): void` | function |
| `enet_host_set_protocol_id` | `NetworkHost.setProtocolId(...)` | `NetworkHost.setProtocolId(protocolId: Uint16TT): void` | function |
| `enet_host_get_protocol_id` | `NetworkHost.getProtocolId(...)` | `NetworkHost.getProtocolId(): Uint16TT` | function |
| `enet_host_create_with_transport` (overload 2) | `NetworkAddress.hostCreateWithTransport(...)` | `NetworkAddress.hostCreateWithTransport(peerCount: number, channelLimit: number, incomingBandwidth: number, outgoingBandwidth: number, transport: NetworkTransportType): NetworkHost` | function |
| `enet_host_connect` (overload 3) | `NetworkHost.connect(...)` | `NetworkHost.connect(value: Readonly<NetworkAddress>, izeT: S, int32T: U): NetworkPeer` | function |
| `enet_host_connect_ex` (overload 2) | `NetworkHost.connectEx(...)` | `NetworkHost.connectEx(endpoint: string, izeT: S, int32T: U): NetworkPeer` | function |
| `enet_host_transport_caps` (overload 2) | `NetworkHost.transportCaps(...)` | `NetworkHost.transportCaps(): number` | function |
| `enet_http_server_start` | `network.httpServerStart(...)` | `network.httpServerStart(bindAddress: string, port: NetworkUint16T): NetworkHttpServer` | function |
| `enet_http_server_stop` | `NetworkHttpServer.stop(...)` | `NetworkHttpServer.stop(): void` | function |
| `enet_http_register_endpoint` | `NetworkHttpServer.httpRegisterEndpoint(...)` | `NetworkHttpServer.httpRegisterEndpoint(path: string, handler: NetworkHttpEndpointHandler): void` | function |
| `enet_host_create_secure` | `NetworkAddress.hostCreateSecure(...)` | `NetworkAddress.hostCreateSecure(peerCount: number, channelLimit: number, incomingBandwidth: number, outgoingBandwidth: number, transport: NetworkTransportType, security: Readonly<NetworkTransportSecurity>): NetworkHost` | function |
| `enet_http_request_async` | `network.httpRequestAsync(...)` | `network.httpRequestAsync(desc: Readonly<NetworkHttpRequestDesc>, cb: NetworkHttpResponseCallback): NetworkHttpRequest` | function |
| `enet_http_request_cancel` | `NetworkHttpRequest.cancel(...)` | `NetworkHttpRequest.cancel(): void` | function |
| `enet_http_request_release` | `NetworkHttpRequest.release(...)` | `NetworkHttpRequest.release(): void` | function |
| `enet_host_set_connect_validator` | `NetworkHost.setConnectValidator(...)` | `NetworkHost.setConnectValidator(validator: NetworkConnectValidator): void` | function |
| `enet_transport_send_stream` | `network.transportSendStream(...)` | `network.transportSendStream(args: ...): number` | function |
| `enet_transport_receive_stream` | `network.transportReceiveStream(...)` | `network.transportReceiveStream(args: ...): number` | function |
| `enet_transport_ws_close` | `NetworkTransportPeer.transportWsClose(...)` | `NetworkTransportPeer.transportWsClose(code: number): void` | function |
| `enet_transport_webrtc_signal` | `NetworkWebRTCSignal.transportWebRtc(...)` | `NetworkWebRTCSignal.transportWebRtc(value: NetworkTransport, value: NetworkTransportPeer): void` | function |
| `enet_host_set_webrtc_signal_callback` | `NetworkHost.setWebRtcSignalCallback(...)` | `NetworkHost.setWebRtcSignalCallback(netWebRTCSignalCallback: E, value: Uint8Array): void` | function |
| `enet_webrtc_handle_signal` | `NetworkWebRTCSignal.webRtcHandle(...)` | `NetworkWebRTCSignal.webRtcHandle(value: NetworkHost, value: NetworkPeer): number` | function |

### `Moss_Physics.h` → `physics`

| Native symbol (traceability) | Public API projection | Target-language signature | Kind |
|---|---|---|---|
| `CollideShapeResult_FreeMembers` | `CollideShapeResult.freeMembers(...)` | `CollideShapeResult.freeMembers(): void` | function |
| `CollisionEstimationResult_FreeMembers` | `CollisionEstimationResult.freeMembers(...)` | `CollisionEstimationResult.freeMembers(): void` | function |
| `BroadPhaseLayerInterfaceMask_Create` | `physics.createBroadPhaseLayerInterfaceMask(...)` | `physics.createBroadPhaseLayerInterfaceMask(numBroadPhaseLayers: number): BroadPhaseLayerInterface` | function |
| `BroadPhaseLayerInterfaceMask_ConfigureLayer` | `BroadPhaseLayerInterface.maskConfigureLayer(...)` | `BroadPhaseLayerInterface.maskConfigureLayer(broadPhaseLayer: BroadPhaseLayer, groupsToInclude: number, groupsToExclude: number): void` | function |
| `BroadPhaseLayerInterfaceTable_Create` | `physics.createBroadPhaseLayerInterfaceTable(...)` | `physics.createBroadPhaseLayerInterfaceTable(numObjectLayers: number, numBroadPhaseLayers: number): BroadPhaseLayerInterface` | function |
| `BroadPhaseLayerInterfaceTable_MapObjectToBroadPhaseLayer` | `BroadPhaseLayerInterface.tableMapObjectToBroadPhaseLayer(...)` | `BroadPhaseLayerInterface.tableMapObjectToBroadPhaseLayer(objectLayer: ObjectLayer, broadPhaseLayer: BroadPhaseLayer): void` | function |
| `ObjectLayerPairFilterMask_Create` | `physics.createObjectLayerPairFilterMask(...)` | `physics.createObjectLayerPairFilterMask(): ObjectLayerPairFilter` | function |
| `ObjectLayerPairFilterMask_GetObjectLayer` | `physics.objectLayerPairFilterMaskGetObjectLayer(...)` | `physics.objectLayerPairFilterMaskGetObjectLayer(group: number, mask: number): ObjectLayer` | function |
| `ObjectLayerPairFilterMask_GetGroup` | `physics.objectLayerPairFilterMaskGetGroup(...)` | `physics.objectLayerPairFilterMaskGetGroup(layer: ObjectLayer): number` | function |
| `ObjectLayerPairFilterMask_GetMask` | `physics.objectLayerPairFilterMaskGetMask(...)` | `physics.objectLayerPairFilterMaskGetMask(layer: ObjectLayer): number` | function |
| `ObjectLayerPairFilterTable_Create` | `physics.createObjectLayerPairFilterTable(...)` | `physics.createObjectLayerPairFilterTable(numObjectLayers: number): ObjectLayerPairFilter` | function |
| `ObjectLayerPairFilterTable_DisableCollision` | `ObjectLayerPairFilter.tableDisableCollision(...)` | `ObjectLayerPairFilter.tableDisableCollision(layer1: ObjectLayer, layer2: ObjectLayer): void` | function |
| `ObjectLayerPairFilterTable_EnableCollision` | `ObjectLayerPairFilter.tableEnableCollision(...)` | `ObjectLayerPairFilter.tableEnableCollision(layer1: ObjectLayer, layer2: ObjectLayer): void` | function |
| `ObjectLayerPairFilterTable_ShouldCollide` | `ObjectLayerPairFilter.tableShouldCollide(...)` | `ObjectLayerPairFilter.tableShouldCollide(layer1: ObjectLayer, layer2: ObjectLayer): boolean` | function |
| `ObjectVsBroadPhaseLayerFilterMask_Create` | `BroadPhaseLayerInterface.createObjectVsBroadPhaseLayerFilterMask(...)` | `BroadPhaseLayerInterface.createObjectVsBroadPhaseLayerFilterMask(): ObjectVsBroadPhaseLayerFilter` | function |
| `ObjectVsBroadPhaseLayerFilterTable_Create` | `BroadPhaseLayerInterface.createObjectVsBroadPhaseLayerFilterTable(...)` | `BroadPhaseLayerInterface.createObjectVsBroadPhaseLayerFilterTable(numBroadPhaseLayers: number, objectLayerPairFilter: ObjectLayerPairFilter, numObjectLayers: number): ObjectVsBroadPhaseLayerFilter` | function |
| `DrawSettings_InitDefault` | `DrawSettings.initDefault(...)` | `DrawSettings.initDefault(): void` | function |
| `ContactManifold_GetWorldSpaceNormal` | `ContactManifold.getWorldSpaceNormal(...)` | `ContactManifold.getWorldSpaceNormal(result: Vec3): void` | function |
| `ContactManifold_GetPenetrationDepth` | `ContactManifold.getPenetrationDepth(...)` | `ContactManifold.getPenetrationDepth(): number` | function |
| `ContactManifold_GetSubShapeID1` | `ContactManifold.getSubShapeId1(...)` | `ContactManifold.getSubShapeId1(): SubShapeId` | function |
| `ContactManifold_GetSubShapeID2` | `ContactManifold.getSubShapeId2(...)` | `ContactManifold.getSubShapeId2(): SubShapeId` | function |
| `ContactManifold_GetPointCount` | `ContactManifold.getPointCount(...)` | `ContactManifold.getPointCount(): number` | function |
| `ContactManifold_GetWorldSpaceContactPointOn1` | `ContactManifold.getWorldSpaceContactPointOn1(...)` | `ContactManifold.getWorldSpaceContactPointOn1(index: number, result: Vec3): void` | function |
| `ContactManifold_GetWorldSpaceContactPointOn2` | `ContactManifold.getWorldSpaceContactPointOn2(...)` | `ContactManifold.getWorldSpaceContactPointOn2(index: number, result: Vec3): void` | function |
| `CollisionDispatch_CollideShapeVsShape` | `CollideShapeCollectorCallback.collisionDispatchCollideShapeVsShape(...)` | `CollideShapeCollectorCallback.collisionDispatchCollideShapeVsShape(shape1: Readonly<Shape>, shape2: Readonly<Shape>, scale1: Readonly<Vec3>, scale2: Readonly<Vec3>, centerOfMassTransform1: Readonly<Mat44>, centerOfMassTransform2: Readonly<Mat44>, collideShapeSettings: Readonly<CollideShapeSettings>, shapeFilter: Readonly<ShapeFilter>): boolean` | function |
| `CollisionDispatch_CastShapeVsShapeLocalSpace` | `CastShapeCollectorCallback.collisionDispatchCastShapeVsShapeLocalSpace(...)` | `CastShapeCollectorCallback.collisionDispatchCastShapeVsShapeLocalSpace(direction: Readonly<Vec3>, shape1: Readonly<Shape>, shape2: Readonly<Shape>, scale1InShape2LocalSpace: Readonly<Vec3>, scale2: Readonly<Vec3>, centerOfMassTransform1InShape2LocalSpace: Mat44, centerOfMassWorldTransform2: Mat44, shapeCastSettings: Readonly<ShapeCastSettings>, shapeFilter: Readonly<ShapeFilter>): boolean` | function |
| `CollisionDispatch_CastShapeVsShapeWorldSpace` | `CastShapeCollectorCallback.collisionDispatchCastShapeVsShapeWorldSpace(...)` | `CastShapeCollectorCallback.collisionDispatchCastShapeVsShapeWorldSpace(direction: Readonly<Vec3>, shape1: Readonly<Shape>, shape2: Readonly<Shape>, scale1: Readonly<Vec3>, inScale2: Readonly<Vec3>, centerOfMassWorldTransform1: Readonly<Mat44>, centerOfMassWorldTransform2: Readonly<Mat44>, shapeCastSettings: Readonly<ShapeCastSettings>, shapeFilter: Readonly<ShapeFilter>): boolean` | function |
| `Skeleton_Create` | `physics.createSkeleton(...)` | `physics.createSkeleton(): Skeleton` | function |
| `Skeleton_Destroy` | `Skeleton.dispose(...)` | `Skeleton.dispose(): void` | function |
| `Skeleton_AddJoint` | `Skeleton.addJoint(...)` | `Skeleton.addJoint(name: string): number` | function |
| `Skeleton_AddJoint2` | `Skeleton.addJoint2(...)` | `Skeleton.addJoint2(name: string, parentIndex: number): number` | function |
| `Skeleton_AddJoint3` | `Skeleton.addJoint3(...)` | `Skeleton.addJoint3(name: string, parentName: string): number` | function |
| `Skeleton_GetJointCount` | `Skeleton.getJointCount(...)` | `Skeleton.getJointCount(): number` | function |
| `Skeleton_GetJoint` | `SkeletonJoint.skeletonGet(...)` | `SkeletonJoint.skeletonGet(skeleton: Readonly<Skeleton>, index: number): void` | function |
| `Skeleton_GetJointIndex` | `Skeleton.getJointIndex(...)` | `Skeleton.getJointIndex(name: string): number` | function |
| `Skeleton_CalculateParentJointIndices` | `Skeleton.calculateParentJointIndices(...)` | `Skeleton.calculateParentJointIndices(): void` | function |
| `Skeleton_AreJointsCorrectlyOrdered` | `Skeleton.areJointsCorrectlyOrdered(...)` | `Skeleton.areJointsCorrectlyOrdered(): boolean` | function |
| `Indexify` | `physics.indexify(...)` | `physics.indexify(inTriangles: TriangleList, outVertices: VertexList, outTriangles: IndexedTriangleList, inVertexWeldDistance?: number): void` | function |
| `Deindexify` | `physics.deindexify(...)` | `physics.deindexify(inVertices: VertexList, inTriangles: IndexedTriangleList, outTriangles: TriangleList): void` | function |

### `Moss_Platform.h` → `platform`

| Native symbol (traceability) | Public API projection | Target-language signature | Kind |
|---|---|---|---|
| `Moss_CreateWindow` | `platform.createWindow(...)` | `platform.createWindow(options: WindowOptions): Window` | function |
| `Moss_TerminateWindow` | `Window.dispose(...)` | `Window.dispose(): void` | function |
| `Moss_CreateMessageBox` | `Window.showMessageBox(...)` | `Window.showMessageBox(title: string, message: string, flags: MessageBoxFlags): boolean` | function |
| `Moss_ShouldWindowClose` | `Window.shouldClose` | `Window.shouldClose: boolean` | function |
| `Moss_PollEvents` | `platform.pollEvents(...)` | `platform.pollEvents(): void` | function |
| `Moss_GetWindowWidth` | `platform.getWindowWidth(...)` | `platform.getWindowWidth(): number` | function |
| `Moss_GetWindowHeight` | `platform.getWindowHeight(...)` | `platform.getWindowHeight(): number` | function |
| `Moss_SetWindowTitle` | `Window.title` | `Window.title: string` | function |
| `Moss_SetWindowIcon` | `Window.setIcon(...)` | `Window.setIcon(image: Image): void` | function |
| `Moss_CloseWindow` | `Window.dispose(...)` | `Window.dispose(): void` | function |
| `Moss_MonitorGetPrimary` | `platform.monitors.monitorGetPrimary(...)` | `platform.monitors.monitorGetPrimary(): Monitor` | function |
| `Moss_MonitorGetSecondary` | `platform.monitors.monitorGetSecondary(...)` | `platform.monitors.monitorGetSecondary(): Monitor` | function |
| `Moss_MonitorGetPhysicalSize` | `Monitor.getPhysicalSize(...)` | `Monitor.getPhysicalSize(widthMm: number[], heightMm: number[]): void` | function |
| `Moss_MonitorGetContentScale` | `Monitor.getContentScale(...)` | `Monitor.getContentScale(xscale: number[], yscale: number[]): void` | function |
| `Moss_MonitorGetPosition` | `Monitor.getPosition(...)` | `Monitor.getPosition(x: number[], y: number[]): void` | function |
| `Moss_MonitorGetName` | `Monitor.getName(...)` | `Monitor.getName(): string \| null` | function |
| `Moss_MonitorSetGammaRamp` | `GammaRamp.monitorSet(...)` | `GammaRamp.monitorSet(monitor: Monitor): void` | function |
| `Moss_MonitorGetGammaRamp` | `Monitor.getGammaRamp(...)` | `Monitor.getGammaRamp(): GammaRamp` | function |
| `Moss_MonitorSetGamma` | `Monitor.setGamma(...)` | `Monitor.setGamma(gamma: number): void` | function |
| `Moss_IsKeyPressed` | `platform.input.isKeyPressed(...)` | `platform.input.isKeyPressed(key: Keyboard): boolean` | function |
| `Moss_IsReleased` | `platform.isReleased(...)` | `platform.isReleased(key: Keyboard): boolean` | function |
| `Moss_IsKeyJustPressed` | `platform.input.isKeyJustPressed(...)` | `platform.input.isKeyJustPressed(key: Keyboard): boolean` | function |
| `Moss_IsKeyJustReleased` | `platform.input.isKeyJustReleased(...)` | `platform.input.isKeyJustReleased(key: Keyboard): boolean` | function |
| `Moss_InputGetKey` | `platform.input.inputGetKey(...)` | `platform.input.inputGetKey(): Keyboard` | function |
| `Moss_IsMousePressed` | `platform.input.isMousePressed(...)` | `platform.input.isMousePressed(button: Mouse): boolean` | function |
| `Moss_IsMouseReleased` | `platform.input.isMouseReleased(...)` | `platform.input.isMouseReleased(button: Mouse): boolean` | function |
| `Moss_IsMouseJustPressed` | `platform.input.isMouseJustPressed(...)` | `platform.input.isMouseJustPressed(button: Mouse): boolean` | function |
| `Moss_IsMouseJustReleased` | `platform.input.isMouseJustReleased(...)` | `platform.input.isMouseJustReleased(button: Mouse): boolean` | function |
| `Moss_InputGetMouseButton` | `platform.input.inputGetMouseButton(...)` | `platform.input.inputGetMouseButton(): Mouse` | function |
| `Moss_GetMousePosition` | `platform.input.mousePosition(...)` | `platform.input.mousePosition(): Point` | function |
| `Moss_SetMousePosition` | `platform.input.setMousePosition(...)` | `platform.input.setMousePosition(x: number, y: number): void` | function |
| `Moss_SetMouseVisible` | `platform.input.setMouseVisible(...)` | `platform.input.setMouseVisible(visible: boolean): void` | function |
| `Moss_GetNumGamepads` | `platform.getNumGamepads(...)` | `platform.getNumGamepads(): number` | function |
| `Moss_OpenGamepad` | `platform.gamepads.openGamepad(...)` | `platform.gamepads.openGamepad(id: GamepadId): Gamepad` | function |
| `Moss_CloseGamepad` | `Gamepad.dispose(...)` | `Gamepad.dispose(): void` | function |
| `Moss_GamepadConnected` | `Gamepad.connected(...)` | `Gamepad.connected(): boolean` | function |
| `Moss_UpdateGamepads` | `platform.updateGamepads(...)` | `platform.updateGamepads(): void` | function |
| `Moss_IsGamepadButtonPressed` | `Gamepad.isButtonPressed(...)` | `Gamepad.isButtonPressed(button: GamepadButton): boolean` | function |
| `Moss_IsGamepadButtonJustPressed` | `Gamepad.isButtonJustPressed(...)` | `Gamepad.isButtonJustPressed(button: GamepadButton): boolean` | function |
| `Moss_IsGamepadButtonJustReleased` | `Gamepad.isButtonJustReleased(...)` | `Gamepad.isButtonJustReleased(button: GamepadButton): boolean` | function |
| `Moss_GetGamepadAxis` | `Gamepad.getAxis(...)` | `Gamepad.getAxis(axis: GamepadAxis): number` | function |
| `Moss_GetGamepadOrientation` | `Gamepad.getOrientation(...)` | `Gamepad.getOrientation(w: number[], x: number[], y: number[], z: number[]): boolean` | function |
| `Moss_GetGamepadPlayerSpaceGyro` | `Gamepad.getPlayerSpaceGyro(...)` | `Gamepad.getPlayerSpaceGyro(outX: number[], outY: number[], yawRelaxFactor?: number): void` | function |
| `Moss_GetGamepadWorldSpaceGyro` | `Gamepad.getWorldSpaceGyro(...)` | `Gamepad.getWorldSpaceGyro(outX: number[], outY: number[], sideReductionThreshold?: number): void` | function |
| `Moss_ResetGamepadCalibration` | `Gamepad.resetCalibration(...)` | `Gamepad.resetCalibration(): void` | function |
| `Moss_RumbleGamepad` | `Gamepad.rumble(...)` | `Gamepad.rumble(low: number, high: number, durationMs: number): boolean` | function |
| `Moss_RumbleGamepadTriggers` | `Gamepad.rumbleTriggers(...)` | `Gamepad.rumbleTriggers(left: number, right: number, durationMs: number): boolean` | function |
| `Moss_SetGamepadLED` | `Gamepad.setLed(...)` | `Gamepad.setLed(r: number, g: number, b: number): boolean` | function |
| `Moss_GetGamepadName` | `Gamepad.getName(...)` | `Gamepad.getName(): string \| null` | function |
| `Moss_GetGamepadID` | `Gamepad.getId(...)` | `Gamepad.getId(): GamepadId` | function |
| `Moss_GetGamepadPlayerIndex` | `Gamepad.getPlayerIndex(...)` | `Gamepad.getPlayerIndex(): number` | function |
| `Moss_GetGamepadPowerInfo` | `Gamepad.getPowerInfo(...)` | `Gamepad.getPowerInfo(percent: number[]): PowerState` | function |
| `Moss_GetNumGamepadTouchpads` | `Gamepad.getNumTouchpads(...)` | `Gamepad.getNumTouchpads(): number` | function |
| `Moss_GetNumGamepadTouchpadFingers` | `Gamepad.getNumTouchpadFingers(...)` | `Gamepad.getNumTouchpadFingers(): number` | function |
| `Moss_GetGamepadTouchpadFinger` | `Gamepad.getTouchpadFinger(...)` | `Gamepad.getTouchpadFinger(pad: number, finger: number, down: boolean[], x: number[], y: number[], pressure: number[]): boolean` | function |
| `Moss_GetGamepadMapping` | `Gamepad.getMapping(...)` | `Gamepad.getMapping(): string \| null` | function |
| `Moss_SetGamepadMapping` | `Gamepad.setMapping(...)` | `Gamepad.setMapping(mapping: string): boolean` | function |
| `Moss_ReloadGamepadMappings` | `platform.gamepads.reloadGamepadMappings(...)` | `platform.gamepads.reloadGamepadMappings(): void` | function |
| `Moss_InputGetGamepadButton` | `platform.gamepads.inputGetGamepadButton(...)` | `platform.gamepads.inputGetGamepadButton(): GamepadButton` | function |
| `Moss_InputGetGamepadAxis` | `platform.gamepads.inputGetGamepadAxis(...)` | `platform.gamepads.inputGetGamepadAxis(): GamepadAxis` | function |
| `Moss_GetPenDeviceType` | `platform.input.getPenDeviceType(...)` | `platform.input.getPenDeviceType(instanceId: PenId): PenDeviceType` | function |
| `Moss_GetTouchDeviceName` | `platform.input.getTouchDeviceName(...)` | `platform.input.getTouchDeviceName(touchId: TouchId): string \| null` | function |
| `Moss_GetTouchDevices` | `platform.input.getTouchDevices(...)` | `platform.input.getTouchDevices(count: number[]): TouchId` | function |
| `Moss_GetTouchDeviceType` | `platform.input.getTouchDeviceType(...)` | `platform.input.getTouchDeviceType(touchId: TouchId): TouchDeviceType` | function |
| `Moss_GetTouchFingers` | `platform.input.getTouchFingers(...)` | `platform.input.getTouchFingers(touchId: TouchId, count: number[]): Finger` | function |
| `Moss_GamepadHasSensor` | `Gamepad.hasSensor(...)` | `Gamepad.hasSensor(sensor: GamepadSensorType): boolean` | function |
| `Moss_SetGamepadSensorEnabled` | `Gamepad.setSensorEnabled(...)` | `Gamepad.setSensorEnabled(sensor: GamepadSensorType, enabled: boolean): boolean` | function |
| `Moss_GetGamepadSensorData` | `GamepadSensorData.get(...)` | `GamepadSensorData.get(gp: Gamepad, sensor: GamepadSensorType): boolean` | function |
| `Moss_OpenHaptic` | `platform.haptics.openHaptic(...)` | `platform.haptics.openHaptic(id: HapticId): Haptic` | function |
| `Moss_CloseHaptic` | `Haptic.dispose(...)` | `Haptic.dispose(): void` | function |
| `Moss_CreateHapticEffect` | `Haptic.createEffect(...)` | `Haptic.createEffect(): HapticId` | function |
| `Moss_DestroyHapticEffect` | `Haptic.destroyEffect(...)` | `Haptic.destroyEffect(): void` | function |
| `Moss_GetHapticEffectStatus` | `Haptic.getEffectStatus(...)` | `Haptic.getEffectStatus(): boolean` | function |
| `Moss_GetHapticFeatures` | `Haptic.getFeatures(...)` | `Haptic.getFeatures(): number` | function |
| `Moss_GetHapticFromID` | `Haptic.getFromId(...)` | `Haptic.getFromId(): Haptic` | function |
| `Moss_GetHapticID` | `Haptic.getId(...)` | `Haptic.getId(): HapticId` | function |
| `Moss_GetHapticName` | `Haptic.getName(...)` | `Haptic.getName(): string \| null` | function |
| `Moss_GetHapticNameForID` | `Haptic.getNameForId(...)` | `Haptic.getNameForId(): string \| null` | function |
| `Moss_GetHaptics` | `Haptic.getHaptics(...)` | `Haptic.getHaptics(): HapticId` | function |
| `Moss_GetMaxHapticEffects` | `Haptic.getMaxEffects(...)` | `Haptic.getMaxEffects(): number` | function |
| `Moss_GetMaxHapticEffectsPlaying` | `Haptic.getMaxEffectsPlaying(...)` | `Haptic.getMaxEffectsPlaying(): number` | function |
| `Moss_GetNumHapticAxes` | `Haptic.getNumAxes(...)` | `Haptic.getNumAxes(): number` | function |
| `Moss_HapticEffectSupported` | `Haptic.effectSupported(...)` | `Haptic.effectSupported(): boolean` | function |
| `Moss_HapticRumbleSupported` | `Haptic.rumbleSupported(...)` | `Haptic.rumbleSupported(): boolean` | function |
| `Moss_InitHapticRumble` | `Haptic.initRumble(...)` | `Haptic.initRumble(): boolean` | function |
| `Moss_IsJoystickHaptic` | `Gamepad.isJoystickHaptic(...)` | `Gamepad.isJoystickHaptic(): boolean` | function |
| `Moss_IsMouseHaptic` | `platform.haptics.isMouseHaptic(...)` | `platform.haptics.isMouseHaptic(): boolean` | function |
| `Moss_OpenHapticFromJoystick` | `Gamepad.openHapticFromJoystick(...)` | `Gamepad.openHapticFromJoystick(): Haptic` | function |
| `Moss_OpenHapticFromMouse` | `platform.haptics.openHapticFromMouse(...)` | `platform.haptics.openHapticFromMouse(): Haptic` | function |
| `Moss_PauseHaptic` | `Haptic.pause(...)` | `Haptic.pause(): boolean` | function |
| `Moss_PlayHapticRumble` | `Haptic.playRumble(...)` | `Haptic.playRumble(strength: number, length: number): boolean` | function |
| `Moss_ResumeHaptic` | `Haptic.resume(...)` | `Haptic.resume(): boolean` | function |
| `Moss_RunHapticEffect` | `Haptic.runEffect(...)` | `Haptic.runEffect(iterations: number): boolean` | function |
| `Moss_SetHapticAutocenter` | `Haptic.setAutocenter(...)` | `Haptic.setAutocenter(center: number): boolean` | function |
| `Moss_SetHapticGain` | `Haptic.setGain(...)` | `Haptic.setGain(gain: number): boolean` | function |
| `Moss_StopHapticEffect` | `Haptic.stopEffect(...)` | `Haptic.stopEffect(effect: HapticEffectId): boolean` | function |
| `Moss_StopHapticEffects` | `Haptic.stopEffects(...)` | `Haptic.stopEffects(): boolean` | function |
| `Moss_StopHapticRumble` | `Haptic.stopRumble(...)` | `Haptic.stopRumble(): boolean` | function |
| `Moss_UpdateHapticEffect` | `HapticEffect.update(...)` | `HapticEffect.update(haptic: Haptic, effect: HapticEffectId): boolean` | function |
| `Moss_GetAvailableCPUCores` | `platform.getAvailableCpuCores(...)` | `platform.getAvailableCpuCores(): number` | function |
| `Moss_GetCPUCacheLineSize` | `platform.getCpuCacheLineSize(...)` | `platform.getCpuCacheLineSize(): number` | function |
| `Moss_GetSystemRAM` | `platform.getSystemRam(...)` | `platform.getSystemRam(): number` | function |
| `Moss_OpenURL` | `platform.openUrl(...)` | `platform.openUrl(url: string): boolean` | function |
| `Moss_GetLocale` | `platform.getLocale(...)` | `platform.getLocale(): Locale` | function |
| `Moss_GetPowerInfo` | `platform.getPowerInfo(...)` | `platform.getPowerInfo(seconds: number[], percent: number[]): PowerState` | function |
| `Moss_IsProcessRunningByName` | `platform.isProcessRunningByName(...)` | `platform.isProcessRunningByName(executablePath: string): boolean` | function |
| `Moss_LoadDynamicLibrary` | `platform.loadDynamicLibrary(...)` | `platform.loadDynamicLibrary(libPath: string): Uint8Array` | function |
| `Moss_GetLibrarySymbol` | `platform.getLibrarySymbol(...)` | `platform.getLibrarySymbol(handle: Uint8Array, symbolName: string): Uint8Array` | function |
| `Moss_UnloadDynamicLibrary` | `platform.unloadDynamicLibrary(...)` | `platform.unloadDynamicLibrary(handle: Uint8Array): void` | function |
| `Moss_SetFramebufferResizeCallback` | `platform.setFramebufferResizeCallback(...)` | `platform.setFramebufferResizeCallback(callback: FramebufferResizeCallback): void` | function |
| `Moss_SetWindowSizeCallback` | `platform.setWindowSizeCallback(...)` | `platform.setWindowSizeCallback(callback: WindowSizeCallback): void` | function |
| `Moss_SetWindowResizeCallback` | `platform.setWindowResizeCallback(...)` | `platform.setWindowResizeCallback(callback: WindowResizeCallback): void` | function |
| `Moss_SetWindowPositionCallback` | `platform.setWindowPositionCallback(...)` | `platform.setWindowPositionCallback(callback: WindowPositionCallback): void` | function |
| `Moss_SetWindowFocusCallback` | `platform.setWindowFocusCallback(...)` | `platform.setWindowFocusCallback(callback: WindowFocusCallback): void` | function |
| `Moss_SetWindowContentScaleCallback` | `platform.setWindowContentScaleCallback(...)` | `platform.setWindowContentScaleCallback(callback: WindowContentScaleCallback): void` | function |
| `Moss_SetMonitorCallback` | `platform.monitors.setMonitorCallback(...)` | `platform.monitors.setMonitorCallback(callback: MonitorCallback): void` | function |
| `Moss_GetCameras` | `platform.getCameras(...)` | `platform.getCameras(count: number[]): CameraId` | function |
| `Moss_GetCameraName` | `platform.cameras.getCameraName(...)` | `platform.cameras.getCameraName(cameraId: CameraId): string \| null` | function |
| `Moss_GetCameraPosition` | `platform.cameras.getCameraPosition(...)` | `platform.cameras.getCameraPosition(cameraId: CameraId): CameraPosition` | function |
| `Moss_GetCurrentCameraDriver` | `platform.cameras.getCurrentCameraDriver(...)` | `platform.cameras.getCurrentCameraDriver(): string \| null` | function |
| `Moss_GetNumCameraDrivers` | `platform.cameras.getNumCameraDrivers(...)` | `platform.cameras.getNumCameraDrivers(): number` | function |
| `Moss_GetCameraSupportedFormats` | `platform.cameras.getCameraSupportedFormats(...)` | `platform.cameras.getCameraSupportedFormats(cameraId: CameraId, count: number[]): CameraSpec` | function |
| `Moss_AcquireCameraFrame` | `Capture.acquireCameraFrame(...)` | `Capture.acquireCameraFrame(timestampNs: bigint[]): Surface` | function |
| `Moss_ReleaseCameraFrame` | `Capture.releaseCameraFrame(...)` | `Capture.releaseCameraFrame(frame: Surface): void` | function |
| `Moss_GetCameraFormat` | `Capture.getCameraFormat(...)` | `Capture.getCameraFormat(outSpec: CameraSpec): boolean` | function |
| `Moss_GetCameraPermissionState` | `Capture.getCameraPermissionState(...)` | `Capture.getCameraPermissionState(): CameraPermissionState` | function |
| `Moss_GetCameraProperties` (overload 1) | `Capture.getCameraProperties(...)` | `Capture.getCameraProperties(): PropertiesId` | function |
| `Moss_CloseCamera` | `Capture.closeCamera(...)` | `Capture.closeCamera(): void` | function |
| `Moss_GetCameraID` | `Capture.getCameraId(...)` | `Capture.getCameraId(): CameraId` | function |
| `Moss_GetCameraProperties` (overload 2) | `Capture.getCameraProperties(...)` | `Capture.getCameraProperties(): PropertiesId` | function |
| `Moss_OpenCapture` | `CameraSpec.openCapture(...)` | `CameraSpec.openCapture(id: CameraId): Capture` | function |
| `Moss_CopyFile` | `platform.storage.copyFile(...)` | `platform.storage.copyFile(srcPath: string, dstPath: string, overwrite: boolean): boolean` | function |
| `Moss_CreateDirectory` | `platform.storage.createDirectory(...)` | `platform.storage.createDirectory(path: string, recursive: boolean): boolean` | function |
| `Moss_RemovePath` | `platform.storage.removePath(...)` | `platform.storage.removePath(path: string, recursive: boolean): boolean` | function |
| `Moss_RenamePath` | `platform.storage.renamePath(...)` | `platform.storage.renamePath(oldPath: string, newPath: string, overwrite: boolean): boolean` | function |
| `Moss_GetPathInfo` | `platform.storage.getPathInfo(...)` | `platform.storage.getPathInfo(path: string, outInfo: PathInfo): boolean` | function |
| `Moss_GetCurrentDirectory` | `platform.storage.getCurrentDirectory(...)` | `platform.storage.getCurrentDirectory(outPath: Uint8Array, maxLen: number): boolean` | function |
| `Moss_GetBasePath` | `platform.storage.getBasePath(...)` | `platform.storage.getBasePath(outPath: Uint8Array, maxLen: number): boolean` | function |
| `Moss_GetUserFolder` | `platform.getUserFolder(...)` | `platform.getUserFolder(folder: UserFolder, outPath: Uint8Array, maxLen: number): boolean` | function |
| `Moss_GetPrefPath` | `platform.storage.getPrefPath(...)` | `platform.storage.getPrefPath(orgName: string, appName: string, outPath: Uint8Array, maxLen: number): boolean` | function |
| `Moss_EnumerateDirectory` | `platform.storage.enumerateDirectory(...)` | `platform.storage.enumerateDirectory(path: string, recursive: boolean, callback: DirectoryIterateFn): boolean` | function |
| `Moss_GlobDirectory` | `platform.storage.globDirectory(...)` | `platform.storage.globDirectory(pattern: string, callback: DirectoryIterateFn): boolean` | function |
| `Moss_ShowFileDialogWithProperties` | `DialogFileFilter.showFileDialogWithProperties(...)` | `DialogFileFilter.showFileDialogWithProperties(callback: DialogFileCallback, window: Window, nfilters: number, defaultLocation: string): void` | function |
| `Moss_ShowOpenFolderDialog` | `DialogFileFilter.showOpenFolderDialog(...)` | `DialogFileFilter.showOpenFolderDialog(callback: DialogFileCallback, window: Window, nfilters: number, defaultLocation: string, allowMany: boolean): void` | function |
| `Moss_ShowOpenFileDialog` | `Window.showOpenFileDialog(...)` | `Window.showOpenFileDialog(callback: DialogFileCallback, defaultLocation: string, allowMany: boolean): void` | function |
| `Moss_ShowSaveFileDialog` | `platform.dialogs.showSaveFileDialog(...)` | `platform.dialogs.showSaveFileDialog(type: FileDialogType, callback: DialogFileCallback, props: PropertiesId): void` | function |
| `Moss_CloseStorage` | `Storage.dispose(...)` | `Storage.dispose(): boolean` | function |
| `Moss_CopyStorageFile` | `Storage.copyFile(...)` | `Storage.copyFile(oldpath: string, newpath: string): boolean` | function |
| `Moss_CreateStorageDirectory` | `Storage.createDirectory(...)` | `Storage.createDirectory(path: string): boolean` | function |
| `Moss_EnumerateStorageDirectory` | `Storage.enumerateDirectory(...)` | `Storage.enumerateDirectory(path: string, callback: EnumerateDirectoryCallback): boolean` | function |
| `Moss_GetStorageFileSize` | `Storage.getFileSize(...)` | `Storage.getFileSize(path: string, length: Uint64): boolean` | function |
| `Moss_GetStorageSpaceRemaining` | `Storage.getSpaceRemaining(...)` | `Storage.getSpaceRemaining(): bigint` | function |
| `Moss_GetStoragePathInfo` | `Storage.getPathInfo(...)` | `Storage.getPathInfo(path: string, info: PathInfo): boolean` | function |
| `Moss_GlobStorageDirectory` | `Storage.globDirectory(...)` | `Storage.globDirectory(path: string, pattern: string, flags: GlobFlags, count: number[]): string \| null` | function |
| `Moss_OpenFileStorage` | `platform.storage.openFileStorage(...)` | `platform.storage.openFileStorage(path: string): Storage` | function |
| `Moss_OpenStorage` | `Storage.open(...)` | `Storage.open(): Storage` | function |
| `Moss_OpenTitleStorage` | `platform.storage.openTitleStorage(...)` | `platform.storage.openTitleStorage(override: string, props: PropertiesId): Storage` | function |
| `Moss_OpenUserStorage` | `platform.storage.openUserStorage(...)` | `platform.storage.openUserStorage(org: string, app: string, props: PropertiesId): Storage` | function |
| `Moss_ReadStorageFile` | `Storage.readFile(...)` | `Storage.readFile(path: string, destination: Uint8Array, length: Uint64): boolean` | function |
| `Moss_RemoveStoragePath` | `Storage.removePath(...)` | `Storage.removePath(path: string): boolean` | function |
| `Moss_RenameStoragePath` | `Storage.renamePath(...)` | `Storage.renamePath(oldpath: string, newpath: string): boolean` | function |
| `Moss_StorageReady` | `Storage.ready(...)` | `Storage.ready(): boolean` | function |
| `Moss_WriteStorageFile` | `Storage.writeFile(...)` | `Storage.writeFile(path: string, source: Uint8Array, length: Uint64): boolean` | function |
| `Moss_CreateEmbeddedWindow` | `platform.createEmbeddedWindow(...)` | `platform.createEmbeddedWindow(): Window` | function |
| `Moss_Terminate_EmbeddedWindow` | `platform.terminateEmbeddedWindow(...)` | `platform.terminateEmbeddedWindow(): void` | function |
| `Moss_EmbeddedWindow_OnOrientationChanged` | `platform.embeddedWindowOnOrientationChanged(...)` | `platform.embeddedWindowOnOrientationChanged(focused: boolean): void` | function |
| `Moss_EmbeddedWindow_OnResume` | `platform.embeddedWindowOnResume(...)` | `platform.embeddedWindowOnResume(): void` | function |
| `Moss_EmbeddedWindow_OnPause` | `platform.embeddedWindowOnPause(...)` | `platform.embeddedWindowOnPause(): void` | function |
| `Moss_MakeContextCurrent` | `Window.makeContextCurrent(...)` | `Window.makeContextCurrent(): void` | function |
| `Moss_SwapBuffers` | `platform.swapBuffers(...)` | `platform.swapBuffers(): void` | function |
| `Moss_SwapBuffersInterval` | `platform.swapBuffersInterval(...)` | `platform.swapBuffersInterval(interval: number): void` | function |
| `Moss_GetProcAddress` | `platform.getProcAddress(...)` | `platform.getProcAddress(procname: string): Uint8Array` | function |
| `Moss_CreateWindowSurface` | `Window.createVulkanSurface(...)` | `Window.createVulkanSurface(instance: VkInstance, allocator?: VkAllocationCallbacks): VkSurfaceKHR` | function |
| `Moss_VulkanSupported` | `platform.supported(...)` | `platform.supported(): number` | function |
| `Moss_GetRequiredInstanceExtensions` | `platform.getRequiredInstanceExtensions(...)` | `platform.getRequiredInstanceExtensions(count: number[]): string \| null` | function |
| `Moss_GetInstanceProcAddress` | `platform.getInstanceProcAddress(...)` | `platform.getInstanceProcAddress(instance: VkInstance, procname: string): Uint8Array` | function |
| `Moss_GetPhysicalDevicePresentationSupport` | `Window.getPhysicalDevicePresentationSupport(...)` | `Window.getPhysicalDevicePresentationSupport(device: VkPhysicalDevice, queuefamily: number): number` | function |
| `Moss_Metal_CreateView` | `Window.createView(...)` | `Window.createView(): MetalView` | function |
| `Moss_Metal_DestroyView` | `platform.destroyView(...)` | `platform.destroyView(view: MetalView): void` | function |
| `Moss_Metal_GetLayer` | `platform.getLayer(...)` | `platform.getLayer(view: MetalView): Uint8Array` | function |
| `Moss_Metal_Resize` | `platform.resize(...)` | `platform.resize(handle: MetalView, width: number, height: number): void` | function |

### `Moss_Renderer.h` → `renderer`

| Native symbol (traceability) | Public API projection | Target-language signature | Kind |
|---|---|---|---|
| `Moss_RendererCreateMaterial` | `Renderer.createMaterial(...)` | `Renderer.createMaterial(desc: Readonly<MaterialDesc>): Material` | function |
| `Moss_RendererDestroyMaterial` | `Renderer.destroyMaterial(...)` | `Renderer.destroyMaterial(material: Material): void` | function |
| `Moss_MaterialSetUniform` | `Material.setUniform(...)` | `Material.setUniform(name: string, data: Uint8Array, size: number): number` | function |
| `Moss_MaterialSetTexture` | `Material.setTexture(...)` | `Material.setTexture(type: MaterialTextureType, texture: Texture, sampler: GpuSampler): number` | function |
| `Moss_RendererCreatePipeline` | `Renderer.createPipeline(...)` | `Renderer.createPipeline(desc: Readonly<PipelineDesc>): PipelineState` | function |
| `Moss_PipelineBind` | `renderer.pipelineBind(...)` | `renderer.pipelineBind(pipeline: PipelineState): void` | function |
| `Moss_PipelineUnbind` | `renderer.pipelineUnbind(...)` | `renderer.pipelineUnbind(pipeline: PipelineState): void` | function |
| `Moss_PipelineSetUniform` | `renderer.pipelineSetUniform(...)` | `renderer.pipelineSetUniform(pipeline: PipelineState, name: string, data: Uint8Array, size: number): void` | function |
| `Moss_PipelineFlush` | `renderer.pipelineFlush(...)` | `renderer.pipelineFlush(pipeline: PipelineState): void` | function |
| `Moss_PipelineValidate` | `renderer.pipelineValidate(...)` | `renderer.pipelineValidate(pipeline: PipelineState): number` | function |
| `Moss_RendererCreateResourceSet` | `Renderer.createResourceSet(...)` | `Renderer.createResourceSet(desc: Readonly<ResourceSetLayoutDesc>): ResourceSet` | function |
| `Moss_RendererDestroyResourceSet` | `Renderer.destroyResourceSet(...)` | `Renderer.destroyResourceSet(set: ResourceSet): void` | function |
| `Moss_ResourceSetBindUniformBuffer` | `ResourceSet.bindUniformBuffer(...)` | `ResourceSet.bindUniformBuffer(binding: number, buffer: GpuBuffer): void` | function |
| `Moss_ResourceSetBindUniformBufferRange` | `ResourceSet.bindUniformBufferRange(...)` | `ResourceSet.bindUniformBufferRange(binding: number, buffer: GpuBuffer, offset: number, size: number): void` | function |
| `Moss_ResourceSetBindStorageBuffer` | `ResourceSet.bindStorageBuffer(...)` | `ResourceSet.bindStorageBuffer(binding: number, buffer: GpuBuffer): void` | function |
| `Moss_ResourceSetBindTexture` | `ResourceSet.bindTexture(...)` | `ResourceSet.bindTexture(binding: number, texture: Texture): void` | function |
| `Moss_ResourceSetBindSampler` | `ResourceSet.bindSampler(...)` | `ResourceSet.bindSampler(binding: number, sampler: GpuSampler): void` | function |
| `Moss_ResourceSetBind` | `Renderer.resourceSetBind(...)` | `Renderer.resourceSetBind(pipeline: PipelineState, setIndex: number, set: ResourceSet): void` | function |
| `Moss_RendererCreateFramebuffer` | `Renderer.createFramebuffer(...)` | `Renderer.createFramebuffer(desc: Readonly<FramebufferDesc>): Framebuffer` | function |
| `Moss_RendererDestroyFramebuffer` | `Renderer.destroyFramebuffer(...)` | `Renderer.destroyFramebuffer(framebuffer: Framebuffer): void` | function |
| `Moss_FramebufferResize` | `Framebuffer.resize(...)` | `Framebuffer.resize(width: number, height: number): void` | function |
| `Moss_PostProcessExecute` | `Renderer.postProcessExecute(...)` | `Renderer.postProcessExecute(pass: Readonly<PostProcessPass>): void` | function |
| `Moss_FramebufferBegin` | `Renderer.framebufferBegin(...)` | `Renderer.framebufferBegin(framebuffer: Framebuffer): void` | function |
| `Moss_FramebufferEnd` | `Renderer.framebufferEnd(...)` | `Renderer.framebufferEnd(framebuffer: Framebuffer): void` | function |
| `Moss_TextureBarrier` | `Renderer.textureBarrier(...)` | `Renderer.textureBarrier(texture: Texture, oldState: ResourceState, newState: ResourceState): void` | function |
| `Moss_RendererGetBackbuffer` | `Renderer.getBackbuffer(...)` | `Renderer.getBackbuffer(): Framebuffer` | function |
| `Moss_RenderSubViewport` | `SubViewport.render(...)` | `SubViewport.render(cmd: GpuCommandBuffer, record: SubViewportRecordFn): void` | function |
| `Moss_RendererCreate` | `renderer.create(...)` | `renderer.create(desc: Readonly<RendererDesc>): Renderer` | function |
| `Moss_CreateRenderer` | `renderer.createRenderer(...)` | `renderer.createRenderer(window: Window): Renderer` | function |
| `Moss_RendererDestroy` | `Renderer.dispose(...)` | `Renderer.dispose(): void` | function |
| `Moss_TerminateRenderer` | `Renderer.dispose(...)` | `Renderer.dispose(): void` | function |
| `Moss_RendererBeginFrame` | `Renderer.beginFrame(...)` | `Renderer.beginFrame(): void` | function |
| `Moss_RendererBeginFrameEx` | `Renderer.beginFrameEx(...)` | `Renderer.beginFrameEx(desc: Readonly<RenderFrameDesc>): void` | function |
| `Moss_RendererEndFrame` | `Renderer.endFrame(...)` | `Renderer.endFrame(): void` | function |
| `Moss_RendererGetGPUDevice` | `Renderer.getGpuDevice(...)` | `Renderer.getGpuDevice(): GpuDevice` | function |
| `Moss_MeshLoadFBX` | `renderer.meshLoadFbx(...)` | `renderer.meshLoadFbx(filePath: string): Mesh` | function |
| `Moss_MeshLoadOBJ` | `renderer.meshLoadObj(...)` | `renderer.meshLoadObj(filePath: string): Mesh` | function |
| `Moss_MeshLoadGLB` | `renderer.meshLoadGlb(...)` | `renderer.meshLoadGlb(filePath: string): Mesh` | function |
| `Moss_ModelLoadFBX` | `renderer.modelLoadFbx(...)` | `renderer.modelLoadFbx(filePath: string): Model` | function |
| `Moss_ModelLoadOBJ` | `renderer.modelLoadObj(...)` | `renderer.modelLoadObj(filePath: string): Model` | function |
| `Moss_ModelLoadGLB` | `renderer.modelLoadGlb(...)` | `renderer.modelLoadGlb(filePath: string): Model` | function |
| `Moss_ModelLoad` | `renderer.modelLoad(...)` | `renderer.modelLoad(filePath: string): Model` | function |
| `Moss_ModelGetLastLoadError` | `renderer.modelGetLastLoadError(...)` | `renderer.modelGetLastLoadError(): ModelLoadError` | function |
| `Moss_ModelGetLastLoadErrorMessage` | `renderer.modelGetLastLoadErrorMessage(...)` | `renderer.modelGetLastLoadErrorMessage(): string \| null` | function |
| `Moss_ModelFindBone` | `Model.findBone(...)` | `Model.findBone(name: string): number` | function |
| `Moss_ModelFindMorphTarget` | `Model.findMorphTarget(...)` | `Model.findMorphTarget(name: string): number` | function |
| `Moss_ModelSetBoneTransform` | `Model.setBoneTransform(...)` | `Model.setBoneTransform(boneIndex: number, transform16: number[]): boolean` | function |
| `Moss_ModelSetMorphWeight` | `Model.setMorphWeight(...)` | `Model.setMorphWeight(morphIndex: number, weight: number): boolean` | function |
| `Moss_RendererDrawMesh` | `Renderer.drawMesh(...)` | `Renderer.drawMesh(desc: Readonly<DrawMeshDesc>): void` | function |
| `Moss_RendererDrawModelDesc` | `Renderer.drawModelDesc(...)` | `Renderer.drawModelDesc(desc: Readonly<DrawModelDesc>): void` | function |
| `Moss_RendererDrawModel` | `Renderer.drawModel(...)` | `Renderer.drawModel(model: Readonly<Model>, transform: number[]): void` | function |
| `Moss_RendererCreateFont` | `Renderer.createFont(...)` | `Renderer.createFont(desc: Readonly<FontDesc>): Font` | function |
| `Moss_RendererDestroyFont` | `Renderer.destroyFont(...)` | `Renderer.destroyFont(font: Font): void` | function |
| `Moss_FontLoad` | `renderer.fontLoad(...)` | `renderer.fontLoad(path: string, pixelSize: number): Font` | function |
| `Moss_FontCreateFromMemory` | `renderer.fontCreateFromMemory(...)` | `renderer.fontCreateFromMemory(data: Uint8Array, dataSize: number, pixelSize: number): Font` | function |
| `Moss_FontDestroy` | `Font.dispose(...)` | `Font.dispose(): void` | function |
| `Moss_FontGetMetrics` | `Font.getMetrics(...)` | `Font.getMetrics(): FontMetrics` | function |
| `Moss_FontGetGlyph` | `Font.getGlyph(...)` | `Font.getGlyph(codepoint: number): FontGlyph` | function |
| `Moss_FontGetAtlasPixels` | `Font.getAtlasPixels(...)` | `Font.getAtlasPixels(width: number[], height: number[], stride: number[]): UnsignedChar` | function |
| `Moss_FontGetAtlasTexture` | `Font.getAtlasTexture(...)` | `Font.getAtlasTexture(): Texture` | function |
| `Moss_FontUploadAtlas` | `Renderer.fontUploadAtlas(...)` | `Renderer.fontUploadAtlas(font: Font): boolean` | function |
| `Moss_FontMeasureText` | `Font.measureText(...)` | `Font.measureText(text: string, scale: number): TextMeasure` | function |
| `Moss_FontBuildTextQuads` | `renderer.fontBuildTextQuads(...)` | `renderer.fontBuildTextQuads(desc: Readonly<DrawTextDesc>, outVertices: FontVertex, vertexCapacity: number): number` | function |
| `Moss_RendererDrawText` | `Renderer.drawText(...)` | `Renderer.drawText(desc: Readonly<DrawTextDesc>): void` | function |
| `Moss_RendererGetFontBackendMask` | `renderer.getFontBackendMask(...)` | `renderer.getFontBackendMask(): number` | function |
| `Moss_RendererBackendSupportsFonts` | `renderer.backendSupportsFonts(...)` | `renderer.backendSupportsFonts(backend: GpuBackendType): boolean` | function |
| `Moss_RendererDrawSprite` | `Renderer.drawSprite(...)` | `Renderer.drawSprite(desc: Readonly<SpriteDesc>): void` | function |
| `Moss_RendererCreateSpriteBatch` | `Renderer.createSpriteBatch(...)` | `Renderer.createSpriteBatch(capacity: number): SpriteBatch` | function |
| `Moss_RendererDestroySpriteBatch` | `Renderer.destroySpriteBatch(...)` | `Renderer.destroySpriteBatch(batch: SpriteBatch): void` | function |
| `Moss_SpriteBatchClear` | `SpriteBatch.clear(...)` | `SpriteBatch.clear(): void` | function |
| `Moss_SpriteBatchAdd` | `SpriteBatch.add(...)` | `SpriteBatch.add(desc: Readonly<SpriteDesc>): number` | function |
| `Moss_RendererDrawSpriteBatch` | `Renderer.drawSpriteBatch(...)` | `Renderer.drawSpriteBatch(batch: Readonly<SpriteBatch>): void` | function |
| `Moss_RendererCreateDebugDrawList` | `Renderer.createDebugDrawList(...)` | `Renderer.createDebugDrawList(capacity: number): DebugDrawList` | function |
| `Moss_RendererDestroyDebugDrawList` | `Renderer.destroyDebugDrawList(...)` | `Renderer.destroyDebugDrawList(list: DebugDrawList): void` | function |
| `Moss_DebugDrawListClear` | `DebugDrawList.clear(...)` | `DebugDrawList.clear(): void` | function |
| `Moss_DebugDrawLine` | `DebugDrawList.debugDrawLine(...)` | `DebugDrawList.debugDrawLine(desc: Readonly<DebugLineDesc>): number` | function |
| `Moss_DebugDrawBox` | `DebugDrawList.debugDrawBox(...)` | `DebugDrawList.debugDrawBox(desc: Readonly<DebugBoxDesc>): number` | function |
| `Moss_DebugDrawSphere` | `DebugDrawList.debugDrawSphere(...)` | `DebugDrawList.debugDrawSphere(desc: Readonly<DebugSphereDesc>): number` | function |
| `Moss_DebugDrawNavmesh` | `DebugDrawList.debugDrawNavmesh(...)` | `DebugDrawList.debugDrawNavmesh(desc: Readonly<DebugNavmeshDesc>): number` | function |
| `Moss_DebugDrawPhysicsContact` | `DebugDrawList.debugDrawPhysicsContact(...)` | `DebugDrawList.debugDrawPhysicsContact(desc: Readonly<DebugPhysicsContactDesc>): number` | function |
| `Moss_RendererDrawDebugList` | `Renderer.drawDebugList(...)` | `Renderer.drawDebugList(list: Readonly<DebugDrawList>): void` | function |
| `Moss_CameraCreate2D` | `renderer.cameraCreateDimension2D(...)` | `renderer.cameraCreateDimension2D(desc: Readonly<CameraDIMENSION2DDesc>): Camera` | function |
| `Moss_CameraCreate3D` | `renderer.cameraCreateDimension3D(...)` | `renderer.cameraCreateDimension3D(desc: Readonly<CameraDIMENSION3DDesc>): Camera` | function |
| `Moss_CameraCreateEditor` | `renderer.cameraCreateEditor(...)` | `renderer.cameraCreateEditor(desc: Readonly<EditorCameraDesc>): Camera` | function |
| `Moss_CameraDestroy` | `Camera.dispose(...)` | `Camera.dispose(): void` | function |
| `Moss_RendererSetCamera` | `Renderer.setCamera(...)` | `Renderer.setCamera(camera: Camera): void` | function |
| `Moss_RendererCreateShadowMap` | `Renderer.createShadowMap(...)` | `Renderer.createShadowMap(desc: Readonly<ShadowMapDesc>): ShadowMap` | function |
| `Moss_RendererDestroyShadowMap` | `Renderer.destroyShadowMap(...)` | `Renderer.destroyShadowMap(shadowMap: ShadowMap): void` | function |
| `Moss_RendererBeginShadowPass` | `Renderer.beginShadowPass(...)` | `Renderer.beginShadowPass(shadowMap: ShadowMap): void` | function |
| `Moss_RendererEndShadowPass` | `Renderer.endShadowPass(...)` | `Renderer.endShadowPass(shadowMap: ShadowMap): void` | function |
| `Moss_MeshDraw` | `Mesh.draw(...)` | `Mesh.draw(): void` | function |
| `Moss_ModelDraw` | `Model.draw(...)` | `Model.draw(): void` | function |
| `Moss_MeshRemove` | `Mesh.dispose(...)` | `Mesh.dispose(): void` | function |
| `Moss_ModelRemove` | `Model.dispose(...)` | `Model.dispose(): void` | function |
| `Moss_ResourceSetCreate` | `Renderer.createResourceSet(...)` | `Renderer.createResourceSet(desc: Readonly<ResourceSetLayoutDesc>): ResourceSet` | function |
| `Moss_ResourceSetDestroy` | `ResourceSet.dispose(...)` | `ResourceSet.dispose(): void` | function |
| `Moss_FramebufferCreate` | `Renderer.createFramebuffer(...)` | `Renderer.createFramebuffer(desc: Readonly<FramebufferDesc>): Framebuffer` | function |
| `Moss_FramebufferDestroy` | `Framebuffer.dispose(...)` | `Framebuffer.dispose(): void` | function |
| `Moss_RendererSetUpscaler` | `Renderer.setUpscaler(...)` | `Renderer.setUpscaler(upscaler: Upscaler): void` | function |
| `Moss_RendererSetUpscalerDesc` | `Renderer.setUpscalerDesc(...)` | `Renderer.setUpscalerDesc(desc: Readonly<RendererUpscalerDesc>): void` | function |
| `Moss_GPUGetPhysicalDeviceName` | `renderer.getPhysicalDeviceName(...)` | `renderer.getPhysicalDeviceName(): string \| null` | function |
| `Moss_GPUAPIGetName` | `renderer.gpuapiGetName(...)` | `renderer.gpuapiGetName(): string \| null` | function |
| `Moss_GPUAPIGetVersion` | `renderer.gpuapiGetVersion(...)` | `renderer.gpuapiGetVersion(): string \| null` | function |
| `Moss_RendererDrawLine2D` | `Renderer.drawLineDimension2D(...)` | `Renderer.drawLineDimension2D(from: Readonly<Vec2>, to: Readonly<Vec2>, color: Color, thickness: number): void` | function |
| `Moss_RendererDrawLines2D` | `Renderer.drawLinesDimension2D(...)` | `Renderer.drawLinesDimension2D(positions: Readonly<TArray<Vec2>>, color: Color, thickness: number): void` | function |
| `Moss_RendererDrawBox2D` | `Renderer.drawBoxDimension2D(...)` | `Renderer.drawBoxDimension2D(center: ReadonlyArray<Vec2>, rotation: number, color: Color): void` | function |
| `Moss_RendererDrawCircle2D` | `Renderer.drawCircleDimension2D(...)` | `Renderer.drawCircleDimension2D(center: Readonly<Vec2>, radius: number, color: Color): void` | function |
| `Moss_RendererDrawCylinder2D` | `Renderer.drawCylinderDimension2D(...)` | `Renderer.drawCylinderDimension2D(matrix: RMat44, halfHeight: number, radius: number, color: Color): void` | function |
| `Moss_RendererDrawTriangle2D` | `Renderer.drawTriangleDimension2D(...)` | `Renderer.drawTriangleDimension2D(p0: Readonly<Vec2>, p1: Readonly<Vec2>, p2: Readonly<Vec2>, color: Color): void` | function |
| `Moss_RendererDrawArrow2D` | `Renderer.drawArrowDimension2D(...)` | `Renderer.drawArrowDimension2D(from: Readonly<Vec2>, to: Readonly<Vec2>, color: Color, headSize: number): void` | function |
| `Moss_RendererDrawMarker2D` | `Renderer.drawMarkerDimension2D(...)` | `Renderer.drawMarkerDimension2D(position: Readonly<Vec2>, color: Color, size: number): void` | function |
| `Moss_RendererDrawBone2D` | `Renderer.drawBoneDimension2D(...)` | `Renderer.drawBoneDimension2D(transform: RMat44, length: number, color: Color): void` | function |
| `Moss_RendererDrawCoordinateSystem2D` | `Renderer.drawCoordinateSystemDimension2D(...)` | `Renderer.drawCoordinateSystemDimension2D(matrix: RMat44, size: number): void` | function |
| `Moss_RendererDrawCapsule2D` | `Renderer.drawCapsuleDimension2D(...)` | `Renderer.drawCapsuleDimension2D(matrix: RMat44, halfHeightOfCylinder: number, radius: number, color: Color): void` | function |
| `Moss_RendererDrawConvex2D` | `Renderer.drawConvexDimension2D(...)` | `Renderer.drawConvexDimension2D(points: ReadonlyArray<Vec2>, color: Color): void` | function |
| `Moss_RendererDrawConvexHull2D` | `Renderer.drawConvexHullDimension2D(...)` | `Renderer.drawConvexHullDimension2D(points: ReadonlyArray<Vec2>, color: Color): void` | function |
| `Moss_RendererDrawPolygonShape2D` | `Renderer.drawPolygonShapeDimension2D(...)` | `Renderer.drawPolygonShapeDimension2D(points: ReadonlyArray<Vec2>, color: Color): void` | function |
| `Moss_RendererDrawGizmo2D` | `Renderer.drawGizmoDimension2D(...)` | `Renderer.drawGizmoDimension2D(transform: RMat44, size: number): void` | function |
| `Moss_RendererDrawLineBatch2D` | `Renderer.drawLineBatchDimension2D(...)` | `Renderer.drawLineBatchDimension2D(transform: RMat44, colour: Color, count: number): void` | function |
| `Moss_RendererDrawBoxBatch2D` | `Renderer.drawBoxBatchDimension2D(...)` | `Renderer.drawBoxBatchDimension2D(transform: RMat44, colour: Color, count: number): void` | function |
| `Moss_RendererDrawCircleBatch2D` | `Renderer.drawCircleBatchDimension2D(...)` | `Renderer.drawCircleBatchDimension2D(transform: RMat44, colour: Color, count: number): void` | function |
| `Moss_RendererDrawArrowBatch2D` | `Renderer.drawArrowBatchDimension2D(...)` | `Renderer.drawArrowBatchDimension2D(transform: RMat44, colour: Color, count: number): void` | function |
| `Moss_RendererDrawMarkerBatch2D` | `Renderer.drawMarkerBatchDimension2D(...)` | `Renderer.drawMarkerBatchDimension2D(transform: RMat44, colour: Color, count: number): void` | function |
| `Moss_RendererDrawBoxInstanced2D` | `Renderer.drawBoxInstancedDimension2D(...)` | `Renderer.drawBoxInstancedDimension2D(size: Readonly<Vec2>, transform: RMat44, colour: Color, count: number): void` | function |
| `Moss_RendererDrawCircleInstanced2D` | `Renderer.drawCircleInstancedDimension2D(...)` | `Renderer.drawCircleInstancedDimension2D(radius: number, transform: RMat44, colour: Color, count: number): void` | function |
| `Moss_RendererDrawCapsuleInstanced2D` | `Renderer.drawCapsuleInstancedDimension2D(...)` | `Renderer.drawCapsuleInstancedDimension2D(halfHeightOfCylinder: number, radius: number, transform: RMat44, colour: Color, count: number): void` | function |
| `Moss_RendererDrawMarkerInstanced2D` | `Renderer.drawMarkerInstancedDimension2D(...)` | `Renderer.drawMarkerInstancedDimension2D(size: number, transform: RMat44, colour: Color, count: number): void` | function |
| `Moss_RendererDrawGizmoInstanced2D` | `Renderer.drawGizmoInstancedDimension2D(...)` | `Renderer.drawGizmoInstancedDimension2D(size: number, transform: RMat44, count: number): void` | function |
| `Moss_RendererDrawLine3D` | `Renderer.drawLineDimension3D(...)` | `Renderer.drawLineDimension3D(from: Readonly<Vec3>, to: Readonly<Vec3>, color: Color, thickness: number): void` | function |
| `Moss_RendererDrawLines3D` | `Renderer.drawLinesDimension3D(...)` | `Renderer.drawLinesDimension3D(positions: Readonly<TArray<Vec3>>, color: Color, thickness: number): void` | function |
| `Moss_RendererDrawBox3D` | `Renderer.drawBoxDimension3D(...)` | `Renderer.drawBoxDimension3D(center: ReadonlyArray<Vec3>, rotation: RMat44, color: Color): void` | function |
| `Moss_RendererDrawSphere3D` | `Renderer.drawSphereDimension3D(...)` | `Renderer.drawSphereDimension3D(center: Readonly<Vec3>, radius: number, color: Color): void` | function |
| `Moss_RendererDrawCylinder3D` | `Renderer.drawCylinderDimension3D(...)` | `Renderer.drawCylinderDimension3D(base: Readonly<Vec3>, top: Readonly<Vec3>, radius: number, color: Color): void` | function |
| `Moss_RendererDrawCone3D` | `Renderer.drawConeDimension3D(...)` | `Renderer.drawConeDimension3D(base: Readonly<Vec3>, tip: Readonly<Vec3>, radius: number, color: Color): void` | function |
| `Moss_RendererDrawTriangle3D` | `Renderer.drawTriangleDimension3D(...)` | `Renderer.drawTriangleDimension3D(p0: Readonly<Vec3>, p1: Readonly<Vec3>, p2: Readonly<Vec3>, color: Color): void` | function |
| `Moss_RendererDrawArrow3D` | `Renderer.drawArrowDimension3D(...)` | `Renderer.drawArrowDimension3D(from: Readonly<Vec3>, to: Readonly<Vec3>, color: Color, headSize: number): void` | function |
| `Moss_RendererDrawMarker3D` | `Renderer.drawMarkerDimension3D(...)` | `Renderer.drawMarkerDimension3D(position: Readonly<Vec3>, color: Color, size: number): void` | function |
| `Moss_RendererDrawPlane3D` | `Renderer.drawPlaneDimension3D(...)` | `Renderer.drawPlaneDimension3D(center: Readonly<Vec3>, normal: ReadonlyArray<Vec3>, color: Color): void` | function |
| `Moss_RendererDrawBone3D` | `Renderer.drawBoneDimension3D(...)` | `Renderer.drawBoneDimension3D(transform: RMat44, length: number, color: Color): void` | function |
| `Moss_RendererDrawCoordinateSystem3D` | `Renderer.drawCoordinateSystemDimension3D(...)` | `Renderer.drawCoordinateSystemDimension3D(matrix: RMat44, size: number): void` | function |
| `Moss_RendererDrawCapsule3D` | `Renderer.drawCapsuleDimension3D(...)` | `Renderer.drawCapsuleDimension3D(matrix: RMat44, halfHeightOfCylinder: number, radius: number, color: Color): void` | function |
| `Moss_RendererDrawConvex3D` | `Renderer.drawConvexDimension3D(...)` | `Renderer.drawConvexDimension3D(points: ReadonlyArray<Vec3>, color: Color): void` | function |
| `Moss_RendererDrawConvexHull3D` | `Renderer.drawConvexHullDimension3D(...)` | `Renderer.drawConvexHullDimension3D(points: ReadonlyArray<Vec3>, color: Color): void` | function |
| `Moss_RendererDrawPolygonShape3D` | `Renderer.drawPolygonShapeDimension3D(...)` | `Renderer.drawPolygonShapeDimension3D(points: ReadonlyArray<Vec3>, color: Color): void` | function |
| `Moss_RendererDrawGizmo3D` | `Renderer.drawGizmoDimension3D(...)` | `Renderer.drawGizmoDimension3D(transform: RMat44, size: number): void` | function |
| `Moss_RendererDrawLineBatch3D` | `Renderer.drawLineBatchDimension3D(...)` | `Renderer.drawLineBatchDimension3D(transform: RMat44, colour: Color, count: number): void` | function |
| `Moss_RendererDrawBoxBatch3D` | `Renderer.drawBoxBatchDimension3D(...)` | `Renderer.drawBoxBatchDimension3D(transform: RMat44, colour: Color, count: number): void` | function |
| `Moss_RendererDrawSphereBatch3D` | `Renderer.drawSphereBatchDimension3D(...)` | `Renderer.drawSphereBatchDimension3D(transform: RMat44, colour: Color, count: number): void` | function |
| `Moss_RendererDrawArrowBatch3D` | `Renderer.drawArrowBatchDimension3D(...)` | `Renderer.drawArrowBatchDimension3D(transform: RMat44, colour: Color, count: number): void` | function |
| `Moss_RendererDrawMarkerBatch3D` | `Renderer.drawMarkerBatchDimension3D(...)` | `Renderer.drawMarkerBatchDimension3D(transform: RMat44, colour: Color, count: number): void` | function |
| `Moss_RendererDrawBoxInstanced3D` | `Renderer.drawBoxInstancedDimension3D(...)` | `Renderer.drawBoxInstancedDimension3D(size: Readonly<Vec3>, transform: RMat44, colour: Color, count: number): void` | function |
| `Moss_RendererDrawSphereInstanced3D` | `Renderer.drawSphereInstancedDimension3D(...)` | `Renderer.drawSphereInstancedDimension3D(radius: number, transform: RMat44, colour: Color, count: number): void` | function |
| `Moss_RendererDrawCapsuleInstanced3D` | `Renderer.drawCapsuleInstancedDimension3D(...)` | `Renderer.drawCapsuleInstancedDimension3D(halfHeightOfCylinder: number, radius: number, transform: RMat44, colour: Color, count: number): void` | function |
| `Moss_RendererDrawCylinderInstanced3D` | `Renderer.drawCylinderInstancedDimension3D(...)` | `Renderer.drawCylinderInstancedDimension3D(halfHeight: number, radius: number, transform: RMat44, colour: Color, count: number): void` | function |
| `Moss_RendererDrawConeInstanced3D` | `Renderer.drawConeInstancedDimension3D(...)` | `Renderer.drawConeInstancedDimension3D(height: number, radius: number, transform: RMat44, colour: Color, count: number): void` | function |
| `Moss_RendererDrawMarkerInstanced3D` | `Renderer.drawMarkerInstancedDimension3D(...)` | `Renderer.drawMarkerInstancedDimension3D(size: number, transform: RMat44, colour: Color, count: number): void` | function |
| `Moss_RendererDrawGizmoInstanced3D` | `Renderer.drawGizmoInstancedDimension3D(...)` | `Renderer.drawGizmoInstancedDimension3D(size: number, transform: RMat44, count: number): void` | function |
| `Moss_VulkanGetDevice` | `Renderer.getDevice(...)` | `Renderer.getDevice(): VkDevice` | function |
| `Moss_VulkanGetDescriptorPool` | `Renderer.getDescriptorPool(...)` | `Renderer.getDescriptorPool(): VkDescriptorPool` | function |
| `Moss_VulkanGetDescriptorSetLayoutTexture` | `Renderer.getDescriptorSetLayoutTexture(...)` | `Renderer.getDescriptorSetLayoutTexture(): VkDescriptorSetLayout` | function |
| `Moss_VulkanGetTextureSamplerRepeat` | `Renderer.getTextureSamplerRepeat(...)` | `Renderer.getTextureSamplerRepeat(): VkSampler` | function |
| `Moss_VulkanGetTextureSamplerShadow` | `Renderer.getTextureSamplerShadow(...)` | `Renderer.getTextureSamplerShadow(): VkSampler` | function |
| `Moss_VulkanGetRenderPassShadow` | `Renderer.getRenderPassShadow(...)` | `Renderer.getRenderPassShadow(): VkRenderPass` | function |
| `Moss_VulkanGetRenderPass` | `Renderer.getRenderPass(...)` | `Renderer.getRenderPass(): VkRenderPass` | function |
| `Moss_VulkanGetPipelineLayout` | `Renderer.getPipelineLayout(...)` | `Renderer.getPipelineLayout(): VkPipelineLayout` | function |
| `Moss_VulkanGetCommandBuffer` | `Renderer.getCommandBuffer(...)` | `Renderer.getCommandBuffer(): VkCommandBuffer` | function |
| `Moss_VulkanStartTempCommandBuffer` | `Renderer.startTempCommandBuffer(...)` | `Renderer.startTempCommandBuffer(): VkCommandBuffer` | function |
| `Moss_VulkanEndTempCommandBuffer` | `Renderer.endTempCommandBuffer(...)` | `Renderer.endTempCommandBuffer(commandBuffer: VkCommandBuffer): void` | function |
| `Moss_VulkanAllocateMemory` | `Renderer.allocateMemory(...)` | `Renderer.allocateMemory(size: VkDeviceSize, memoryTypeBits: number, properties: VkMemoryPropertyFlags, outMemory: VkDeviceMemory): void` | function |
| `Moss_VulkanFreeMemory` | `Renderer.freeMemory(...)` | `Renderer.freeMemory(memory: VkDeviceMemory, size: VkDeviceSize): void` | function |
| `Moss_VulkanCreateBuffer` | `Renderer.createBuffer(...)` | `Renderer.createBuffer(size: VkDeviceSize, usage: VkBufferUsageFlags, properties: VkMemoryPropertyFlags, outBuffer: BufferVK): void` | function |
| `Moss_VulkanCopyBuffer` | `Renderer.copyBuffer(...)` | `Renderer.copyBuffer(src: VkBuffer, dst: VkBuffer, size: VkDeviceSize): void` | function |
| `Moss_VulkanCreateDeviceLocalBuffer` | `Renderer.createDeviceLocalBuffer(...)` | `Renderer.createDeviceLocalBuffer(data: Uint8Array, size: VkDeviceSize, usage: VkBufferUsageFlags, outBuffer: BufferVK): void` | function |
| `Moss_VulkanFreeBuffer` | `Renderer.freeBuffer(...)` | `Renderer.freeBuffer(buffer: BufferVK): void` | function |
| `Moss_VulkanCreateConstantBuffer` | `Renderer.createConstantBuffer(...)` | `Renderer.createConstantBuffer(bufferSize: VkDeviceSize): ConstantBufferVK` | function |
| `Moss_VulkanCreateImage` | `Renderer.createImage(...)` | `Renderer.createImage(width: number, height: number, format: VkFormat, tiling: VkImageTiling, usage: VkImageUsageFlags, properties: VkMemoryPropertyFlags, outImage: VkImage, outMemory: VkDeviceMemory): void` | function |
| `Moss_VulkanDestroyImage` | `Renderer.destroyImage(...)` | `Renderer.destroyImage(image: VkImage, memory: VkDeviceMemory): void` | function |
| `Moss_VulkanCreateImageView` | `Renderer.createImageView(...)` | `Renderer.createImageView(image: VkImage, format: VkFormat, aspectFlags: VkImageAspectFlags): VkImageView` | function |
| `Moss_VulkanFindDepthFormat` | `Renderer.findDepthFormat(...)` | `Renderer.findDepthFormat(): VkFormat` | function |
| `Moss_Vulkan_EncodeSPIRV` | `renderer.encodeSpirv(...)` | `renderer.encodeSpirv(path: string): void` | function |
| `Moss_Vulkan_DecodeSPIRV` | `renderer.decodeSpirv(...)` | `renderer.decodeSpirv(path: string): void` | function |
| `Moss_DX12GetDevice` | `Renderer.dx12GetDevice(...)` | `Renderer.dx12GetDevice(): IdDIMENSION3D1DIMENSION2Device` | function |
| `Moss_DX12GetRootSignature` | `Renderer.dx12GetRootSignature(...)` | `Renderer.dx12GetRootSignature(): IdDIMENSION3D12RootSignature` | function |
| `Moss_DX12GetCommandList` | `Renderer.dx12GetCommandList(...)` | `Renderer.dx12GetCommandList(): IdDIMENSION3D12GraphicsCommandList` | function |
| `Moss_DX12GetUploadQueue` | `Renderer.dx12GetUploadQueue(...)` | `Renderer.dx12GetUploadQueue(): CommandQueueDX12` | function |
| `Moss_DX12GetDSVHeap` | `Renderer.dx12GetDsvHeap(...)` | `Renderer.dx12GetDsvHeap(): DescriptorHeapDX12` | function |
| `Moss_DX12GetSRVHeap` | `Renderer.dx12GetSrvHeap(...)` | `Renderer.dx12GetSrvHeap(): DescriptorHeapDX12` | function |
| `Moss_DX_CompileHLSL` | `renderer.compileHlsl(...)` | `renderer.compileHlsl(inputPath: string, outputPath: string, entryPoint: string, target: string): boolean` | function |
| `Moss_DX_EnableDebugLayer` | `renderer.enableDebugLayer(...)` | `renderer.enableDebugLayer(enable: boolean): void` | function |
| `Moss_DX_GetShaderModel` | `renderer.getShaderModel(...)` | `renderer.getShaderModel(): number` | function |
| `Moss_MetalGetView` | `Renderer.getView(...)` | `Renderer.getView(): MTKView` | function |
| `Moss_MetalGetDevice` | `Renderer.getDevice(...)` | `Renderer.getDevice(): Uint8Array` | function |
| `Moss_MetalGetRenderEncoder` | `Renderer.getRenderEncoder(...)` | `Renderer.getRenderEncoder(): Uint8Array` | function |
| `Moss_Metal_CompileMSL` | `renderer.compileMsl(...)` | `renderer.compileMsl(inputPath: string, outputPath: string): boolean` | function |
| `Moss_Metal_SupportsFamily` | `renderer.supportsFamily(...)` | `renderer.supportsFamily(family: number): boolean` | function |

### `Moss_stdinc.h` → `core`

| Native symbol (traceability) | Public API projection | Target-language signature | Kind |
|---|---|---|---|
| `Allocate` | `core.allocator.allocate` | `type allocate = (...args: unknown[]) => unknown` | callback |
| `Reallocate` | `core.allocator.reallocate` | `type reallocate = (...args: unknown[]) => unknown` | callback |
| `Free` | `core.allocator.free` | `type free = (...args: unknown[]) => unknown` | callback |
| `AlignedAllocate` | `core.allocator.alignedAllocate` | `type alignedAllocate = (...args: unknown[]) => unknown` | callback |
| `AlignedFree` | `core.allocator.alignedFree` | `type alignedFree = (...args: unknown[]) => unknown` | callback |
| `RegisterDefaultAllocator` | `core.registerDefaultAllocator(...)` | `core.registerDefaultAllocator(): void` | function |
| `Allocate` | `core.allocate(...)` | `core.allocate(inSize: number): Uint8Array` | function |
| `Reallocate` | `core.reallocate(...)` | `core.reallocate(inBlock: Uint8Array, inOldSize: number, inNewSize: number): Uint8Array` | function |
| `Free` | `core.free(...)` | `core.free(inBlock: Uint8Array): void` | function |
| `AlignedAllocate` | `core.alignedAllocate(...)` | `core.alignedAllocate(inSize: number, inAlignment: number): Uint8Array` | function |
| `AlignedFree` | `core.alignedFree(...)` | `core.alignedFree(inBlock: Uint8Array): void` | function |
| `Moss_rand` | `core.rand(...)` | `core.rand(n: Int32): Int32` | function |
| `Moss_randf` | `core.randf(...)` | `core.randf(): number` | function |
| `Moss_randi` | `core.randi(...)` | `core.randi(): number` | function |
| `Moss_srand` | `core.srand(...)` | `core.srand(seed: Int64): void` | function |
| `Moss_randi_range` | `core.randiRange(...)` | `core.randiRange(from: Int32, to: Int32): Int32` | function |
| `Moss_randf_range` | `core.randfRange(...)` | `core.randfRange(from: number, to: number): number` | function |
| `Moss_rand_gaussian` | `core.randGaussian(...)` | `core.randGaussian(mean: number, stddev: number): number` | function |
| `Moss_rand_unit` | `core.randUnit(...)` | `core.randUnit(): number` | function |
| `Moss_GetTicks` | `core.getTicks(...)` | `core.getTicks(): Time` | function |
| `Moss_GetSeconds` | `core.getSeconds(...)` | `core.getSeconds(time: Time): number` | function |
| `Moss_GetMilliseconds` | `core.getMilliseconds(...)` | `core.getMilliseconds(ticks: Time): number` | function |
| `Moss_GetMillisecondsAndReset` | `core.getMillisecondsAndReset(...)` | `core.getMillisecondsAndReset(ticks: Time): number` | function |
| `Moss_Yield` | `core.yield(...)` | `core.yield(): void` | function |
| `Moss_Delay` | `core.delay(...)` | `core.delay(delay: number): number` | function |
| `Moss_LocalTime` | `core.localTime(...)` | `core.localTime(): string \| null` | function |
| `Moss_TimeStamp` | `core.timeStamp(...)` | `core.timeStamp(): string \| null` | function |
| `Moss_TimeNow` | `core.timeNow(...)` | `core.timeNow(): string \| null` | function |
| `Moss_CTimeNow` | `core.cTimeNow(...)` | `core.cTimeNow(): string \| null` | function |
| `Moss_TimeStampET` | `core.timeStampEt(...)` | `core.timeStampEt(): string \| null` | function |
| `Moss_TimeStampCT` | `core.timeStampCt(...)` | `core.timeStampCt(): string \| null` | function |
| `Moss_TimeStampMT` | `core.timeStampMt(...)` | `core.timeStampMt(): string \| null` | function |
| `Moss_TimeStampPT` | `core.timeStampPt(...)` | `core.timeStampPt(): string \| null` | function |
| `Moss_FormatTime` | `core.formatTime(...)` | `core.formatTime(fmt: string): string \| null` | function |

### `Moss_XR.h` → `xr`

| Native symbol (traceability) | Public API projection | Target-language signature | Kind |
|---|---|---|---|
| `Moss_XR_Initialize` | `xr.initialize(...)` | `xr.initialize(): boolean` | function |
| `Moss_XR_Shutdown` | `xr.shutdown(...)` | `xr.shutdown(): void` | function |
| `Moss_XR_GetCapabilities` | `xr.getCapabilities(...)` | `xr.getCapabilities(): XrCapabilities` | function |
| `Moss_XR_EnableValidationLayers` | `xr.enableValidationLayers(...)` | `xr.enableValidationLayers(): boolean` | function |
| `Moss_XR_CreateSession` | `xr.createSession(...)` | `xr.createSession(): XrSession` | function |
| `Moss_XR_DestroySession` | `XrSession.dispose(...)` | `XrSession.dispose(): void` | function |
| `Moss_XR_GetSessionState` | `XrSession.getState(...)` | `XrSession.getState(): XrSessionState` | function |
| `Moss_XR_BeginFrame` | `XrSession.beginFrame(...)` | `XrSession.beginFrame(): boolean` | function |
| `Moss_XR_EndFrame` | `XrSession.endFrame(...)` | `XrSession.endFrame(): void` | function |
| `Moss_XR_GetPredictedDisplayTime` | `XrSession.getPredictedDisplayTime(...)` | `XrSession.getPredictedDisplayTime(): XrTime` | function |
| `Moss_XR_GetDeltaSeconds` | `xr.getDeltaSeconds(...)` | `xr.getDeltaSeconds(): number` | function |
| `Moss_XR_HandleEvents` | `XrSession.handleEvents(...)` | `XrSession.handleEvents(): void` | function |
| `Moss_XR_GetViewCount` | `XrSession.getViewCount(...)` | `XrSession.getViewCount(): number` | function |
| `Moss_XR_GetView` | `XrSession.getView(...)` | `XrSession.getView(index: number, outView: XrView): boolean` | function |
| `Moss_XR_CreateActionSet` | `xr.createActionSet(...)` | `xr.createActionSet(name: string): XrActionSet` | function |
| `Moss_XR_DestroyActionSet` | `XrActionSet.dispose(...)` | `XrActionSet.dispose(): void` | function |
| `Moss_XR_CreateAction` | `XrActionSet.createAction(...)` | `XrActionSet.createAction(name: string, type: XrActionType): XrAction` | function |
| `Moss_XR_DestroyAction` | `XrAction.dispose(...)` | `XrAction.dispose(): void` | function |
| `Moss_XR_AttachActionSet` | `XrActionSet.attach(...)` | `XrActionSet.attach(session: XrSession): void` | function |
| `Moss_XR_SyncActions` | `XrSession.syncActions(...)` | `XrSession.syncActions(): void` | function |
| `Moss_XR_GetActionBoolean` | `XrAction.getBoolean(...)` | `XrAction.getBoolean(): boolean` | function |
| `Moss_XR_GetActionFloat` | `XrAction.getFloat(...)` | `XrAction.getFloat(): number` | function |
| `Moss_XR_GetActionPose` | `XrAction.getPose(...)` | `XrAction.getPose(outPose: XrPose): boolean` | function |
| `Moss_XR_StopHaptic` | `XrAction.stopHaptic(...)` | `XrAction.stopHaptic(): void` | function |
| `Moss_XR_CreateHand` | `xr.createHand(...)` | `xr.createHand(): XrHandModifier` | function |
| `Moss_XR_GetHandJointPose` | `XrHandModifier.getHandJointPose(...)` | `XrHandModifier.getHandJointPose(joint: XrHandJoint, outPose: XrPose): boolean` | function |
| `Moss_XR_DestroyHand` | `XrHandModifier.destroyHand(...)` | `XrHandModifier.destroyHand(): void` | function |
| `Moss_XR_GetHandPose` | `XrHandModifier.getHandPose(...)` | `XrHandModifier.getHandPose(outPose: XrPose): boolean` | function |
| `Moss_XR_CreateBody` | `xr.createBody(...)` | `xr.createBody(): XrBodyModifier` | function |
| `Moss_XR_GetBodyJointPose` | `XrBodyModifier.getBodyJointPose(...)` | `XrBodyModifier.getBodyJointPose(joint: XrBodyJoint, outPose: XrPose): boolean` | function |
| `Moss_XR_DestroyBody` | `XrBodyModifier.destroyBody(...)` | `XrBodyModifier.destroyBody(): void` | function |
| `Moss_XR_CreateFace` | `xr.createFace(...)` | `xr.createFace(): XrFaceModifier` | function |
| `Moss_XR_DestroyFace` | `XrFaceModifier.destroyFace(...)` | `XrFaceModifier.destroyFace(): void` | function |
| `Moss_XR_UpdateBodyTracking` | `XrSession.updateBodyTracking(...)` | `XrSession.updateBodyTracking(): void` | function |
| `Moss_XR_UpdateHandTracking` | `XrSession.updateHandTracking(...)` | `XrSession.updateHandTracking(): void` | function |
| `Moss_XR_EnablePassthrough` | `xr.enablePassthrough(...)` | `xr.enablePassthrough(enable: boolean): boolean` | function |
| `Moss_XR_EnableFoveatedRendering` | `xr.enableFoveatedRendering(...)` | `xr.enableFoveatedRendering(enable: boolean): boolean` | function |
| `Moss_XR_GetBackendName` | `xr.getBackendName(...)` | `xr.getBackendName(): string \| null` | function |
| `Moss_XR_GetBackendVersion` | `xr.getBackendVersion(...)` | `xr.getBackendVersion(): number` | function |
| `Moss_XR_BeginDebugLabel` | `xr.beginDebugLabel(...)` | `xr.beginDebugLabel(label: string): void` | function |
| `Moss_XR_EndDebugLabel` | `xr.endDebugLabel(...)` | `xr.endDebugLabel(): void` | function |


## 8. Native header and feature coverage

The public binding must cover the complete Moss header tree, not only the exported function index. Many Core, Geometry, Physics, Renderer, Resources, Skeleton, TriangleSplitter and Variants APIs are class members, templates, inline methods, or declarations not represented by the top-level free-function catalogue.

## Complete header inventory

Each link opens the corresponding source file in `include/Moss/`. The rightmost count shows declarations beginning with `MOSS_API` in that header; zero does not mean the header has no public API (it may expose class members, templates, inline methods, typedefs, or preprocessor declarations).

### Core (40 headers)

Foundational C++ utilities.

| Header | Export-marked declarations |
|---|---:|
| [`Core/BinaryHeap.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Core/BinaryHeap.h) | 0 |
| [`Core/ByteBuffer.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Core/ByteBuffer.h) | 0 |
| [`Core/FPControlWord.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Core/FPControlWord.h) | 0 |
| [`Core/FPException.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Core/FPException.h) | 0 |
| [`Core/FPFlushDenormals.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Core/FPFlushDenormals.h) | 0 |
| [`Core/Factory.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Core/Factory.h) | 0 |
| [`Core/FixedSizeFreeList.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Core/FixedSizeFreeList.h) | 0 |
| [`Core/FreeBlockAlloc.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Core/FreeBlockAlloc.h) | 0 |
| [`Core/HashCombine.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Core/HashCombine.h) | 0 |
| [`Core/HashTable.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Core/HashTable.h) | 0 |
| [`Core/InsertionSort.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Core/InsertionSort.h) | 0 |
| [`Core/JobSystem.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Core/JobSystem.h) | 0 |
| [`Core/JobSystemSingleThreaded.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Core/JobSystemSingleThreaded.h) | 0 |
| [`Core/JobSystemThreadPool.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Core/JobSystemThreadPool.h) | 0 |
| [`Core/JobSystemWithBarrier.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Core/JobSystemWithBarrier.h) | 0 |
| [`Core/LinearCurve.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Core/LinearCurve.h) | 0 |
| [`Core/LockFreeHashMap.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Core/LockFreeHashMap.h) | 0 |
| [`Core/Memory.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Core/Memory.h) | 11 |
| [`Core/Mutex.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Core/Mutex.h) | 0 |
| [`Core/MutexArray.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Core/MutexArray.h) | 0 |
| [`Core/NonCopyable.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Core/NonCopyable.h) | 0 |
| [`Core/Profiler.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Core/Profiler.h) | 4 |
| [`Core/QuickSort.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Core/QuickSort.h) | 0 |
| [`Core/RTTI.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Core/RTTI.h) | 0 |
| [`Core/Reference.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Core/Reference.h) | 0 |
| [`Core/Result.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Core/Result.h) | 0 |
| [`Core/STLAlignedAllocator.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Core/STLAlignedAllocator.h) | 0 |
| [`Core/STLAllocator.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Core/STLAllocator.h) | 0 |
| [`Core/STLLocalAllocator.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Core/STLLocalAllocator.h) | 0 |
| [`Core/STLTempAllocator.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Core/STLTempAllocator.h) | 0 |
| [`Core/ScopeExit.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Core/ScopeExit.h) | 0 |
| [`Core/Semaphore.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Core/Semaphore.h) | 0 |
| [`Core/StreamIn.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Core/StreamIn.h) | 0 |
| [`Core/StreamOut.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Core/StreamOut.h) | 0 |
| [`Core/StreamUtils.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Core/StreamUtils.h) | 0 |
| [`Core/StreamWrapper.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Core/StreamWrapper.h) | 0 |
| [`Core/StridedPtr.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Core/StridedPtr.h) | 0 |
| [`Core/StringTools.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Core/StringTools.h) | 6 |
| [`Core/TempAllocator.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Core/TempAllocator.h) | 0 |
| [`Core/TickCounter.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Core/TickCounter.h) | 0 |

### Geometry (9 headers)

Geometry algorithms and primitives.

| Header | Export-marked declarations |
|---|---:|
| [`Geometry/ClipPoly.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Geometry/ClipPoly.h) | 0 |
| [`Geometry/ClosestPoint.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Geometry/ClosestPoint.h) | 0 |
| [`Geometry/ConvexHullBuilder.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Geometry/ConvexHullBuilder.h) | 0 |
| [`Geometry/ConvexSupport.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Geometry/ConvexSupport.h) | 0 |
| [`Geometry/EPAConvexHullBuilder.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Geometry/EPAConvexHullBuilder.h) | 0 |
| [`Geometry/EPAPenetrationDepth.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Geometry/EPAPenetrationDepth.h) | 0 |
| [`Geometry/GJKClosestPoint.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Geometry/GJKClosestPoint.h) | 0 |
| [`Geometry/RayCylinder.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Geometry/RayCylinder.h) | 0 |
| [`Geometry/Triangle.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Geometry/Triangle.h) | 0 |

### Physics (69 headers)

Simulation, body, shape, collision, constraint and query implementation headers.

| Header | Export-marked declarations |
|---|---:|
| [`Physics/AABBTree/AABBTreeBuilder.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/AABBTree/AABBTreeBuilder.h) | 0 |
| [`Physics/AABBTree/AABBTreeToBuffer.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/AABBTree/AABBTreeToBuffer.h) | 0 |
| [`Physics/AABBTree/NodeCodec/NodeCodecQuadTreeHalfFloat.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/AABBTree/NodeCodec/NodeCodecQuadTreeHalfFloat.h) | 0 |
| [`Physics/AABBTree/NodeCodec/NodeCodecQuadTreeHalfFloat2D.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/AABBTree/NodeCodec/NodeCodecQuadTreeHalfFloat2D.h) | 0 |
| [`Physics/AABBTree/TriangleCodec/TriangleCodecIndexed8BitPackSOA4Flags.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/AABBTree/TriangleCodec/TriangleCodecIndexed8BitPackSOA4Flags.h) | 0 |
| [`Physics/AABBTree/TriangleCodec/TriangleCodecIndexed8BitPackSOA4Flags2D.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/AABBTree/TriangleCodec/TriangleCodecIndexed8BitPackSOA4Flags2D.h) | 0 |
| [`Physics/Body/Body.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Body/Body.h) | 1 |
| [`Physics/Body/BodyID.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Body/BodyID.h) | 0 |
| [`Physics/Character/Character.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Character/Character.h) | 0 |
| [`Physics/Cloth/Cloth.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Cloth/Cloth.h) | 0 |
| [`Physics/Collision/ActiveEdges.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Collision/ActiveEdges.h) | 0 |
| [`Physics/Collision/BroadPhase/BroadPhase.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Collision/BroadPhase/BroadPhase.h) | 0 |
| [`Physics/Collision/BroadPhase/BroadPhaseLayerInterfaceMask.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Collision/BroadPhase/BroadPhaseLayerInterfaceMask.h) | 0 |
| [`Physics/Collision/BroadPhase/QuadTree.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Collision/BroadPhase/QuadTree.h) | 0 |
| [`Physics/Collision/CastConvexVsTriangles.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Collision/CastConvexVsTriangles.h) | 0 |
| [`Physics/Collision/CastResult.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Collision/CastResult.h) | 0 |
| [`Physics/Collision/CastSphereVsTriangles.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Collision/CastSphereVsTriangles.h) | 0 |
| [`Physics/Collision/CollideConvexVsTriangles.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Collision/CollideConvexVsTriangles.h) | 0 |
| [`Physics/Collision/CollidePointResult.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Collision/CollidePointResult.h) | 0 |
| [`Physics/Collision/CollideShape.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Collision/CollideShape.h) | 0 |
| [`Physics/Collision/CollideShapeVsShapePerLeaf.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Collision/CollideShapeVsShapePerLeaf.h) | 0 |
| [`Physics/Collision/CollideSoftBodyVertexIterator.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Collision/CollideSoftBodyVertexIterator.h) | 0 |
| [`Physics/Collision/CollideSoftBodyVerticesVsTriangles.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Collision/CollideSoftBodyVerticesVsTriangles.h) | 0 |
| [`Physics/Collision/CollideSphereVsTriangles.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Collision/CollideSphereVsTriangles.h) | 0 |
| [`Physics/Collision/CollisionCollector.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Collision/CollisionCollector.h) | 0 |
| [`Physics/Collision/CollisionCollectorImpl.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Collision/CollisionCollectorImpl.h) | 0 |
| [`Physics/Collision/CollisionDispatch.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Collision/CollisionDispatch.h) | 0 |
| [`Physics/Collision/CollisionGroup.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Collision/CollisionGroup.h) | 0 |
| [`Physics/Collision/ContactListener.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Collision/ContactListener.h) | 0 |
| [`Physics/Collision/EstimateCollisionResponse.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Collision/EstimateCollisionResponse.h) | 1 |
| [`Physics/Collision/GroupFilter.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Collision/GroupFilter.h) | 0 |
| [`Physics/Collision/GroupFilterTable.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Collision/GroupFilterTable.h) | 0 |
| [`Physics/Collision/InternalEdgeRemovingCollector.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Collision/InternalEdgeRemovingCollector.h) | 0 |
| [`Physics/Collision/ManifoldBetweenTwoFaces.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Collision/ManifoldBetweenTwoFaces.h) | 2 |
| [`Physics/Collision/NarrowPhaseQuery.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Collision/NarrowPhaseQuery.h) | 0 |
| [`Physics/Collision/NarrowPhaseStats.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Collision/NarrowPhaseStats.h) | 0 |
| [`Physics/Collision/ObjectLayer.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Collision/ObjectLayer.h) | 0 |
| [`Physics/Collision/ObjectLayerPairFilterMask.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Collision/ObjectLayerPairFilterMask.h) | 0 |
| [`Physics/Collision/ObjectLayerPairFilterTable.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Collision/ObjectLayerPairFilterTable.h) | 0 |
| [`Physics/Collision/PhysicsMaterial.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Collision/PhysicsMaterial.h) | 0 |
| [`Physics/Collision/PhysicsMaterialSimple.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Collision/PhysicsMaterialSimple.h) | 0 |
| [`Physics/Collision/RayCast.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Collision/RayCast.h) | 0 |
| [`Physics/Collision/Shape/CompoundShapeVisitors.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Collision/Shape/CompoundShapeVisitors.h) | 0 |
| [`Physics/Collision/Shape/ConvexHullShape.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Collision/Shape/ConvexHullShape.h) | 0 |
| [`Physics/Collision/Shape/GetTrianglesContext.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Collision/Shape/GetTrianglesContext.h) | 0 |
| [`Physics/Collision/Shape/PolyhedronSubmergedVolumeCalculator.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Collision/Shape/PolyhedronSubmergedVolumeCalculator.h) | 0 |
| [`Physics/Collision/Shape/Shapes.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Collision/Shape/Shapes.h) | 0 |
| [`Physics/Collision/Shape/SubShapeID.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Collision/Shape/SubShapeID.h) | 0 |
| [`Physics/Collision/Shape/SubShapeIDPair.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Collision/Shape/SubShapeIDPair.h) | 0 |
| [`Physics/Collision/ShapeCast.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Collision/ShapeCast.h) | 0 |
| [`Physics/Collision/ShapeFilter.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Collision/ShapeFilter.h) | 0 |
| [`Physics/Collision/SimShapeFilter.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Collision/SimShapeFilter.h) | 0 |
| [`Physics/Collision/SimShapeFilterWrapper.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Collision/SimShapeFilterWrapper.h) | 0 |
| [`Physics/Collision/SortReverseAndStore.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Collision/SortReverseAndStore.h) | 0 |
| [`Physics/Collision/TransformedShape.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Collision/TransformedShape.h) | 0 |
| [`Physics/Constraints/ConstraintManager.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Constraints/ConstraintManager.h) | 0 |
| [`Physics/Constraints/Constraints.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Constraints/Constraints.h) | 0 |
| [`Physics/Hair/Hair.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Hair/Hair.h) | 0 |
| [`Physics/IslandBuilder.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/IslandBuilder.h) | 0 |
| [`Physics/LargeIslandSplitter.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/LargeIslandSplitter.h) | 0 |
| [`Physics/PhysicsLock.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/PhysicsLock.h) | 0 |
| [`Physics/PhysicsScene.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/PhysicsScene.h) | 0 |
| [`Physics/PhysicsSystem.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/PhysicsSystem.h) | 0 |
| [`Physics/PhysicsUpdateContext.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/PhysicsUpdateContext.h) | 0 |
| [`Physics/Ragdoll/Ragdoll.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Ragdoll/Ragdoll.h) | 0 |
| [`Physics/SoftBody/Softbody.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/SoftBody/Softbody.h) | 0 |
| [`Physics/StateRecorder.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/StateRecorder.h) | 0 |
| [`Physics/StateRecorderImpl.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/StateRecorderImpl.h) | 0 |
| [`Physics/Vehicle/Vehicle.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Vehicle/Vehicle.h) | 0 |

### Renderer (65 headers)

Renderer resources and graphics backend headers.

| Header | Export-marked declarations |
|---|---:|
| [`Renderer/DX12/CommandQueueDX12.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/DX12/CommandQueueDX12.h) | 0 |
| [`Renderer/DX12/ConstantBufferDX12.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/DX12/ConstantBufferDX12.h) | 0 |
| [`Renderer/DX12/DescriptorHeapDX12.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/DX12/DescriptorHeapDX12.h) | 0 |
| [`Renderer/DX12/LightingDX12.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/DX12/LightingDX12.h) | 0 |
| [`Renderer/DX12/PipelineStateDX12.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/DX12/PipelineStateDX12.h) | 0 |
| [`Renderer/DX12/RenderInstancesDX12.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/DX12/RenderInstancesDX12.h) | 0 |
| [`Renderer/DX12/RenderPrimitiveDX12.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/DX12/RenderPrimitiveDX12.h) | 0 |
| [`Renderer/DX12/RendererDX12.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/DX12/RendererDX12.h) | 0 |
| [`Renderer/DX12/ShaderDX12.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/DX12/ShaderDX12.h) | 0 |
| [`Renderer/DX12/TextureDX12.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/DX12/TextureDX12.h) | 0 |
| [`Renderer/DebugRenderer.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/DebugRenderer.h) | 0 |
| [`Renderer/DebugRendererPlayback.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/DebugRendererPlayback.h) | 0 |
| [`Renderer/DebugRendererRecorder.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/DebugRendererRecorder.h) | 0 |
| [`Renderer/DebugRendererSimple.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/DebugRendererSimple.h) | 0 |
| [`Renderer/FogVolume.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/FogVolume.h) | 0 |
| [`Renderer/Font.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/Font.h) | 0 |
| [`Renderer/GL/FontGL.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/GL/FontGL.h) | 0 |
| [`Renderer/GL/LightingGL.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/GL/LightingGL.h) | 0 |
| [`Renderer/GL/MeshGL.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/GL/MeshGL.h) | 0 |
| [`Renderer/GL/PipelineStateGL.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/GL/PipelineStateGL.h) | 0 |
| [`Renderer/GL/PostProcess.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/GL/PostProcess.h) | 0 |
| [`Renderer/GL/ReflectionProbe.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/GL/ReflectionProbe.h) | 0 |
| [`Renderer/GL/Renderer_GL.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/GL/Renderer_GL.h) | 0 |
| [`Renderer/GL/ShaderGL.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/GL/ShaderGL.h) | 0 |
| [`Renderer/GL/StorageBufferGL.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/GL/StorageBufferGL.h) | 0 |
| [`Renderer/GL/SubViewportGL.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/GL/SubViewportGL.h) | 0 |
| [`Renderer/GL/SurfaceGL.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/GL/SurfaceGL.h) | 0 |
| [`Renderer/GL/TextureGL.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/GL/TextureGL.h) | 0 |
| [`Renderer/GL/glad.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/GL/glad.h) | 0 |
| [`Renderer/MTL/PipelineStateMTL.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/MTL/PipelineStateMTL.h) | 0 |
| [`Renderer/MTL/RenderInstancesMTL.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/MTL/RenderInstancesMTL.h) | 0 |
| [`Renderer/MTL/RenderPrimitiveMTL.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/MTL/RenderPrimitiveMTL.h) | 0 |
| [`Renderer/MTL/RendererMTL.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/MTL/RendererMTL.h) | 0 |
| [`Renderer/MTL/ShaderMTL.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/MTL/ShaderMTL.h) | 0 |
| [`Renderer/MTL/TextureMTL.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/MTL/TextureMTL.h) | 0 |
| [`Renderer/Managers/DecalManager.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/Managers/DecalManager.h) | 0 |
| [`Renderer/Managers/FontManager.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/Managers/FontManager.h) | 0 |
| [`Renderer/Managers/LightingManager.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/Managers/LightingManager.h) | 0 |
| [`Renderer/Managers/MeshManager3D.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/Managers/MeshManager3D.h) | 0 |
| [`Renderer/Managers/ParticleEffectManager.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/Managers/ParticleEffectManager.h) | 0 |
| [`Renderer/Managers/SurfaceManager.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/Managers/SurfaceManager.h) | 0 |
| [`Renderer/Material2.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/Material2.h) | 0 |
| [`Renderer/Material3.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/Material3.h) | 0 |
| [`Renderer/Mesh.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/Mesh.h) | 0 |
| [`Renderer/Model.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/Model.h) | 0 |
| [`Renderer/PipelineState.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/PipelineState.h) | 0 |
| [`Renderer/Renderer.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/Renderer.h) | 0 |
| [`Renderer/Shader.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/Shader.h) | 0 |
| [`Renderer/SkyBox.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/SkyBox.h) | 0 |
| [`Renderer/StorageBuffer.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/StorageBuffer.h) | 0 |
| [`Renderer/SubViewport.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/SubViewport.h) | 0 |
| [`Renderer/Surface.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/Surface.h) | 0 |
| [`Renderer/Texture.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/Texture.h) | 0 |
| [`Renderer/VK/ConstantBufferVK.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/VK/ConstantBufferVK.h) | 0 |
| [`Renderer/VK/LightingManagerVK.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/VK/LightingManagerVK.h) | 0 |
| [`Renderer/VK/MeshVK.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/VK/MeshVK.h) | 0 |
| [`Renderer/VK/PipelineStateVK.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/VK/PipelineStateVK.h) | 0 |
| [`Renderer/VK/RenderInstancesVK.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/VK/RenderInstancesVK.h) | 0 |
| [`Renderer/VK/RenderPrimitiveVK.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/VK/RenderPrimitiveVK.h) | 0 |
| [`Renderer/VK/RendererVK.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/VK/RendererVK.h) | 0 |
| [`Renderer/VK/Renderer_VK.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/VK/Renderer_VK.h) | 4 |
| [`Renderer/VK/ShaderVK.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/VK/ShaderVK.h) | 0 |
| [`Renderer/VK/StorageBufferVK.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/VK/StorageBufferVK.h) | 0 |
| [`Renderer/VK/SubViewportVK.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/VK/SubViewportVK.h) | 0 |
| [`Renderer/VK/TextureVK.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Renderer/VK/TextureVK.h) | 0 |

### Resources (21 headers)

File/resource and serialization headers.

| Header | Export-marked declarations |
|---|---:|
| [`Resources/BezierCurve.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Resources/BezierCurve.h) | 0 |
| [`Resources/Config.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Resources/Config.h) | 0 |
| [`Resources/Gradient.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Resources/Gradient.h) | 0 |
| [`Resources/Json.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Resources/Json.h) | 0 |
| [`Resources/ObjectStream/GetPrimitiveTypeOfType.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Resources/ObjectStream/GetPrimitiveTypeOfType.h) | 0 |
| [`Resources/ObjectStream/ObjectStream.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Resources/ObjectStream/ObjectStream.h) | 4 |
| [`Resources/ObjectStream/ObjectStreamBinaryIn.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Resources/ObjectStream/ObjectStreamBinaryIn.h) | 0 |
| [`Resources/ObjectStream/ObjectStreamBinaryOut.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Resources/ObjectStream/ObjectStreamBinaryOut.h) | 0 |
| [`Resources/ObjectStream/ObjectStreamIn.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Resources/ObjectStream/ObjectStreamIn.h) | 0 |
| [`Resources/ObjectStream/ObjectStreamOut.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Resources/ObjectStream/ObjectStreamOut.h) | 0 |
| [`Resources/ObjectStream/ObjectStreamTextIn.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Resources/ObjectStream/ObjectStreamTextIn.h) | 0 |
| [`Resources/ObjectStream/ObjectStreamTextOut.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Resources/ObjectStream/ObjectStreamTextOut.h) | 0 |
| [`Resources/ObjectStream/ObjectStreamTypes.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Resources/ObjectStream/ObjectStreamTypes.h) | 0 |
| [`Resources/ObjectStream/SerializableAttribute.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Resources/ObjectStream/SerializableAttribute.h) | 0 |
| [`Resources/ObjectStream/SerializableAttributeEnum.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Resources/ObjectStream/SerializableAttributeEnum.h) | 0 |
| [`Resources/ObjectStream/SerializableAttributeTyped.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Resources/ObjectStream/SerializableAttributeTyped.h) | 0 |
| [`Resources/ObjectStream/SerializableObject.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Resources/ObjectStream/SerializableObject.h) | 0 |
| [`Resources/ObjectStream/TypeDeclarations.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Resources/ObjectStream/TypeDeclarations.h) | 0 |
| [`Resources/PCK.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Resources/PCK.h) | 0 |
| [`Resources/Timer.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Resources/Timer.h) | 0 |
| [`Resources/Tween.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Resources/Tween.h) | 0 |

### Skeleton (1 headers)

Skeleton and animation declarations.

| Header | Export-marked declarations |
|---|---:|
| [`Skeleton/Skeleton.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Skeleton/Skeleton.h) | 0 |

### TriangleSplitter (1 headers)

Triangle splitting utilities.

| Header | Export-marked declarations |
|---|---:|
| [`TriangleSplitter/TriangleSplitter.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/TriangleSplitter/TriangleSplitter.h) | 0 |

### Variants (53 headers)

Math, geometry and container value types.

| Header | Export-marked declarations |
|---|---:|
| [`Variants/AABB2.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/AABB2.h) | 0 |
| [`Variants/AABB3.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/AABB3.h) | 0 |
| [`Variants/Basis.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Basis.h) | 0 |
| [`Variants/Color.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Color.h) | 0 |
| [`Variants/Math/EigenValueSymmetric.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Math/EigenValueSymmetric.h) | 0 |
| [`Variants/Math/GaussianElimination.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Math/GaussianElimination.h) | 0 |
| [`Variants/Math/HalfFloat.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Math/HalfFloat.h) | 0 |
| [`Variants/Math/MathTypes.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Math/MathTypes.h) | 0 |
| [`Variants/Math/Real.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Math/Real.h) | 0 |
| [`Variants/Math/Trigonometry.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Math/Trigonometry.h) | 0 |
| [`Variants/Matrix/DMat44.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Matrix/DMat44.h) | 0 |
| [`Variants/Matrix/DMat4x3.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Matrix/DMat4x3.h) | 0 |
| [`Variants/Matrix/DynMatrix.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Matrix/DynMatrix.h) | 0 |
| [`Variants/Matrix/Mat22.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Matrix/Mat22.h) | 0 |
| [`Variants/Matrix/Mat2x3.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Matrix/Mat2x3.h) | 0 |
| [`Variants/Matrix/Mat2x4.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Matrix/Mat2x4.h) | 0 |
| [`Variants/Matrix/Mat33.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Matrix/Mat33.h) | 0 |
| [`Variants/Matrix/Mat3x2.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Matrix/Mat3x2.h) | 0 |
| [`Variants/Matrix/Mat3x4.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Matrix/Mat3x4.h) | 0 |
| [`Variants/Matrix/Mat44.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Matrix/Mat44.h) | 0 |
| [`Variants/Matrix/Mat4x2.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Matrix/Mat4x2.h) | 0 |
| [`Variants/Matrix/Mat4x3.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Matrix/Mat4x3.h) | 0 |
| [`Variants/Matrix/TMatrix.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Matrix/TMatrix.h) | 0 |
| [`Variants/OBB2.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/OBB2.h) | 0 |
| [`Variants/OBB3.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/OBB3.h) | 0 |
| [`Variants/Plane.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Plane.h) | 0 |
| [`Variants/Quat.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Quat.h) | 0 |
| [`Variants/Rect.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Rect.h) | 0 |
| [`Variants/TArray.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/TArray.h) | 0 |
| [`Variants/TMap.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/TMap.h) | 0 |
| [`Variants/TSet.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/TSet.h) | 0 |
| [`Variants/TStaticArray.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/TStaticArray.h) | 0 |
| [`Variants/Vector/BVec16.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Vector/BVec16.h) | 0 |
| [`Variants/Vector/DVec2.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Vector/DVec2.h) | 0 |
| [`Variants/Vector/DVec3.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Vector/DVec3.h) | 0 |
| [`Variants/Vector/DVec4.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Vector/DVec4.h) | 0 |
| [`Variants/Vector/Double2.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Vector/Double2.h) | 0 |
| [`Variants/Vector/Double3.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Vector/Double3.h) | 0 |
| [`Variants/Vector/Double4.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Vector/Double4.h) | 0 |
| [`Variants/Vector/Float2.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Vector/Float2.h) | 0 |
| [`Variants/Vector/Float3.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Vector/Float3.h) | 0 |
| [`Variants/Vector/Float4.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Vector/Float4.h) | 0 |
| [`Variants/Vector/Int2.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Vector/Int2.h) | 0 |
| [`Variants/Vector/Int3.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Vector/Int3.h) | 0 |
| [`Variants/Vector/Int4.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Vector/Int4.h) | 0 |
| [`Variants/Vector/TVec.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Vector/TVec.h) | 0 |
| [`Variants/Vector/UVec4.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Vector/UVec4.h) | 0 |
| [`Variants/Vector/Vec2.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Vector/Vec2.h) | 1 |
| [`Variants/Vector/Vec3.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Vector/Vec3.h) | 1 |
| [`Variants/Vector/Vec4.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Vector/Vec4.h) | 0 |
| [`Variants/Vector/iVec2.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Vector/iVec2.h) | 1 |
| [`Variants/Vector/iVec3.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Vector/iVec3.h) | 1 |
| [`Variants/Vector/iVec4.h`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Vector/iVec4.h) | 1 |

### Top-level subsystem headers (11 headers)

| Header | Export-marked declarations |
|---|---:|
| [`Moss.h](https://github.com/TxbiG/Moss/blob/main/include/Moss/Moss.h)` | 0 |
| [`Moss_Audio.h](https://github.com/TxbiG/Moss/blob/main/include/Moss/Moss_Audio.h)` | 113 |
| [`Moss_GPU.h](https://github.com/TxbiG/Moss/blob/main/include/Moss/Moss_GPU.h)` | 241 |
| [`Moss_GUI.h](https://github.com/TxbiG/Moss/blob/main/include/Moss/Moss_GUI.h)` | 73 |
| [`Moss_Navigation.h](https://github.com/TxbiG/Moss/blob/main/include/Moss/Moss_Navigation.h)` | 0 |
| [`Moss_Network.h](https://github.com/TxbiG/Moss/blob/main/include/Moss/Moss_Network.h)` | 146 |
| [`Moss_Physics.h](https://github.com/TxbiG/Moss/blob/main/include/Moss/Moss_Physics.h)` | 39 |
| [`Moss_Platform.h](https://github.com/TxbiG/Moss/blob/main/include/Moss/Moss_Platform.h)` | 181 |
| [`Moss_Renderer.h](https://github.com/TxbiG/Moss/blob/main/include/Moss/Moss_Renderer.h)` | 195 |
| [`Moss_XR.h](https://github.com/TxbiG/Moss/blob/main/include/Moss/Moss_XR.h)` | 41 |
| [`Moss_stdinc.h](https://github.com/TxbiG/Moss/blob/main/include/Moss/Moss_stdinc.h)` | 34 |

## Inline implementation header inventory (19 files)

These files provide inline/template definitions or companion implementation details. They are included here because the public API surface cannot be understood from declarations alone when these files contain required definitions.

| Inline header |
|---|
| [`Core/FixedSizeFreeList.inl`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Core/FixedSizeFreeList.inl) |
| [`Core/JobSystem.inl`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Core/JobSystem.inl) |
| [`Core/LockFreeHashMap.inl`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Core/LockFreeHashMap.inl) |
| [`Core/Profiler.inl`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Core/Profiler.inl) |
| [`Physics/Body/Body.inl`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Physics/Body/Body.inl) |
| [`Variants/Matrix/DMat44.inl`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Matrix/DMat44.inl) |
| [`Variants/Matrix/Mat44.inl`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Matrix/Mat44.inl) |
| [`Variants/Quat.inl`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Quat.inl) |
| [`Variants/Vector/BVec16.inl`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Vector/BVec16.inl) |
| [`Variants/Vector/DVec2.inl`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Vector/DVec2.inl) |
| [`Variants/Vector/DVec3.inl`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Vector/DVec3.inl) |
| [`Variants/Vector/DVec4.inl`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Vector/DVec4.inl) |
| [`Variants/Vector/UVec4.inl`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Vector/UVec4.inl) |
| [`Variants/Vector/Vec2.inl`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Vector/Vec2.inl) |
| [`Variants/Vector/Vec3.inl`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Vector/Vec3.inl) |
| [`Variants/Vector/Vec4.inl`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Vector/Vec4.inl) |
| [`Variants/Vector/iVec2.inl`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Vector/iVec2.inl) |
| [`Variants/Vector/iVec3.inl`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Vector/iVec3.inl) |
| [`Variants/Vector/iVec4.inl`](https://github.com/TxbiG/Moss/blob/main/include/Moss/Variants/Vector/iVec4.inl) |

## 9. Native integration details to preserve

This is a language-level API specification, not a mechanical ABI mirror. When implementing the projection, keep these source-level distinctions intact:

- Convert pointer-plus-count inputs into a single slice/span/list/array/table/sequence; do not expose a redundant count parameter when the collection already carries its length.
- Convert output pointer pairs (for example, mouse coordinates) into a point/tuple/value object. Do not expose C-style `int*` output arguments.
- Resource factories return owned language objects and translate native allocation/backend failure into the language's normal error model. Destroy/terminate functions become deterministic resource disposal (`Dispose`, `Drop`, `close`, or context-manager release), not ordinary public `destroy_*` calls.
- GUI calls should be scoped to a `GuiContext` instance in every managed binding. They must not silently depend on mutable global “current context” state when the public API can retain an explicit context.
- Vulkan handles, callback pointers, allocator callbacks and backend-specific opaque handles stay behind advanced interop APIs unless the user explicitly opts into native interop. They must not be confused with Moss-owned managed resources.
- Where the native declaration lacks information required by the intended safe API (for example a pointer/count pair, output values, or a resource owner), resolve the native declaration or binding-side contract before implementing that projection; do not guess buffer length or object lifetime.

## 10. Build and distribution contract

- Ship matching native binaries and binding artifacts for each supported OS/architecture combination.
- Keep ABI declarations internal; public API users should not need to know calling conventions, struct packing, pointer widths or native library search paths.
- Make backend and optional dependency feature flags visible through capability queries or build metadata.
- Test a clean build, import/load, initialization/shutdown, object lifetime, callback detach, error translation and resource ownership in CI for every supported platform.
- Keep the generated language API index in sync with the native CMoss export catalogue whenever public native declarations change.

## 11. Compatibility and versioning

Treat the binding's public API as a separately versioned language library. A native API addition does not become a public binding API until the type mapping, ownership, failure semantics and example are specified. Avoid leaking raw `Moss_*` function names into application code. Deprecate public methods using the target language's normal annotation mechanism and provide a migration note in the API Reference itself when a breaking change is unavoidable.

---

**Canonical authority:** Moss C++/CMoss headers and the CMoss API Reference supplied for this documentation set. The native symbol column preserves traceability; the public API column is the intended language-specific design.
