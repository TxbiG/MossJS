# Lesson 9 — Polish, Testing and Performance

## Goal

Turn the prototype into a stable small game. This lesson focuses on predictable behaviour, testability, frame pacing, resource ownership and a cautious introduction to Moss audio.

## 1. Test the game systems without Moss

Most game logic should not require a window. Write tests for:

- A player landing on a floor and hitting the underside of a platform.
- A player resolving against the left and right side of a tile.
- A collectible being awarded once, but never twice.
- Buff expiry restoring base statistics.
- Death/restart returning the level to a clean state.
- A wave/level completion transition occurring once.

Use small deterministic level fixtures. Collision bugs are easier to diagnose in a six-tile test than in a finished level.

## 2. Integrate Moss audio carefully

The current audio header exposes the following lifecycle:

```cpp
const int audio_result = Moss_Init_Audio();
const bool audio_ready = (audio_result == 0);

// Inside the application update loop, when audio_ready is true:
Moss_AudioUpdate(delta_seconds);

// During shutdown, after audio objects have been released:
Moss_Terminate_Audio();
```

It also declares `Moss_AudioLoadWavFile`, `Moss_AudioSourceDestroy` and stream play/stop calls. However, the audited public header does **not** declare a simple source-play function. Loading a WAV source does not itself start playback. Do not invent `Moss_AudioPlaySource` or assume that a stream can play a loaded source without following the actual current audio implementation. When adding sound effects, verify the current stream/source binding and backend support from `Moss_Audio.h` and its implementation first.

The header describes Windows audio backends as XAudio2/WASAPI and marks macOS CoreAudio and Linux ALSA as under testing. Treat platform status as something to re-check at the selected commit.

## 3. Keep frame-time and simulation-time separate

Use the fixed update from Lesson 4 for gameplay. Measure frame time with `std::chrono::steady_clock`, clamp very large deltas and profile expensive update/render sections. Avoid making movement, buff duration or spawn timers depend on render frame count.

## 4. Optimise only measured problems

Start with inexpensive wins:

- Draw only nearby or visible map cells in large levels.
- Reuse vectors and entity storage where allocation churn is measurable.
- Avoid loading assets from disk during gameplay.
- Batch repeated debug or sprite draws when the real asset pipeline is in place.
- Reduce expensive collision candidates with a tile grid or spatial partition.

The renderer exposes sprite batch and debug draw-list entry points. Use them only after confirming the resource types and asset path for your target revision; don't reinterpret a debug box as a production sprite renderer.

## 5. Final play-test checklist

- Can the game be closed cleanly from every game state?
- Are all resources destroyed before their owning renderer/device is destroyed?
- Can the level restart repeatedly without duplicated pickups or stale timers?
- Does the player behave similarly at 30, 60 and 144 Hz render rates?
- Are missing assets reported clearly?
- Is the selected graphics/audio backend documented in the build notes?

## Checkpoint

Create a release candidate build, run through the full level three times, and record any reproducible issue with steps, expected result and actual result.
