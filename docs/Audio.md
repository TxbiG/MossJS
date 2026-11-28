# Audio
## Overview
Moss features a flexible and extensible Audio System designed for immersive soundscapes and high-performance 2D and 3D audio rendering. It provides a unified API across platforms and integrates with Moss scene and physics systems.

Supported Audio - ```XAudio2 / WASAPI```, ```CoreAudio```, ```ALSA```.

Supports ```Mono```, ```Stereo```, ```Quadraphonic```, ```4.1 Surround```, ```5.1 Surround```, ```6.1 Surround```, ```7.1 Surround```, ```Spatial```

- [Overview](#overview)
  - [Channels](#channels), [Effects](#effects)
  - [AudioStream](#audiostream), [AudioStream2](#audiostream2), [AudioStream3](#audiostream3)
  - [AudioListener2](#audiolistener2), [AudioListener3](#audiolistener3)
  - [Speaker](#speaker), [Microphone](#microphone)
  - [RayTrace Audio](#raytraceaudio)

## Enums
```cpp
enum class AudioEffectType {
    LOWPASS,
    HIGHPASS,
    ECHO,
    FLANGE,
    DISTORTION,
    NORMALIZE,
    PARAMEQ,
    PITCHSHIFTER,
    CHORUS,
    COMPRESSOR,
    REVERB,
    DELAY,
    DOPPLER,
    PANNING,
    DISTANCE_ATTENUATION
};

enum class DistanceModel {
    LINEAR,
    INVERSE,
    EXPONENTIAL
};
```
## Structs
```cpp
// Provided by Moss
struct Microphone;
struct Speakers;
struct AudioStream2D;
struct AudioStream3D;
struct AudioListener2D;
struct AudioListener3D;
struct RayCastListenerStream2D;
struct RayCastListenerStream3D;
```

## Functions


### Initialization
```cpp
int Moss_Init_Audio();
void Moss_Terminate_Audio();
void Moss_AudioUpdate(float deltaTime);
```


### Loading Audio
```cpp
Moss_AudioSource* Moss_AudioLoadWavFile(const char* filename);
Moss_AudioSource* Moss_AudioLoadOgg(const char* filename, AudioLoadType type);
Moss_AudioSource* Moss_AudioLoadMP3(const char* filename);
void Moss_AudioSourceDestroy(Moss_AudioSource* source);
```

WAV loading is implemented through the shared PCM source path. OGG and MP3 entry points first try decoders registered with `Moss_AudioRegisterDecoder`; without a registered decoder they fall back to WAV-compatible data for older smoke tests.

### Audio Effects
```cpp
```

### Channels
```cpp
ChannelID Moss_AudioCreateChannel(ChannelID channel);
void Audio_RemoveChannel(ChannelID channel);
ChannelID Moss_AudioGetMasterChannel();
void Moss_AudioSetChannelVolume(ChannelID channel, float volume);
void Moss_AudioSetChannelMute(ChannelID channel, bool mute);
void Moss_AudioAddChannelEffect(ChannelID channel, AudioEffect* effect);
void Moss_AudioRemoveChannelEffect(ChannelID channel, AudioEffect* effect);
void Moss_AudioRemoveAllChannelEffects(ChannelID channel);
```

### AudioStreams
```cpp
AudioStream* Moss_AudioStreamCreate();
void Moss_AudioStreamPlay(AudioStream* stream);
void Moss_AudioStreamStop(AudioStream* stream);
void Moss_AudioStreamSetVolume(AudioStream* stream, float volume);
void Moss_AudioStreamSetPitch(AudioStream* stream, float pitch);
void Moss_AudioStreamSetLoop(AudioStream* stream, bool loop);
void Moss_AudioStreamRemove(AudioStream* stream);

AudioStream2D* Moss_AudioStream2DCreate();
void Moss_AudioStream2DPlay(AudioStream2D* stream);
void Moss_AudioStream2DStop(AudioStream2D* stream);
void Moss_AudioStream2DSetVolume(AudioStream2D* stream, float volume);
void Moss_AudioStream2DSetPitch(AudioStream2D* stream, float pitch);
void Moss_AudioStream2DSetLoop(AudioStream2D* stream, bool loop);
void Moss_AudioStream2DRemove(AudioStream2D* stream);

AudioStream3D* Moss_AudioStream3DCreate();
void Moss_AudioStream3DPlay(AudioStream3D* stream);
void Moss_AudioStream3DStop(AudioStream3D* stream);
void Moss_AudioStream3DSetVolume(AudioStream3D* stream, float volume);
void Moss_AudioStream3DSetPitch(AudioStream3D* stream, float pitch);
void Moss_AudioStream3DSetLoop(AudioStream3D* stream, bool loop);
void Moss_AudioStream3DSetPosition(AudioStream3D* stream);
void Moss_AudioStream3DSetVelocity(AudioStream3D* stream);
void Moss_AudioStream3DSetMaxDistance(AudioStream3D* stream);
void Moss_AudioStream3DSetDistanceModel(AudioStream3D* stream, DistanceModel model);
void Moss_AudioStream3DRemove(AudioStream3D* stream);
```


### Listeners
```cpp
AudioListener2D* Moss_AudioCreateAudioListener2D();
AudioListener3D* Moss_AudioCreateAudioListener3D();
RayAudioListener2D* Moss_AudioCreateRayAudioListener2D();
RayAudioListener3D* Moss_AudioCreateRayAudioListener3D();
```


```cpp
void Moss_AudioActivateAudioListener2D(AudioListener2D* listener, bool active);
void Moss_AudioActivateAudioListener3D(AudioListener3D* listener, bool active);
void Moss_AudioActivateRayAudioListener2D(RayAudioListener2D* listener, bool active);
void Moss_AudioActivateRayAudioListener3D(RayAudioListener3D* listener, bool active);


void Moss_AudioListenerSetOrientation(
    AudioListener3D* listener,
    const Vec3& forward,
    const Vec3& up
);
```


## Speaker Device
```cpp
bool Moss_IsSpeakerDeviceReady();
bool Moss_AudioSpeakerOpen();
void Moss_AudioSpeakerPause();
void Moss_AudioSpeakerResume();
bool Moss_AudioSpeakerIsPaused();
bool Moss_AudioSelectSpeakerDevice(int id);
int Moss_GetCurrentSpeakerDeviceID();
const char* Moss_GetSpeakerDeviceName(int id);
int Moss_ListSpeakerDevices();
```

## Microphone Device
```cpp
bool Moss_IsMicrophoneDeviceReady();
int Moss_AudioMicrophoneOpen();
void Moss_AudioMicrophoneClose();
void Moss_AudioMicrophonePlay();
void Moss_AudioMicrophoneStop();
int Moss_AudioMicrophoneID();
bool Moss_AudioSelectMicrophoneDevice(int id);
const char* Moss_GetMicrophoneDeviceName(int index);
int Moss_ListMicrophoneDevices();

void Moss_AudioMicrophoneSetGain(Microphone* mic, float gain);
int Moss_AudioMicrophoneGetSampleRate(Microphone* mic);
int Moss_AudioMicrophoneGetChannels(Microphone* mic);
```

### Callbacks
```cpp
typedef void (*AudioStreamCallback)(float* buffer, int frames, void* userData);

void Moss_AudioStreamSetCallback(AudioStream* stream, AudioStreamCallback callback, void* userData);

typedef void (*MicrophoneCallback)(const float* buffer, int frames, void* userData);

void Moss_AudioMicrophoneSetCallback(Microphone* mic, MicrophoneCallback callback, void* userData);
```

## Examples
```cpp

...

int main() {

  if (!Moss_Init_Audio()) { return 0; }

  bool running = true;
    while (running) {
        float deltaTime = 1.0f / 60.0f;

        ...

        // Update audio once per frame
        Moss_AudioUpdate(deltaTime);

        ...

        // Application logic...
    }

  Moss_Terminate_Audio();
  return 0;

}
```

## Channels
```cpp

...

// Get master channel
ChannelID master = Moss_AudioGetMasterChannel();

// Create channels
ChannelID dialogue = Moss_AudioCreateChannel(1);
ChannelID sfx      = Moss_AudioCreateChannel(2);

// Adjust channel properties
Moss_AudioSetChannelVolume(dialogue, 0.8f);
Moss_AudioSetChannelVolume(sfx, 1.0f);

// Mute / unmute
Moss_AudioSetChannelMute(dialogue, false);

// Remove channel when no longer needed
Audio_RemoveChannel(sfx);

...

```
## Effects
```cpp
// Create a reverb effect
AudioEffect reverb = Moss_AudioCreateEffect(AudioEffectType::REVERB);

// Configure effect parameters
Moss_AudioCreateEffect(&reverb, "decay", 1.2f);
Moss_AudioCreateEffect(&reverb, "mix", 0.5f);

// Attach effect to channel
ChannelID master = Moss_AudioGetMasterChannel();
Moss_AudioAddChannelEffect(master, &reverb);

// Remove effect
Moss_AudioRemoveChannelEffect(master, &reverb);
Moss_AudioRemoveEffect(&reverb);
```

## Listeners
### AudioListener2
```cpp
AudioListener2D* listener2D = Moss_AudioCreateAudioListener2D();

// Activate listener
Moss_AudioActivateAudioListener2D(listener2D, true);

// Deactivate when switching contexts
Moss_AudioActivateAudioListener2D(listener2D, false);

// Remove listener
Moss_AudioRemoveAudioListener2D(listener2D);
```
### AudioListener3
```cpp
AudioListener3D* listener3D = Moss_AudioCreateAudioListener3D();

// Activate listener
Moss_AudioActivateAudioListener3D(listener3D, true);

// Set orientation
Vec3 forward = {0.0f, 0.0f, -1.0f};
Vec3 up      = {0.0f, 1.0f,  0.0f};

Moss_AudioListenerSetOrientation(listener3D, forward, up);
```


## Streams
### AudioStream
```cpp
AudioStream* stream = Moss_AudioStreamCreate();

// Configure stream
Moss_AudioStreamSetVolume(stream, 1.0f);
Moss_AudioStreamSetPitch(stream, 1.0f);
Moss_AudioStreamSetLoop(stream, false);

// Play / stop
Moss_AudioStreamPlay(stream);
Moss_AudioStreamStop(stream);

// Cleanup
Moss_AudioStreamRemove(stream);

```
### AudioStream2
```cpp
AudioStream2D* stream2D = Moss_AudioStream2DCreate();

Moss_AudioStream2DSetVolume(stream2D, 0.8f);
Moss_AudioStream2DSetPitch(stream2D, 1.0f);
Moss_AudioStream2DSetLoop(stream2D, true);

// Play sound
Moss_AudioStream2DPlay(stream2D);

// Stop sound
Moss_AudioStream2DStop(stream2D);

// Remove stream
Moss_AudioStream2DRemove(stream2D);

Moss_Play();
Moss_Pause();
Moss_Stop();
```
### AudioStream3
```cpp
AudioStream3D* stream3D = Moss_AudioStream3DCreate();

...

// Configure spatial parameters
Moss_AudioStream3DSetVolume(stream3D, 1.0f);
Moss_AudioStream3DSetPitch(stream3D, 1.0f);
Moss_AudioStream3DSetLoop(stream3D, true);
Moss_AudioStream3DSetMaxDistance(stream3D);
Moss_AudioStream3DSetDistanceModel(stream3D, DistanceModel::INVERSE);

...

// Update transform (typically each frame)
Moss_AudioStream3DSetPosition(stream3D);
Moss_AudioStream3DSetVelocity(stream3D);

...

// Play
Moss_AudioStream3DPlay(stream3D);

...

// Stop & remove
Moss_AudioStream3DStop(stream3D);
Moss_AudioStream3DRemove(stream3D);
```
### Speaker
Example
```cpp
// Open default speaker device
if (!Moss_AudioSpeakerOpen()) { return 0; }

// List available speakers
int count = Moss_ListSpeakerDevices();
for (int i = 0; i < count; ++i) {
    const char* name = Moss_GetSpeakerDeviceName(i);
}

// Select a speaker
Moss_AudioSelectSpeakerDevice(0);

// Pause / resume output
Moss_AudioSpeakerPause();
Moss_AudioSpeakerResume();
```
### Microphone
The forward microphone API is `Moss_Microphone*`. The older `Microphone*` helpers remain as compatibility wrappers.

```cpp
Moss_MicrophoneDesc desc{};
desc.device_index = 0;
desc.sample_rate = 48000;
desc.channels = 1;
desc.buffer_frames = 480;
desc.ring_buffer_frames = 48000;
desc.start_immediately = true;

Moss_Microphone* mic = Moss_MicrophoneOpen(&desc);

float frames[480] = {};
uint32_t framesRead = Moss_MicrophoneRead(mic, frames, 480);
Moss_MicrophoneLevels levels = Moss_MicrophoneGetLevels(mic);

Moss_MicrophoneStop(mic);
Moss_MicrophoneClose(mic);
```

Callback mode uses the same float sample layout as polling. The callback receives frames already converted to the microphone descriptor channel count when the backend supports conversion.

```cpp
void OnMicData(const float* buffer, int frames, void* userData) {
    // Keep this real-time safe: copy or enqueue only.
}

Moss_MicrophoneSetCallback(mic, OnMicData, nullptr);
```

Microphones can also feed audio sources through the forward-path source helper.

```cpp
Moss_AudioSource* source = Moss_AudioCaptureMossMicrophone(mic);
```

Device enumeration rules:
- `Moss_MicrophoneGetDeviceCount()` and `Moss_MicrophoneGetDeviceName(index)` enumerate capture devices for the new API.
- `Moss_ListMicrophoneDevices()` and `Moss_GetMicrophoneDeviceName(index)` are compatibility wrappers.
- Speaker enumeration uses `Moss_ListSpeakerDevices()` and `Moss_GetSpeakerDeviceName(index)`.

Sample-rate and channel rules:
- Backends open the platform mix format when required by the OS.
- Samples exposed through `Moss_MicrophoneRead` are `float` samples in the requested output channel count when conversion is available.
- Mono output from multi-channel input is averaged. Multi-channel output from fewer source channels repeats/clamps the nearest available source channel.

Thread-safety rules:
- `Moss_MicrophoneRead` is polling-friendly and protected by the microphone ring-buffer lock.
- Microphone callbacks run from the capture thread. Do not allocate heavily, call blocking APIs, or call close/stop from inside the callback.
- Level queries return the latest calculated RMS, peak, smoothed volume, and voice activity values.



## RayTraceAudio
```cpp
RayAudioListener3D* rayListener = Moss_AudioCreateRayAudioListener3D();

// Activate ray-traced listener
Moss_AudioActivateRayAudioListener3D(rayListener, true);

// Remove when no longer needed
Moss_AudioRemoveRayAudioListener3D(rayListener);

```
