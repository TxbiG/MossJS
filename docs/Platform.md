# Platform
## Overview


Platforms supported ```Windows```, ```Linux```, ```MacOS```, ```FreeBSD```, ```Android```, ```IOS```.

> [!NOTE]  
Working with any other platform not specified here will require manual creation, such as creating a window and input.

### Input Support
| Input Devices | Windows | MacOS | Linux  | IOS  | Android  |
| ------------- | ------------- | ------------- | ------------- | ------------- | ------------- |
| Keyboard      | ✅ | ✅ | ✅ | ❌ | ❌  |
| Mouse         | ✅ | ✅ | ✅ | ❌ | ❌ |
| Touch         | ✅ | ✅ | ✅ | ✅ | ✅ |
| Pen           | ✅ |✅  | ✅ | ✅ | ✅ |
| Xbox 360 Controllers  | ✅ | ✅ | ✅ | ❌ | ❌ |
| Xbox One Controllers  | ✅ | ✅ | ✅ | ❌ | ❌ |
| Xbox Series S Controllers | ✅ | ✅ | ✅ | ❌ | ❌ |
| PlayStation 4 Controllers | ✅ | ✅ | ✅ | ❌ | ❌ |
| PlayStation 5 Controllers | ✅ | ✅ | ✅ | ❌ | ❌ |

### Haptic Feedback support
| Haptic Feedback | Playstation (DualSense) | Xbox Impulse Triggers | Mouses        | Keyboard      | IOS Haptic  | Android Haptic |
| -------------   | -------------           | -------------         | ------------- | ------------- | ------------- | ------------- |
| Supported       | ✅                      | ✅                   | ❌            | ❌           | ✅           | ✅            |

### Graphics API Support
| Platforms     |Windows                       | MacOS  | Linux          | IOS              | Android               |
| ------------- | -------------                | -------| -------------  | -------------    | -------------         |
| Supported     | OpenGL, Vulkan, DirectX 12 or 12 | Metal  | OpenGL, Vulkan | OpenGL ES, Metal | OpenGL ES, Vulkan | 

- [Overview](#overview)
    - [Window](#window), [Monitor](#monitor)
    - [Input](#input), [Haptic Feedback](#haptic-feedback)
    - [VideoCapture](#videocapture)
    - [Miscellaneous](#miscellaneous)
    - [Graphics API Specific](#graphics-api-specific)

## Enums
```cpp
// Provided by Moss
enum Keyboard;
enum Mouse;
enum Moss_CursorMode;
enum Moss_CursorShape;
enum Gamepad;
enum Joystick;
enum Moss_GamepadButton;

enum Moss_Gesture;
enum Moss_PenAxis;
enum Moss_PenDeviceType;
enum Moss_TouchDeviceType;

enum Moss_HapticFeedbackType;
enum Moss_HapticEffectType;

enum Moss_CameraPermissionState;
enum Moss_CameraPosition;

enum Moss_WindowFlags;
enum Moss_MessageBoxFlags;
enum Moss_WindowMode;

enum Moss_PowerState;
enum Moss_FileDialogType;
enum Moss_UserFolder;
enum Moss_PathType;
```

## Structs
```cpp
// Provided by Moss
struct Moss_Window;
struct Moss_Monitor;
struct Moss_Cursor;

struct Moss_GammaRamp { 
    uint8_t* size, red, green, blue; 
};

struct Moss_VideoMode;
struct Moss_Image;

struct Moss_Locale {
    char* country;
    char* language;
};
struct Moss_HapticFeedback;
```

## Functions
```cpp
MOSS_API Moss_Window* Moss_CreateWindow(const char* title, int width, int height, Moss_Monitor* monitor, Moss_Window* share);
MOSS_API void Moss_TerminateWindow(Moss_Window* window);
MOSS_API bool Moss_ShouldWindowClose(Moss_Window* window);
MOSS_API void Moss_PollEvents(void);
MOSS_API int Moss_GetWindowWidth();
MOSS_API int Moss_GetWindowHeight();
MOSS_API void Moss_SetWindowTitle(Moss_Window* window, const char* title);
MOSS_API void Moss_SetWindowIcon(Moss_Window* window, Moss_Image image);
MOSS_API void Moss_CloseWindow(Moss_Window* window);

//MOSS_API void Moss_WindowShow(Moss_Window* window, bool show); 
//MOSS_API void Moss_WindowClose(Moss_Window* window); 
//MOSS_API void Moss_WindowSetOpacity(Moss_Window* window, float opacity);


MOSS_API Moss_Monitor* Moss_MonitorGetPrimary();
MOSS_API Moss_Monitor* Moss_MonitorGetSecondary();
MOSS_API void Moss_MonitorGetPhysicalSize(Moss_Monitor monitor, int* width_mm, int* height_mm);
MOSS_API void Moss_MonitorGetContentScale(Moss_Monitor monitor, float* xscale, float* yscale);
MOSS_API void Moss_MonitorGetPosition(Moss_Monitor monitor, int* x, int* y);
MOSS_API const char* Moss_MonitorGetName(Moss_Monitor monitor);
MOSS_API void Moss_MonitorSetGammaRamp(Moss_Monitor monitor, const Moss_GammaRamp* gammaRamp);
MOSS_API Moss_GammaRamp* Moss_MonitorGetGammaRamp(Moss_Monitor monitor);
MOSS_API void Moss_MonitorSetGamma(Moss_Monitor monitor, float gamma);


MOSS_API bool Moss_IsKeyPressed(Moss_Key key);
MOSS_API bool Moss_IsKeyJustPressed(Moss_Key key);
MOSS_API bool Moss_IsKeyJustReleased(Moss_Key key);
MOSS_API Keyboard Moss_InputGetKey();

MOSS_API bool Moss_IsMousePressed(Moss_MouseButton button);
MOSS_API bool Moss_IsMouseJustPressed(Moss_MouseButton button);
MOSS_API bool Moss_IsMouseJustReleased(Moss_MouseButton button);
MOSS_API Keyboard Moss_InputGetMouse();
MOSS_API void Moss_GetMousePosition(int* x, int* y);
MOSS_API void Moss_SetMousePosition(int x, int y);
MOSS_API void Moss_SetMouseVisible(bool visible);


MOSS_API int Moss_GetNumGamepads(void);
MOSS_API Moss_Gamepad* Moss_OpenGamepad(Moss_GamepadID id);
MOSS_API void Moss_CloseGamepad(Moss_Gamepad* gp);
MOSS_API bool Moss_GamepadConnected(Moss_Gamepad* gp);
MOSS_API void Moss_UpdateGamepads(void); // poll / refresh all gamepads


MOSS_API bool Moss_IsGamepadButtonPressed(Moss_Gamepad* gp, Moss_GamepadButton button);
MOSS_API bool Moss_IsGamepadButtonJustPressed(Moss_Gamepad* gp, Moss_GamepadButton button);
MOSS_API bool Moss_IsGamepadButtonJustReleased(Moss_Gamepad* gp, Moss_GamepadButton button);
MOSS_API float Moss_GetGamepadAxis(Moss_Gamepad* gp, Moss_Joystick axis);


MOSS_API bool Moss_RumbleGamepad(Moss_Gamepad* gp, uint16_t low, uint16_t high, uint32_t duration_ms);
MOSS_API bool Moss_RumbleGamepadTriggers(Moss_Gamepad* gp, uint16_t left, uint16_t right, uint32_t duration_ms);
MOSS_API bool Moss_SetGamepadLED(Moss_Gamepad* gp, uint8_t r, uint8_t g, uint8_t b);


MOSS_API const char* Moss_GetGamepadName(Moss_Gamepad* gp);
MOSS_API Moss_GamepadID Moss_GetGamepadID(Moss_Gamepad* gp);
MOSS_API int Moss_GetGamepadPlayerIndex(Moss_Gamepad* gp);
MOSS_API Moss_PowerState Moss_GetGamepadPowerInfo(Moss_Gamepad* gp, int* percent);
MOSS_API int Moss_GetNumGamepadTouchpads(Moss_Gamepad* gp);
MOSS_API int Moss_GetNumGamepadTouchpadFingers(Moss_Gamepad* gp);
MOSS_API bool Moss_GetGamepadTouchpadFinger(Moss_Gamepad* gp, int pad, int finger, bool* down, float* x, float* y, float* pressure);


MOSS_API const char* Moss_GetGamepadMapping(Moss_Gamepad* gp);
MOSS_API bool Moss_SetGamepadMapping(Moss_Gamepad* gp, const char* mapping);
MOSS_API void Moss_ReloadGamepadMappings(void);

MOSS_API Moss_GamepadButton Moss_InputGetGamepadButton();
MOSS_API Moss_GamepadAxis Moss_InputGetGamepadAxis();

MOSS_API Moss_PenDeviceType Moss_GetPenDeviceType(Moss_PenID instance_id);
MOSS_API const char* Moss_GetTouchDeviceName(Moss_TouchID touchID);
MOSS_API Moss_TouchID* Moss_GetTouchDevices(int *count);
MOSS_API Moss_TouchDeviceType Moss_GetTouchDeviceType(Moss_TouchID touchID);
MOSS_API Moss_Finger** Moss_GetTouchFingers(Moss_TouchID touchID, int *count);


MOSS_API Moss_Haptic* Moss_OpenHaptic(Moss_HapticID id);
MOSS_API void Moss_CloseHaptic(Moss_Haptic* haptic);
MOSS_API Moss_HapticID Moss_CreateHapticEffect(Moss_Haptic* haptic);
MOSS_API void Moss_DestroyHapticEffect(Moss_Haptic* haptic);
MOSS_API bool Moss_GetHapticEffectStatus(Moss_Haptic* haptic);
MOSS_API uint32_t Moss_GetHapticFeatures(Moss_Haptic* haptic);
MOSS_API Moss_Haptic* Moss_GetHapticFromID(Moss_Haptic* haptic);
MOSS_API Moss_HapticID* Moss_GetHapticID(Moss_Haptic* haptic);
MOSS_API const char* Moss_GetHapticName(Moss_Haptic* haptic);
MOSS_API const char* Moss_GetHapticNameForID(Moss_Haptic* haptic);
MOSS_API Moss_HapticID* Moss_GetHaptics(Moss_Haptic* haptic);
MOSS_API int Moss_GetMaxHapticEffects(Moss_Haptic* haptic);
MOSS_API int Moss_GetMaxHapticEffectsPlaying(Moss_Haptic* haptic);
MOSS_API int Moss_GetNumHapticAxes(Moss_Haptic* haptic);
MOSS_API bool Moss_HapticEffectSupported(Moss_Haptic* haptic);
MOSS_API bool Moss_HapticRumbleSupported(Moss_Haptic* haptic);
MOSS_API bool Moss_InitHapticRumble(Moss_Haptic* joystick);
MOSS_API bool Moss_IsJoystickHaptic(Moss_Joystick);
MOSS_API bool Moss_IsMouseHaptic(void);
MOSS_API Moss_Haptic* Moss_OpenHapticFromJoystick(Moss_Joystick* joystick);
MOSS_API Moss_Haptic* Moss_OpenHapticFromMouse(void);
MOSS_API bool Moss_PauseHaptic(Moss_Haptic* haptic);
MOSS_API bool Moss_PlayHapticRumble(Moss_Haptic* haptic, float strength, uint32_t length);
MOSS_API bool Moss_ResumeHaptic(Moss_Haptic* haptic);
MOSS_API bool Moss_RunHapticEffect(Moss_Haptic* haptic, uint32_t iterations);
MOSS_API bool Moss_SetHapticAutocenter(Moss_Haptic* haptic, int center);
MOSS_API bool Moss_SetHapticGain(Moss_Haptic* haptic, int gain);
MOSS_API bool Moss_StopHapticEffect(Moss_Haptic* haptic, Moss_HapticEffectID effect);
MOSS_API bool Moss_StopHapticEffects(Moss_Haptic* haptic);
MOSS_API bool Moss_StopHapticRumble(Moss_Haptic* haptic);
MOSS_API bool Moss_UpdateHapticEffect(Moss_Haptic* haptic, Moss_HapticEffectID effect, const Moss_HapticEffect* data);


MOSS_API int Moss_GetAvailableCPUCores(void);
MOSS_API int Moss_GetCPUCacheLineSize(void);
MOSS_API int Moss_GetSystemRAM(void);


MOSS_API bool Moss_OpenURL(const char *url);
MOSS_API Moss_Locale* Moss_GetLocale();
MOSS_API Moss_PowerState Moss_GetPowerInfo(int *seconds, int *percent);
MOSS_API bool Moss_IsProcessRunningByName(const char* executable_path);
MOSS_API void* Moss_LoadDynamicLibrary(const char* lib_path);
MOSS_API void* Moss_GetLibrarySymbol(void* handle, const char* symbol_name);
MOSS_API void Moss_UnloadDynamicLibrary(void* handle);


MOSS_API void Moss_SetFramebufferResizeCallback(Moss_FramebufferResizeCallback callback);
MOSS_API void Moss_SetWindowSizeCallback(Moss_WindowSizeCallback callback);
MOSS_API void Moss_SetWindowResizeCallback(Moss_WindowResizeCallback callback);
MOSS_API void Moss_SetWindowPositionCallback(Moss_WindowPositionCallback callback);
MOSS_API void Moss_SetWindowFocusCallback(Moss_WindowFocusCallback callback);
MOSS_API void Moss_SetWindowContentScaleCallback(Moss_WindowContentScaleCallback callback);
MOSS_API void Moss_SetMonitorCallback(Moss_MonitorCallback callback);


MOSS_API Moss_CameraID* Moss_GetCameras(int* count);
MOSS_API const char* Moss_GetCameraName(Moss_CameraID camera_id);
MOSS_API Moss_CameraPosition Moss_GetCameraPosition(Moss_CameraID camera_id);
MOSS_API const char* Moss_GetCurrentCameraDriver(void);
MOSS_API int Moss_GetNumCameraDrivers(void);
MOSS_API const Moss_CameraSpec* Moss_GetCameraSupportedFormats(Moss_CameraID camera_id, int* count);
MOSS_API Moss_Surface* Moss_AcquireCameraFrame(Moss_Camera* camera, uint64_t* timestamp_ns);
MOSS_API void Moss_ReleaseCameraFrame(Moss_Camera* camera, Moss_Surface* frame);
MOSS_API bool Moss_GetCameraFormat(Moss_Camera* camera, Moss_CameraSpec* out_spec);
MOSS_API Moss_CameraPermissionState Moss_GetCameraPermissionState(Moss_Camera* camera);
MOSS_API Moss_PropertiesID Moss_GetCameraProperties(Moss_Camera* camera);
MOSS_API void Moss_CloseCamera(Moss_Camera *camera);
MOSS_API Moss_CameraID Moss_GetCameraID(Moss_Camera *camera);
MOSS_API Moss_PropertiesID Moss_GetCameraProperties(Moss_Camera *camera);
MOSS_API Moss_Camera * Moss_OpenCamera(Moss_CameraID instance_id, const Moss_CameraSpec *spec);
MOSS_API Moss_VideoCapture* Moss_OpenVideoCapture(Moss_VideoCaptureID captureID);
MOSS_API void Moss_CloseVideoCapture(Moss_VideoCapture* cap);
MOSS_API uint8_t* Moss_VideoCaptureReadFrame(Moss_VideoCapture* cap);


MOSS_API bool Moss_CopyFile(const char* src_path, const char* dst_path, bool overwrite);
MOSS_API bool Moss_CreateDirectory(const char* path, bool recursive);
MOSS_API bool Moss_RemovePath(const char* path, bool recursive);
MOSS_API bool Moss_RenamePath(const char* old_path, const char* new_path, bool overwrite);


MOSS_API bool Moss_GetPathInfo(const char* path, Moss_PathInfo* out_info);
MOSS_API bool Moss_GetCurrentDirectory(char* out_path, int max_len);
MOSS_API bool Moss_GetBasePath(char* out_path, int max_len);

MOSS_API bool Moss_GetUserFolder(Moss_UserFolder folder, char* out_path, int max_len);
MOSS_API bool Moss_GetPrefPath(const char* org_name, const char* app_name, char* out_path, int max_len);
MOSS_API bool Moss_EnumerateDirectory( const char* path, bool recursive, Moss_DirectoryIterateFn callback, void* user_data);
MOSS_API bool Moss_GlobDirectory(const char* pattern, Moss_DirectoryIterateFn callback, void* user_data);


MOSS_API void Moss_ShowFileDialogWithProperties(Moss_DialogFileCallback callback, void *userdata, Moss_Window *window, const Moss_DialogFileFilter *filters, int nfilters, const char *default_location);
MOSS_API void Moss_ShowOpenFileDialog(Moss_DialogFileCallback callback, void *userdata, Moss_Window *window, const char *default_location, bool allow_many);
MOSS_API void Moss_ShowOpenFolderDialog(Moss_DialogFileCallback callback, void *userdata, Moss_Window *window, const Moss_DialogFileFilter *filters, int nfilters, const char *default_location, bool allow_many);
MOSS_API void Moss_ShowSaveFileDialog(Moss_FileDialogType type, Moss_DialogFileCallback callback, void *userdata, Moss_PropertiesID props);

MOSS_API bool Moss_CloseStorage(Moss_Storage *storage);
MOSS_API bool Moss_CopyStorageFile(Moss_Storage *storage, const char *oldpath, const char *newpath);
MOSS_API bool Moss_CreateStorageDirectory(Moss_Storage *storage, const char *path);
MOSS_API bool Moss_EnumerateStorageDirectory(Moss_Storage *storage, const char *path, Moss_EnumerateDirectoryCallback callback, void *userdata);
MOSS_API bool Moss_GetStorageFileSize(Moss_Storage *storage, const char *path, uint64_t *length);
MOSS_API uint64_t Moss_GetStorageSpaceRemaining(Moss_Storage *storage);
MOSS_API bool Moss_GetStoragePathInfo(Moss_Storage *storage, const char *path, Moss_PathInfo *info);
MOSS_API char** Moss_GlobStorageDirectory(Moss_Storage *storage, const char *path, const char *pattern, Moss_GlobFlags flags, int *count);
MOSS_API Moss_Storage* Moss_OpenFileStorage(const char *path);
MOSS_API Moss_Storage* Moss_OpenStorage(const Moss_Storage *iface, void *userdata);
MOSS_API Moss_Storage* Moss_OpenTitleStorage(const char *override, Moss_PropertiesID props);
MOSS_API Moss_Storage* Moss_OpenUserStorage(const char *org, const char *app, Moss_PropertiesID props);
MOSS_API bool Moss_ReadStorageFile(Moss_Storage *storage, const char *path, void *destination, uint64_t length);
MOSS_API bool Moss_RemoveStoragePath(Moss_Storage *storage, const char *path);
MOSS_API bool Moss_RenameStoragePath(Moss_Storage *storage, const char *oldpath, const char *newpath);
MOSS_API bool Moss_StorageReady(Moss_Storage *storage);
MOSS_API bool Moss_WriteStorageFile(Moss_Storage *storage, const char *path, const void *source, uint64_t length);
```

## Examples
### Window
```cpp
#pragma once
#include <Moss/Moss.h>

int main()
{

  Moss_Window* m_window = Moss_CreateWindow("Game", CENTER_POSITION, CENTER_POSITION, 700, 600, NULL, NULL);
  if (!m_window) { return 0; }

  while(!Moss_ShouldWindowClose(m_window))
  {
      PollEvents();
      /*    Code    */
      SwapBuffers(m_window);
  }

  Moss_TerminateWindow(m_window);
  return 0;
}
```

Callback for framebuffer resize events. @param width  New framebuffer width, in pixels. @param height New framebuffer height, in pixels.
```cpp
typedef void (*Moss_FramebufferResizeCallback)(int width, int height);
```
Sets the framebuffer resize callback. callback Pointer to a function to be invoked when framebuffer size changes.
```cpp
MOSS_API void Moss_SetFramebufferResizeCallback(Moss_FramebufferResizeCallback callback);
```

Callback for logical window size changes. @param width  New window width, in screen coordinates. @param height New window height, in screen coordinates.
```cpp
typedef void (*Moss_WindowSizeCallback)(int width, int height);
```
Sets the window size callback. Callback Pointer to a function invoked when the window size changes.
```cpp
MOSS_API void Moss_SetWindowSizeCallback(Moss_WindowSizeCallback callback);
```

Callback for window position changes on screen. @param xpos New X coordinate of the window’s top-left corner. @param ypos New Y coordinate of the window’s top-left corner.
```cpp
typedef void (*Moss_WindowPositionCallback)(int xpos, int ypos);
```
Sets the window resize callback (platform-level, e.g. minimize/maximize). @param callback Pointer to a function invoked when window resizing events occur.
```cpp
MOSS_API void Moss_SetWindowResizeCallback(Moss_WindowResizeCallback callback);
```

Callback for window focus events. @param focused True if the window gained focus; false if it lost focus.
```cpp
typedef void (*Moss_WindowFocusCallback)(bool focused);
```
Sets the window position callback. Callback Pointer to a function invoked when the window position changes.
```cpp
MOSS_API void Moss_SetWindowPositionCallback(Moss_WindowPositionCallback callback);
```

Callback for content scale changes (e.g., HiDPI scaling). @param xscale X-axis content scale factor. @param yscale Y-axis content scale factor.
```cpp
typedef void (*Moss_WindowContentScaleCallback)(float xscale, float yscale);
```
Sets the window focus callback. Callback Pointer to a function invoked when the window focus changes.
```cpp
MOSS_API void Moss_SetWindowFocusCallback(Moss_WindowFocusCallback callback);
```

Callback for general window resize notifications (platform-driven). @param width  New window width in pixels. @param height New window height in pixels.
```cpp
typedef void (*Moss_WindowResizeCallback)(int width, int height);
```
Sets the window content scale callback (for HiDPI / Retina support). @param callback Pointer to a function invoked when the content scale changes
```cpp
MOSS_API void Moss_SetWindowContentScaleCallback(Moss_WindowContentScaleCallback callback);
```

### Monitor
```cpp
```
Callback for monitor configuration changes (e.g. hotplug events). @param monitorName Name or ID of the monitor that changed. connected True if the monitor was connected; false if disconnected.
```cpp
typedef void (*Moss_MonitorCallback)(const char* monitorName, bool connected);
```
Sets the monitor connection or configuration callback. Callback pointer to a function invoked when a monitor is connected or disconnected
```cpp
MOSS_API void Moss_SetMonitorCallback(Moss_MonitorCallback callback);
```

### Input
```cpp
#pragma once
#include <Moss/Moss.h>

```
### Haptic Feedback
```cpp

```
### Video Capture
```cpp
#pragma once
#include <Moss/Moss.h>
```

## Storage / File
```cpp
#pragma once
#include <Moss/Moss.h>

```
## Miscellaneous
```cpp
```

##

## Embedded window Specific
These are used for creating windows for mobile and console-specific
```cpp
MOSS_API void Moss_CreateEmbeddedWindow();
```
```cpp
MOSS_API void Moss_Terminate_EmbeddedWindow();
```
```cpp
MOSS_API void Moss_EmbeddedWindow_OnOrientationChanged(bool focused);
```
```cpp
MOSS_API void Moss_EmbeddedWindow_OnResume();
```
```cpp
MOSS_API void Moss_EmbeddedWindow_OnPause();
```

## Graphics API Specific
### OpenGL / OpenGL ES
```cpp
void Moss_MakeContextCurrent(Moss_Window* window);
```
```cpp
void Moss_SwapBuffers(); // < Used in Moss_Renderer so only call if you are making your own renderer
```
Moss_SwapBuffersInterval is used for V-Sync. However, it may cause the game to lag and introduce input lag.
```cpp
void Moss_SwapBuffersInterval(int interval);
```

```cpp
void* Moss_GetProcAddress(const char* procname);
```
##

### Vulkan
```cpp
VkResult Moss_CreateWindowSurface(Moss_Window* window, VkInstance vk_instance,
  const VkAllocationCallbacks *allocator, VkSurfaceKHR* vk_surface);
```
Moss_VulkanSupported is used to help check if the Vulkan driver is installed.
```cpp
int Moss_VulkanSupported(void);
```
```cpp
void Moss_InitVulkanLoader(PFN_vkGetInstanceProcAddr loader);
```
```cpp
const char** Moss_GetRequiredInstanceExtensions(uint32_t* count);
```
```cpp
void* Moss_GetInstanceProcAddress(VkInstance instance, const char* procname);
```
```cpp
int Moss_GetPhysicalDevicePresentationSupport(Moss_Window& window, VkPhysicalDevice device, uint32_t queuefamily);
```
