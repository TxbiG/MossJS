#include <Moss/Moss_Platform.h>
#include <Moss/Moss_stdinc.h>
#include <Moss/Moss_Web.h>

#if defined(MOSS_PLATFORM_WASM)

#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/html5_webgl.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

namespace {

struct Moss_Window {
    std::string title;
    int width = 1280;
    int height = 720;
    bool closed = false;
    bool focused = true;
    bool borderless = false;
    bool always_on_top = false;
};

struct Moss_Monitor {
    int index = 0;
};

struct Moss_Gamepad {
    Moss_GamepadID id = 0;
    int index = -1;
};

struct Moss_Haptic {
    Moss_HapticID id = 0;
    Moss_Gamepad* gamepad = nullptr;
};

struct Moss_GamepadAxis {
    int unused = 0;
};

struct Moss_Capture {
    Moss_CameraID id = 0;
};

struct Moss_Surface {
    int unused = 0;
};

struct Moss_Storage {
    std::string root;
};

Moss_Window* g_window = nullptr;
Moss_Monitor g_primary_monitor{};
Moss_Monitor g_secondary_monitor{};
Moss_Window* g_last_window = nullptr;

Moss_FramebufferResizeCallback g_framebuffer_resize_callback = nullptr;
Moss_WindowSizeCallback g_window_size_callback = nullptr;
Moss_WindowResizeCallback g_window_resize_callback = nullptr;
Moss_WindowPositionCallback g_window_position_callback = nullptr;
Moss_WindowFocusCallback g_window_focus_callback = nullptr;
Moss_WindowContentScaleCallback g_window_content_scale_callback = nullptr;
Moss_MonitorCallback g_monitor_callback = nullptr;

int g_last_width = -1;
int g_last_height = -1;
bool g_last_focus = true;
float g_last_scale = 1.0f;
EMSCRIPTEN_WEBGL_CONTEXT_HANDLE g_webgl_context = 0;
std::string g_canvas_selector = "#canvas";

std::string g_locale_country = "";
std::string g_locale_language = "";
Moss_Locale g_locale{};

std::string g_time_string;

std::string normalize_storage_path(const std::string& root, const char* path)
{
    std::filesystem::path base(root.empty() ? "/" : root);
    std::filesystem::path relative(path ? path : "");
    if (relative.is_absolute())
        relative = relative.lexically_relative(relative.root_path());
    return (base / relative).lexically_normal().string();
}

bool write_string(char* out, int max_len, const std::string& value)
{
    if (!out || max_len <= 0)
        return false;

    const std::size_t copy_len = std::min<std::size_t>(
        value.size(), static_cast<std::size_t>(max_len - 1));
    std::memcpy(out, value.data(), copy_len);
    out[copy_len] = '\0';
    return copy_len == value.size();
}

int64_t now_ns()
{
    return static_cast<int64_t>(emscripten_get_now() * 1000000.0);
}

bool wildcard_match(const char* pattern, const std::string& value, bool insensitive)
{
    if (!pattern)
        return false;

    std::string p(pattern);
    std::string v(value);

    if (insensitive) {
        std::transform(p.begin(), p.end(), p.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        std::transform(v.begin(), v.end(), v.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    }

    std::size_t pi = 0;
    std::size_t vi = 0;
    std::size_t star = std::string::npos;
    std::size_t mark = 0;

    while (vi < v.size()) {
        if (pi < p.size() && (p[pi] == '?' || p[pi] == v[vi])) {
            ++pi;
            ++vi;
        } else if (pi < p.size() && p[pi] == '*') {
            star = pi++;
            mark = vi;
        } else if (star != std::string::npos) {
            pi = star + 1;
            vi = ++mark;
        } else {
            return false;
        }
    }

    while (pi < p.size() && p[pi] == '*')
        ++pi;

    return pi == p.size();
}

void fill_path_info(const std::filesystem::directory_entry& entry, Moss_PathInfo* info)
{
    if (!info)
        return;

    std::memset(info, 0, sizeof(*info));

    std::error_code ec;
    if (entry.is_directory(ec))
        info->type = Moss_PathType::DIRECTORY;
    else if (entry.is_symlink(ec))
        info->type = Moss_PathType::SYMLINK;
    else if (entry.is_regular_file(ec))
        info->type = Moss_PathType::FILE;
    else
        info->type = Moss_PathType::UNKNOWN;

    info->size = entry.is_regular_file(ec) ? entry.file_size(ec) : 0;
    info->readable = true;
    info->writable = true;
    info->executable = false;
}

bool enumerate_directory_impl(const char* path, bool recursive,
                              Moss_DirectoryIterateFn callback, void* user_data)
{
    if (!callback)
        return false;

    std::error_code ec;
    const std::filesystem::path root(path ? path : ".");

    if (!std::filesystem::exists(root, ec))
        return false;

    if (recursive) {
        for (std::filesystem::recursive_directory_iterator it(root, ec), end; it != end; it.increment(ec)) {
            if (ec)
                return false;
            Moss_PathInfo info{};
            fill_path_info(*it, &info);
            if (!callback(&info, it->path().string().c_str(), user_data))
                return true;
        }
    } else {
        for (std::filesystem::directory_iterator it(root, ec), end; it != end; it.increment(ec)) {
            if (ec)
                return false;
            Moss_PathInfo info{};
            fill_path_info(*it, &info);
            if (!callback(&info, it->path().string().c_str(), user_data))
                return true;
        }
    }

    return true;
}

} // namespace

namespace MossWeb {

namespace {

EM_JS(void, JS_SetCanvas, (const char* selector), {
    const s = UTF8ToString(selector || 0);
    const canvas = document.querySelector(s);
    if (!canvas) return;
    Module.canvas = canvas;
    Module.mossWebCanvas = canvas;
});

EM_JS(int, JS_GetCanvasWidth, (), {
    const c = Module.mossWebCanvas || Module.canvas;
    return c ? (c.width | 0) : 0;
});

EM_JS(int, JS_GetCanvasHeight, (), {
    const c = Module.mossWebCanvas || Module.canvas;
    return c ? (c.height | 0) : 0;
});

EM_JS(void, JS_SetCanvasSize, (int width, int height), {
    const c = Module.mossWebCanvas || Module.canvas;
    if (!c) return;
    c.width = width;
    c.height = height;
});

EM_JS(void, JS_ResizeCanvasToDisplaySize, (), {
    const c = Module.mossWebCanvas || Module.canvas;
    if (!c) return;
    const dpr = Math.max(1, globalThis.devicePixelRatio || 1);
    const w = Math.max(1, Math.round((c.clientWidth || c.width) * dpr));
    const h = Math.max(1, Math.round((c.clientHeight || c.height) * dpr));
    if (c.width !== w) c.width = w;
    if (c.height !== h) c.height = h;
});

EM_JS(float, JS_GetDevicePixelRatio, (), {
    return Math.max(1, globalThis.devicePixelRatio || 1);
});

EM_JS(void, JS_RequestFullscreen, (), {
    const c = Module.mossWebCanvas || Module.canvas;
    if (!c) return;
    (c.requestFullscreen || c.webkitRequestFullscreen || c.msRequestFullscreen)?.call(c);
});

EM_JS(void, JS_ExitFullscreen, (), {
    (document.exitFullscreen || document.webkitExitFullscreen || document.msExitFullscreen)?.call(document);
});

EM_JS(bool, JS_IsFullscreen, (), {
    const c = Module.mossWebCanvas || Module.canvas;
    return !!c && (document.fullscreenElement === c || document.webkitFullscreenElement === c);
});

EM_JS(void, JS_RequestPointerLock, (), {
    const c = Module.mossWebCanvas || Module.canvas;
    if (!c) return;
    (c.requestPointerLock || c.webkitRequestPointerLock)?.call(c);
});

EM_JS(void, JS_ExitPointerLock, (), {
    (document.exitPointerLock || document.webkitExitPointerLock)?.call(document);
});

EM_JS(bool, JS_IsPointerLocked, (), {
    const c = Module.mossWebCanvas || Module.canvas;
    return !!c && (document.pointerLockElement === c || document.webkitPointerLockElement === c);
});

EM_JS(void, JS_SetCanvasCursor, (const char* cursor), {
    const c = Module.mossWebCanvas || Module.canvas;
    if (c) c.style.cursor = UTF8ToString(cursor || 0);
});

EM_JS(void, JS_SetCanvasFocus, (), {
    const c = Module.mossWebCanvas || Module.canvas;
    if (c) c.focus?.();
});

EM_JS(bool, JS_IsFocused, (), {
    const c = Module.mossWebCanvas || Module.canvas;
    return !!c && document.activeElement === c;
});

EM_JS(int, JS_MouseX, (), {
    return (Module.mossWebInput?.mouseX || 0) | 0;
});

EM_JS(int, JS_MouseY, (), {
    return (Module.mossWebInput?.mouseY || 0) | 0;
});

EM_JS(bool, JS_KeyPressed, (int key), {
    const codes = [
        "Digit0","Digit1","Digit2","Digit3","Digit4","Digit5","Digit6","Digit7","Digit8","Digit9",
        "KeyA","KeyB","KeyC","KeyD","KeyE","KeyF","KeyG","KeyH","KeyI","KeyJ","KeyK","KeyL","KeyM",
        "KeyN","KeyO","KeyP","KeyQ","KeyR","KeyS","KeyT","KeyU","KeyV","KeyW","KeyX","KeyY","KeyZ",
        "Quote","Backslash","Comma","Equal","Backquote","BracketLeft","Minus","Period","BracketRight","Semicolon","Slash",
        "IntlBackslash","IntlRo","Backspace","Delete","End","Enter","Escape","Home","Insert","ContextMenu",
        "PageDown","PageUp","Pause","Space","Tab","CapsLock","NumLock","ScrollLock",
        "F1","F2","F3","F4","F5","F6","F7","F8","F9","F10","F11","F12","F13","F14","F15","F16","F17","F18","F19","F20","F21","F22","F23","F24",
        "AltLeft","ControlLeft","ShiftLeft","MetaLeft","PrintScreen","AltRight","ControlRight","ShiftRight","MetaRight",
        "ArrowDown","ArrowLeft","ArrowRight","ArrowUp",
        "Numpad0","Numpad1","Numpad2","Numpad3","Numpad4","Numpad5","Numpad6","Numpad7","Numpad8","Numpad9",
        "NumpadAdd","NumpadDecimal","NumpadDivide","NumpadEnter","NumpadEqual","NumpadMultiply","NumpadSubtract"
    ];
    const c = codes[key | 0];
    return !!c && !!Module.mossWebInput?.keys?.has(c);
});

EM_JS(bool, JS_KeyJustPressed, (int key), {
    const c = Module.mossWebInput?.pressed;
    if (!c) return false;
    const codes = [
        "Digit0","Digit1","Digit2","Digit3","Digit4","Digit5","Digit6","Digit7","Digit8","Digit9",
        "KeyA","KeyB","KeyC","KeyD","KeyE","KeyF","KeyG","KeyH","KeyI","KeyJ","KeyK","KeyL","KeyM",
        "KeyN","KeyO","KeyP","KeyQ","KeyR","KeyS","KeyT","KeyU","KeyV","KeyW","KeyX","KeyY","KeyZ",
        "Quote","Backslash","Comma","Equal","Backquote","BracketLeft","Minus","Period","BracketRight","Semicolon","Slash",
        "IntlBackslash","IntlRo","Backspace","Delete","End","Enter","Escape","Home","Insert","ContextMenu",
        "PageDown","PageUp","Pause","Space","Tab","CapsLock","NumLock","ScrollLock",
        "F1","F2","F3","F4","F5","F6","F7","F8","F9","F10","F11","F12","F13","F14","F15","F16","F17","F18","F19","F20","F21","F22","F23","F24",
        "AltLeft","ControlLeft","ShiftLeft","MetaLeft","PrintScreen","AltRight","ControlRight","ShiftRight","MetaRight",
        "ArrowDown","ArrowLeft","ArrowRight","ArrowUp",
        "Numpad0","Numpad1","Numpad2","Numpad3","Numpad4","Numpad5","Numpad6","Numpad7","Numpad8","Numpad9",
        "NumpadAdd","NumpadDecimal","NumpadDivide","NumpadEnter","NumpadEqual","NumpadMultiply","NumpadSubtract"
    ];
    const code = codes[key | 0];
    return !!code && c.has(code);
});

EM_JS(bool, JS_KeyJustReleased, (int key), {
    const c = Module.mossWebInput?.released;
    if (!c) return false;
    const codes = [
        "Digit0","Digit1","Digit2","Digit3","Digit4","Digit5","Digit6","Digit7","Digit8","Digit9",
        "KeyA","KeyB","KeyC","KeyD","KeyE","KeyF","KeyG","KeyH","KeyI","KeyJ","KeyK","KeyL","KeyM",
        "KeyN","KeyO","KeyP","KeyQ","KeyR","KeyS","KeyT","KeyU","KeyV","KeyW","KeyX","KeyY","KeyZ",
        "Quote","Backslash","Comma","Equal","Backquote","BracketLeft","Minus","Period","BracketRight","Semicolon","Slash",
        "IntlBackslash","IntlRo","Backspace","Delete","End","Enter","Escape","Home","Insert","ContextMenu",
        "PageDown","PageUp","Pause","Space","Tab","CapsLock","NumLock","ScrollLock",
        "F1","F2","F3","F4","F5","F6","F7","F8","F9","F10","F11","F12","F13","F14","F15","F16","F17","F18","F19","F20","F21","F22","F23","F24",
        "AltLeft","ControlLeft","ShiftLeft","MetaLeft","PrintScreen","AltRight","ControlRight","ShiftRight","MetaRight",
        "ArrowDown","ArrowLeft","ArrowRight","ArrowUp",
        "Numpad0","Numpad1","Numpad2","Numpad3","Numpad4","Numpad5","Numpad6","Numpad7","Numpad8","Numpad9",
        "NumpadAdd","NumpadDecimal","NumpadDivide","NumpadEnter","NumpadEqual","NumpadMultiply","NumpadSubtract"
    ];
    const code = codes[key | 0];
    return !!code && c.has(code);
});

EM_JS(int, JS_InputGetKey, (), {
    const s = Module.mossWebInput;
    if (!s?.lastKeyCode) return -1;
    const codes = s.keyCodes || [];
    return codes.indexOf(s.lastKeyCode);
});

EM_JS(int, JS_InputGetMouseButton, (), {
    return Module.mossWebInput?.lastMouseButton ?? -1;
});

EM_JS(bool, JS_MousePressed, (int button), {
    return !!Module.mossWebInput?.mouseButtons?.has(button | 0);
});

EM_JS(bool, JS_MouseJustPressed, (int button), {
    return !!Module.mossWebInput?.mousePressed?.has(button | 0);
});

EM_JS(bool, JS_MouseJustReleased, (int button), {
    return !!Module.mossWebInput?.mouseReleased?.has(button | 0);
});

EM_JS(void, JS_SetMousePosition, (int x, int y), {
    const c = Module.mossWebCanvas || Module.canvas;
    if (!c) return;
    Module.mossWebInput.mouseX = x | 0;
    Module.mossWebInput.mouseY = y | 0;
});

EM_JS(void, JS_SetMouseVisible, (bool visible), {
    const c = Module.mossWebCanvas || Module.canvas;
    if (c) c.style.cursor = visible ? "default" : "none";
});

EM_JS(int, JS_GamepadCount, (), {
    const pads = navigator.getGamepads?.() || [];
    let count = 0;
    for (const p of pads) if (p && p.connected) ++count;
    return count;
});

EM_JS(bool, JS_GamepadConnected, (int index), {
    const p = (navigator.getGamepads?.() || [])[index | 0];
    return !!p && !!p.connected;
});

EM_JS(bool, JS_GamepadButtonPressed, (int index, int button), {
    const p = (navigator.getGamepads?.() || [])[index | 0];
    const b = p?.buttons?.[button | 0];
    return !!b && !!b.pressed;
});

EM_JS(bool, JS_GamepadButtonJustPressed, (int index, int button), {
    const s = Module.mossWebGamepads;
    if (!s) return false;
    return !!s.pressed?.get((index | 0) + ':' + (button | 0));
});

EM_JS(bool, JS_GamepadButtonJustReleased, (int index, int button), {
    const s = Module.mossWebGamepads;
    if (!s) return false;
    return !!s.released?.get((index | 0) + ':' + (button | 0));
});

EM_JS(float, JS_GamepadAxis, (int index, int axis), {
    const p = (navigator.getGamepads?.() || [])[index | 0];
    return p?.axes?.[axis | 0] || 0;
});

EM_JS(void, JS_UpdateGamepads, (), {
    const previous = Module.mossWebGamepads?.current || new Map();
    const current = new Map();
    const pressed = new Map();
    const released = new Map();
    const pads = navigator.getGamepads?.() || [];

    for (const p of pads) {
        if (!p || !p.connected) continue;
        const index = p.index | 0;
        for (let i = 0; i < p.buttons.length; ++i) {
            const down = !!p.buttons[i]?.pressed;
            const key = index + ':' + i;
            current.set(key, down);
            if (down && !previous.get(key)) pressed.set(key, true);
            if (!down && previous.get(key)) released.set(key, true);
        }
    }

    Module.mossWebGamepads = { current, pressed, released };
});

EM_JS(const char*, JS_GamepadName, (int index), {
    const p = (navigator.getGamepads?.() || [])[index | 0];
    const name = p?.id || "Web Gamepad";
    const n = lengthBytesUTF8(name) + 1;
    const ptr = _malloc(n);
    stringToUTF8(name, ptr, n);
    return ptr;
});

EM_JS(bool, JS_RumbleGamepad, (int index, float weak, float strong, int duration), {
    const p = (navigator.getGamepads?.() || [])[index | 0];
    const a = p?.vibrationActuator;
    if (!a || !a.playEffect) return false;
    try {
        a.playEffect("dual-rumble", {
            duration: Math.max(0, duration | 0),
            weakMagnitude: Math.max(0, Math.min(1, weak)),
            strongMagnitude: Math.max(0, Math.min(1, strong))
        });
        return true;
    } catch (_) {
        return false;
    }
});

EM_JS(bool, JS_OpenURL, (const char* url), {
    const u = UTF8ToString(url || 0);
    if (!u) return false;
    const w = globalThis.open(u, "_blank", "noopener,noreferrer");
    return !!w;
});

EM_JS(const char*, JS_Language, (), {
    const value = navigator.language || "en";
    const n = lengthBytesUTF8(value) + 1;
    const ptr = _malloc(n);
    stringToUTF8(value, ptr, n);
    return ptr;
});

EM_JS(void, JS_Free, (const void* ptr), {
    if (ptr) _free(ptr);
});

EM_JS(const char*, JS_Country, (), {
    let value = "";
    try {
        const locale = new Intl.Locale(navigator.language || "en");
        value = locale.region || "";
    } catch (_) {}
    const n = lengthBytesUTF8(value) + 1;
    const ptr = _malloc(n);
    stringToUTF8(value, ptr, n);
    return ptr;
});

EM_JS(int, JS_TouchDeviceCount, (), {
    return (globalThis.navigator?.maxTouchPoints || 0) > 0 ? 1 : 0;
});

EM_JS(int, JS_TouchCount, (), {
    return Module.mossWebInput?.touches?.size || 0;
});

EM_JS(void, JS_TouchInfo, (int index, uint32_t* idPtr, float* xPtr, float* yPtr, float* pressurePtr), {
    const s = Module.mossWebInput;
    if (!s?.touches) return;
    const values = Array.from(s.touches.values());
    const t = values[index | 0];
    if (!t) return;
    HEAPU32[idPtr >> 2] = t.id >>> 0;
    HEAPF32[xPtr >> 2] = Math.max(0, Math.min(1, t.x));
    HEAPF32[yPtr >> 2] = Math.max(0, Math.min(1, t.y));
    HEAPF32[pressurePtr >> 2] = Math.max(0, Math.min(1, t.pressure ?? 1));
});

EM_JS(void, JS_InitInput, (), {
    if (Module.mossWebInput) return;
    const state = {
        keys: new Set(), pressed: new Set(), released: new Set(),
        mouseButtons: new Set(), mousePressed: new Set(), mouseReleased: new Set(),
        mouseX: 0, mouseY: 0, wheelX: 0, wheelY: 0,
        lastKey: -1, lastMouseButton: -1,
        touches: new Map()
    };
    Module.mossWebInput = state;

    const canvas = () => Module.mossWebCanvas || Module.canvas || document;
    const updatePointer = (e) => {
        const c = Module.mossWebCanvas || Module.canvas;
        if (!c) return;
        const r = c.getBoundingClientRect();
        state.mouseX = Math.round((e.clientX - r.left) * ((c.width || r.width) / Math.max(1, r.width)));
        state.mouseY = Math.round((e.clientY - r.top) * ((c.height || r.height) / Math.max(1, r.height)));
    };

    addEventListener("keydown", (e) => {
        state.lastKeyCode = e.code;
        if (!state.keys.has(e.code)) state.pressed.add(e.code);
        state.keys.add(e.code);
    }, { passive: true });
    addEventListener("keyup", (e) => {
        state.lastKeyCode = e.code;
        state.keys.delete(e.code);
        state.released.add(e.code);
    }, { passive: true });
    addEventListener("blur", () => {
        state.keys.clear();
        state.mouseButtons.clear();
    });

    document.addEventListener("pointerdown", (e) => {
        updatePointer(e);
        state.lastMouseButton = e.button | 0;
        state.mouseButtons.add(e.button);
        state.mousePressed.add(e.button);
        if (e.pointerType === "touch") {
            const c = Module.mossWebCanvas || Module.canvas;
            const r = c?.getBoundingClientRect?.();
            const x = r ? (e.clientX - r.left) / Math.max(1, r.width) : 0;
            const y = r ? (e.clientY - r.top) / Math.max(1, r.height) : 0;
            state.touches.set(e.pointerId, { id: e.pointerId >>> 0, x, y, pressure: e.pressure || 1 });
        }
    }, { passive: true });
    document.addEventListener("pointerup", (e) => {
        updatePointer(e);
        state.lastMouseButton = e.button | 0;
        state.mouseButtons.delete(e.button);
        state.mouseReleased.add(e.button);
        if (e.pointerType === "touch") state.touches.delete(e.pointerId);
    }, { passive: true });
    document.addEventListener("pointercancel", (e) => {
        if (e.pointerType === "touch") state.touches.delete(e.pointerId);
        state.mouseButtons.delete(e.button | 0);
        state.mouseReleased.add(e.button | 0);
    }, { passive: true });
    document.addEventListener("pointermove", (e) => {
        updatePointer(e);
        if (e.pointerType === "touch") {
            const c = Module.mossWebCanvas || Module.canvas;
            const r = c?.getBoundingClientRect?.();
            if (r && state.touches.has(e.pointerId)) {
                state.touches.set(e.pointerId, {
                    id: e.pointerId >>> 0,
                    x: (e.clientX - r.left) / Math.max(1, r.width),
                    y: (e.clientY - r.top) / Math.max(1, r.height),
                    pressure: e.pressure || 1
                });
            }
        }
    }, { passive: true });
    document.addEventListener("wheel", (e) => {
        state.wheelX += e.deltaX;
        state.wheelY += e.deltaY;
    }, { passive: true });
});

EM_JS(void, JS_BeginInputFrame, (), {
    if (!Module.mossWebInput) JS_InitInput();
    JS_UpdateGamepads();
});

EM_JS(void, JS_EndInputFrame, (), {
    const s = Module.mossWebInput;
    if (!s) return;
    s.pressed.clear();
    s.released.clear();
    s.mousePressed.clear();
    s.mouseReleased.clear();
    s.wheelX = 0;
    s.wheelY = 0;
    if (Module.mossWebGamepads) {
        Module.mossWebGamepads.pressed.clear();
        Module.mossWebGamepads.released.clear();
    }
});

} // namespace

void SetCanvas(const char* selector) {
    if (g_webgl_context > 0) {
        emscripten_webgl_destroy_context(g_webgl_context);
        g_webgl_context = 0;
    }
    g_canvas_selector = selector && *selector ? selector : "#canvas";
    JS_SetCanvas(g_canvas_selector.c_str());
}
void SetCanvasSize(int width, int height) { JS_SetCanvasSize(width, height); }
void ResizeCanvasToDisplaySize() { JS_ResizeCanvasToDisplaySize(); }
int GetCanvasWidth() { return JS_GetCanvasWidth(); }
int GetCanvasHeight() { return JS_GetCanvasHeight(); }
float GetDevicePixelRatio() { return JS_GetDevicePixelRatio(); }
void RequestFullscreen() { JS_RequestFullscreen(); }
void ExitFullscreen() { JS_ExitFullscreen(); }
void RequestPointerLock() { JS_RequestPointerLock(); }
void ExitPointerLock() { JS_ExitPointerLock(); }
bool IsPointerLocked() { return JS_IsPointerLocked(); }
bool IsFullscreen() { return JS_IsFullscreen(); }
void SetCanvasCursor(const char* cursor) { JS_SetCanvasCursor(cursor); }
void SetCanvasFocus() { JS_SetCanvasFocus(); }
bool IsCanvasFocused() { return JS_IsFocused(); }
bool CreateWebGLContext() {
    if (g_webgl_context != 0) {
        return emscripten_webgl_make_context_current(g_webgl_context) == EMSCRIPTEN_RESULT_SUCCESS;
    }

    EmscriptenWebGLContextAttributes attr;
    emscripten_webgl_init_context_attributes(&attr);
    attr.alpha = EM_TRUE;
    attr.depth = EM_TRUE;
    attr.stencil = EM_TRUE;
    attr.antialias = EM_TRUE;
    attr.premultipliedAlpha = EM_TRUE;
    attr.preserveDrawingBuffer = EM_FALSE;
    attr.enableExtensionsByDefault = EM_TRUE;
    attr.majorVersion = 2;
    attr.minorVersion = 0;

    g_webgl_context = emscripten_webgl_create_context(g_canvas_selector.c_str(), &attr);
    if (g_webgl_context <= 0)
        return false;

    return emscripten_webgl_make_context_current(g_webgl_context) == EMSCRIPTEN_RESULT_SUCCESS;
}
void DestroyWebGLContext() {
    if (g_webgl_context > 0) {
        emscripten_webgl_destroy_context(g_webgl_context);
        g_webgl_context = 0;
    }
}
bool MakeWebGLContextCurrent() {
    return g_webgl_context > 0 &&
           emscripten_webgl_make_context_current(g_webgl_context) == EMSCRIPTEN_RESULT_SUCCESS;
}
void BeginFrame() { JS_BeginInputFrame(); }
void EndFrame() { JS_EndInputFrame(); }

} // namespace MossWeb

MOSS_API Moss_Window* Moss_CreateWindow(const char* title, int width, int height, Moss_Monitor*, Moss_Window*)
{
    if (width <= 0 || height <= 0)
        return nullptr;

    if (!g_window) {
        g_window = new Moss_Window;
        g_last_window = g_window;
    }

    g_window->title = title ? title : "Moss";
    g_window->width = width;
    g_window->height = height;
    g_window->closed = false;

    MossWeb::SetCanvasSize(width, height);
    MossWeb::SetCanvasFocus();
    MossWeb::BeginFrame();

    g_last_width = width;
    g_last_height = height;
    g_last_scale = 1.0f;
    return g_window;
}

MOSS_API void Moss_TerminateWindow(Moss_Window* window)
{
    if (!window)
        return;
    if (window == g_window) {
        g_window = nullptr;
        MossWeb::DestroyWebGLContext();
    }
    delete window;
}

MOSS_API bool Moss_CreateMessageBox(const char* title, const char* message, Moss_MessageBoxFlags, Moss_Window*)
{
    const std::string t = title ? title : "Moss";
    const std::string m = message ? message : "";
    EM_ASM({
        globalThis.alert(UTF8ToString($0) + "\n\n" + UTF8ToString($1));
    }, t.c_str(), m.c_str());
    return true;
}

MOSS_API bool Moss_ShouldWindowClose(Moss_Window* window)
{
    return !window || window->closed;
}

MOSS_API void Moss_PollEvents(void)
{
    if (!g_window)
        return;

    Moss_UpdateGamepads();

    const int width = MossWeb::GetCanvasWidth();
    const int height = MossWeb::GetCanvasHeight();
    const float scale = [] {
        return EM_ASM_DOUBLE({ return Math.max(1, globalThis.devicePixelRatio || 1); });
    }();
    const bool focus = EM_ASM_INT({
        const c = Module.mossWebCanvas || Module.canvas;
        return c && document.activeElement === c ? 1 : 0;
    }) != 0;

    if (width > 0 && height > 0 && (width != g_last_width || height != g_last_height)) {
        g_window->width = width;
        g_window->height = height;
        if (g_framebuffer_resize_callback)
            g_framebuffer_resize_callback(width, height);
        if (g_window_size_callback)
            g_window_size_callback(width, height);
        if (g_window_resize_callback)
            g_window_resize_callback(width, height);
        g_last_width = width;
        g_last_height = height;
    }

    if (focus != g_last_focus) {
        g_window->focused = focus;
        if (g_window_focus_callback)
            g_window_focus_callback(focus);
        g_last_focus = focus;
    }

    if (scale != g_last_scale) {
        if (g_window_content_scale_callback)
            g_window_content_scale_callback(scale, scale);
        g_last_scale = scale;
    }
}

MOSS_API int Moss_GetWindowWidth() { return g_window ? g_window->width : MossWeb::GetCanvasWidth(); }
MOSS_API int Moss_GetWindowHeight() { return g_window ? g_window->height : MossWeb::GetCanvasHeight(); }

MOSS_API void Moss_SetWindowTitle(Moss_Window* window, const char* title)
{
    if (window)
        window->title = title ? title : "Moss";
    if (title) {
        EM_ASM({ document.title = UTF8ToString($0); }, title);
    }
}

MOSS_API void Moss_SetWindowIcon(Moss_Window*, Moss_Image) {}

MOSS_API void Moss_CloseWindow(Moss_Window* window)
{
    if (window)
        window->closed = true;
}

MOSS_API Moss_Monitor* Moss_MonitorGetPrimary() { return &g_primary_monitor; }
MOSS_API Moss_Monitor* Moss_MonitorGetSecondary() { return &g_secondary_monitor; }
MOSS_API void Moss_MonitorGetPhysicalSize(Moss_Monitor*, int* width_mm, int* height_mm)
{
    if (width_mm) *width_mm = 0;
    if (height_mm) *height_mm = 0;
}
MOSS_API void Moss_MonitorGetContentScale(Moss_Monitor*, float* xscale, float* yscale)
{
    const float scale = EM_ASM_DOUBLE({ return Math.max(1, globalThis.devicePixelRatio || 1); });
    if (xscale) *xscale = scale;
    if (yscale) *yscale = scale;
}
MOSS_API void Moss_MonitorGetPosition(Moss_Monitor*, int* x, int* y)
{
    if (x) *x = 0;
    if (y) *y = 0;
}
MOSS_API const char* Moss_MonitorGetName(Moss_Monitor*) { return "primary"; }
MOSS_API void Moss_MonitorSetGammaRamp(Moss_Monitor*, const Moss_GammaRamp*) {}
MOSS_API Moss_GammaRamp* Moss_MonitorGetGammaRamp(Moss_Monitor*) { return nullptr; }
MOSS_API void Moss_MonitorSetGamma(Moss_Monitor*, float) {}

MOSS_API bool Moss_IsKeyPressed(Keyboard key) { return JS_KeyPressed(static_cast<int>(key)); }
MOSS_API bool Moss_IsReleased(Keyboard key) { return JS_KeyJustReleased(static_cast<int>(key)); }
MOSS_API bool Moss_IsKeyJustPressed(Keyboard key) { return JS_KeyJustPressed(static_cast<int>(key)); }
MOSS_API bool Moss_IsKeyJustReleased(Keyboard key) { return JS_KeyJustReleased(static_cast<int>(key)); }
MOSS_API Keyboard Moss_InputGetKey() {
    const int key = JS_InputGetKey();
    if (key < 0 || key > static_cast<int>(Keyboard::COUNT)) return Keyboard::KEY_0;
    return static_cast<Keyboard>(key);
}
MOSS_API bool Moss_IsMousePressed(Mouse button) { return JS_MousePressed(static_cast<int>(button)); }
MOSS_API bool Moss_IsMouseReleased(Mouse button) { return JS_MouseJustReleased(static_cast<int>(button)); }
MOSS_API bool Moss_IsMouseJustPressed(Mouse button) { return JS_MouseJustPressed(static_cast<int>(button)); }
MOSS_API bool Moss_IsMouseJustReleased(Mouse button) { return JS_MouseJustReleased(static_cast<int>(button)); }
MOSS_API Mouse Moss_InputGetMouseButton() {
    const int button = JS_InputGetMouseButton();
    if (button < 0 || button >= static_cast<int>(Mouse::COUNT)) return Mouse::LEFT;
    return static_cast<Mouse>(button);
}
MOSS_API void Moss_GetMousePosition(int* x, int* y) { if (x) *x = JS_MouseX(); if (y) *y = JS_MouseY(); }
MOSS_API void Moss_SetMousePosition(int x, int y) { JS_SetMousePosition(x, y); }
MOSS_API void Moss_SetMouseVisible(bool visible) { JS_SetMouseVisible(visible); }

MOSS_API int Moss_GetNumGamepads(void) { return JS_GamepadCount(); }
MOSS_API Moss_Gamepad* Moss_OpenGamepad(Moss_GamepadID id)
{
    if (!JS_GamepadConnected(static_cast<int>(id)))
        return nullptr;
    auto* gp = new Moss_Gamepad;
    gp->id = id;
    gp->index = static_cast<int>(id);
    return gp;
}
MOSS_API void Moss_CloseGamepad(Moss_Gamepad* gp) { delete gp; }
MOSS_API bool Moss_GamepadConnected(Moss_Gamepad* gp) { return gp && JS_GamepadConnected(gp->index); }
MOSS_API void Moss_UpdateGamepads(void) { JS_UpdateGamepads(); }

MOSS_API bool Moss_IsGamepadButtonPressed(Moss_Gamepad* gp, Moss_GamepadButton button)
{
    return gp && JS_GamepadButtonPressed(gp->index, static_cast<int>(button));
}
MOSS_API bool Moss_IsGamepadButtonJustPressed(Moss_Gamepad* gp, Moss_GamepadButton button) { return gp && JS_GamepadButtonJustPressed(gp->index, static_cast<int>(button)); }
MOSS_API bool Moss_IsGamepadButtonJustReleased(Moss_Gamepad* gp, Moss_GamepadButton button) { return gp && JS_GamepadButtonJustReleased(gp->index, static_cast<int>(button)); }
MOSS_API float Moss_GetGamepadAxis(Moss_Gamepad* gp, GamepadAxis axis)
{
    return gp ? JS_GamepadAxis(gp->index, static_cast<int>(axis)) : 0.0f;
}
void Moss_SetGamepadAxisDeadzone(GamepadAxis, float) {}
void Moss_SetGamepadAxisInverted(GamepadAxis, bool) {}
MOSS_API bool Moss_RumbleGamepad(Moss_Gamepad* gp, uint16_t low, uint16_t high, uint32_t duration_ms)
{
    return gp && JS_RumbleGamepad(gp->index, static_cast<float>(low) / 65535.0f, static_cast<float>(high) / 65535.0f, static_cast<int>(duration_ms));
}
MOSS_API bool Moss_RumbleGamepadTriggers(Moss_Gamepad*, uint16_t, uint16_t, uint32_t) { return false; }
MOSS_API bool Moss_SetGamepadLED(Moss_Gamepad*, uint8_t, uint8_t, uint8_t) { return false; }
MOSS_API const char* Moss_GetGamepadName(Moss_Gamepad* gp)
{
    static std::string name;
    if (!gp)
        return nullptr;
    const char* p = JS_GamepadName(gp->index);
    name = p ? p : "Web Gamepad";
    JS_Free(p);
    return name.c_str();
}
MOSS_API Moss_GamepadID Moss_GetGamepadID(Moss_Gamepad* gp) { return gp ? gp->id : 0; }
MOSS_API int Moss_GetGamepadPlayerIndex(Moss_Gamepad* gp) { return gp ? gp->index : -1; }
MOSS_API Moss_PowerState Moss_GetGamepadPowerInfo(Moss_Gamepad*, int* percent) { if (percent) *percent = -1; return Moss_PowerState::UNKNOWN; }
MOSS_API int Moss_GetNumGamepadTouchpads(Moss_Gamepad*) { return 0; }
MOSS_API int Moss_GetNumGamepadTouchpadFingers(Moss_Gamepad*) { return 0; }
MOSS_API bool Moss_GetGamepadTouchpadFinger(Moss_Gamepad*, int, int, bool*, float*, float*, float*) { return false; }
MOSS_API const char* Moss_GetGamepadMapping(Moss_Gamepad*) { return "standard"; }
MOSS_API bool Moss_SetGamepadMapping(Moss_Gamepad*, const char*) { return false; }
MOSS_API void Moss_ReloadGamepadMappings(void) {}
MOSS_API Moss_GamepadButton Moss_InputGetGamepadButton() { return Moss_GamepadButton::INVALID; }
MOSS_API GamepadAxis Moss_InputGetGamepadAxis() { return GamepadAxis::LEFT_X; }

MOSS_API Moss_PenDeviceType Moss_GetPenDeviceType(Moss_PenID) { return Moss_PenDeviceType::UNKNOWN; }
MOSS_API const char* Moss_GetTouchDeviceName(Moss_TouchID) { return "Web Touch"; }
MOSS_API Moss_TouchID* Moss_GetTouchDevices(int* count)
{
    static Moss_TouchID id = 0;
    const int device_count = JS_TouchDeviceCount();
    if (count) *count = device_count;
    return device_count ? &id : nullptr;
}
MOSS_API Moss_TouchDeviceType Moss_GetTouchDeviceType(Moss_TouchID)
{
    return JS_TouchDeviceCount() ? Moss_TouchDeviceType::DIRECT : Moss_TouchDeviceType::INVALID;
}
MOSS_API Moss_Finger** Moss_GetTouchFingers(Moss_TouchID, int* count)
{
    static std::vector<Moss_Finger> fingers;
    static std::vector<Moss_Finger*> pointers;
    const int n = JS_TouchCount();
    fingers.resize(static_cast<std::size_t>(std::max(0, n)));
    pointers.resize(fingers.size());
    for (int i = 0; i < n; ++i) {
        uint32_t id = 0;
        float x = 0.0f, y = 0.0f, pressure = 0.0f;
        JS_TouchInfo(i, &id, &x, &y, &pressure);
        fingers[static_cast<std::size_t>(i)] = Moss_Finger{ id, x, y, pressure };
        pointers[static_cast<std::size_t>(i)] = &fingers[static_cast<std::size_t>(i)];
    }
    if (count) *count = n;
    return pointers.empty() ? nullptr : pointers.data();
}

MOSS_API Moss_Haptic* Moss_OpenHaptic(Moss_HapticID id)
{
    auto* h = new Moss_Haptic;
    h->id = id;
    h->gamepad = Moss_OpenGamepad(static_cast<Moss_GamepadID>(id));
    return h;
}
MOSS_API void Moss_CloseHaptic(Moss_Haptic* haptic)
{
    if (!haptic) return;
    if (haptic->gamepad) Moss_CloseGamepad(haptic->gamepad);
    delete haptic;
}
MOSS_API Moss_HapticID Moss_CreateHapticEffect(Moss_Haptic*) { return 0; }
MOSS_API void Moss_DestroyHapticEffect(Moss_Haptic*) {}
MOSS_API bool Moss_GetHapticEffectStatus(Moss_Haptic*) { return false; }
MOSS_API uint32_t Moss_GetHapticFeatures(Moss_Haptic*) { return 0; }
MOSS_API Moss_Haptic* Moss_GetHapticFromID(Moss_Haptic* haptic) { return haptic; }
MOSS_API Moss_HapticID* Moss_GetHapticID(Moss_Haptic* haptic) { return haptic ? &haptic->id : nullptr; }
MOSS_API const char* Moss_GetHapticName(Moss_Haptic*) { return "Web Haptics"; }
MOSS_API const char* Moss_GetHapticNameForID(Moss_Haptic*) { return "Web Haptics"; }
MOSS_API Moss_HapticID* Moss_GetHaptics(Moss_Haptic*) { return nullptr; }
MOSS_API int Moss_GetMaxHapticEffects(Moss_Haptic*) { return 0; }
MOSS_API int Moss_GetMaxHapticEffectsPlaying(Moss_Haptic*) { return 0; }
MOSS_API int Moss_GetNumHapticAxes(Moss_Haptic*) { return 0; }
MOSS_API bool Moss_HapticEffectSupported(Moss_Haptic*) { return false; }
MOSS_API bool Moss_HapticRumbleSupported(Moss_Haptic* haptic) { return haptic && haptic->gamepad && JS_GamepadConnected(haptic->gamepad->index); }
MOSS_API bool Moss_InitHapticRumble(Moss_Haptic* haptic) { return Moss_HapticRumbleSupported(haptic); }
MOSS_API bool Moss_IsJoystickHaptic(Moss_GamepadAxis*) { return false; }
MOSS_API bool Moss_IsMouseHaptic(void) { return false; }
MOSS_API Moss_Haptic* Moss_OpenHapticFromJoystick(Moss_GamepadAxis*) { return nullptr; }
MOSS_API Moss_Haptic* Moss_OpenHapticFromMouse(void) { return nullptr; }
MOSS_API bool Moss_PauseHaptic(Moss_Haptic*) { return false; }
MOSS_API bool Moss_PlayHapticRumble(Moss_Haptic* haptic, float strength, uint32_t length) { return haptic && haptic->gamepad && JS_RumbleGamepad(haptic->gamepad->index, strength, strength, static_cast<int>(length)); }
MOSS_API bool Moss_ResumeHaptic(Moss_Haptic*) { return false; }
MOSS_API bool Moss_RunHapticEffect(Moss_Haptic*, uint32_t) { return false; }
MOSS_API bool Moss_SetHapticAutocenter(Moss_Haptic*, int) { return false; }
MOSS_API bool Moss_SetHapticGain(Moss_Haptic*, int) { return false; }
MOSS_API bool Moss_StopHapticEffect(Moss_Haptic*, Moss_HapticEffectID) { return false; }
MOSS_API bool Moss_StopHapticEffects(Moss_Haptic*) { return false; }
MOSS_API bool Moss_StopHapticRumble(Moss_Haptic*) { return false; }
MOSS_API bool Moss_UpdateHapticEffect(Moss_Haptic*, Moss_HapticEffectID, const Moss_HapticEffect*) { return false; }

MOSS_API int Moss_GetAvailableCPUCores(void) {
    return EM_ASM_INT({ return Math.max(1, globalThis.navigator?.hardwareConcurrency || 1); });
}
MOSS_API int Moss_GetCPUCacheLineSize(void) { return 64; }
MOSS_API int Moss_GetSystemRAM(void) {
    return EM_ASM_INT({ return globalThis.navigator?.deviceMemory ? Math.round(globalThis.navigator.deviceMemory * 1024) : 0; });
}
MOSS_API bool Moss_OpenURL(const char* url) { return JS_OpenURL(url); }
MOSS_API Moss_Locale* Moss_GetLocale()
{
    static bool initialized = false;
    if (!initialized) {
        const char* language = JS_Language();
        const char* country = JS_Country();
        g_locale_language = language ? language : "en";
        g_locale_country = country ? country : "";
        JS_Free(language);
        JS_Free(country);
        g_locale.country = g_locale_country.empty() ? nullptr : const_cast<char*>(g_locale_country.c_str());
        g_locale.language = g_locale_language.empty() ? nullptr : const_cast<char*>(g_locale_language.c_str());
        initialized = true;
    }
    return &g_locale;
}
MOSS_API Moss_PowerState Moss_GetPowerInfo(int* seconds, int* percent) { if (seconds) *seconds = -1; if (percent) *percent = -1; return Moss_PowerState::UNKNOWN; }
MOSS_API bool Moss_IsProcessRunningByName(const char*) { return false; }
MOSS_API void* Moss_LoadDynamicLibrary(const char*) { return nullptr; }
MOSS_API void* Moss_GetLibrarySymbol(void*, const char*) { return nullptr; }
MOSS_API void Moss_UnloadDynamicLibrary(void*) {}

MOSS_API void Moss_SetFramebufferResizeCallback(Moss_FramebufferResizeCallback callback) { g_framebuffer_resize_callback = callback; }
MOSS_API void Moss_SetWindowSizeCallback(Moss_WindowSizeCallback callback) { g_window_size_callback = callback; }
MOSS_API void Moss_SetWindowResizeCallback(Moss_WindowResizeCallback callback) { g_window_resize_callback = callback; }
MOSS_API void Moss_SetWindowPositionCallback(Moss_WindowPositionCallback callback) { g_window_position_callback = callback; }
MOSS_API void Moss_SetWindowFocusCallback(Moss_WindowFocusCallback callback) { g_window_focus_callback = callback; }
MOSS_API void Moss_SetWindowContentScaleCallback(Moss_WindowContentScaleCallback callback) { g_window_content_scale_callback = callback; }
MOSS_API void Moss_SetMonitorCallback(Moss_MonitorCallback callback) { g_monitor_callback = callback; }

MOSS_API Moss_CameraID* Moss_GetCameras(int* count) { if (count) *count = 0; return nullptr; }
MOSS_API const char* Moss_GetCameraName(Moss_CameraID) { return nullptr; }
MOSS_API Moss_CameraPosition Moss_GetCameraPosition(Moss_CameraID) { return Moss_CameraPosition::UNKNOWN; }
MOSS_API const char* Moss_GetCurrentCameraDriver(void) { return "web"; }
MOSS_API int Moss_GetNumCameraDrivers(void) { return 0; }
MOSS_API const Moss_CameraSpec* Moss_GetCameraSupportedFormats(Moss_CameraID, int* count) { if (count) *count = 0; return nullptr; }
MOSS_API Moss_Surface* Moss_AcquireCameraFrame(Moss_Capture*, uint64_t*) { return nullptr; }
MOSS_API void Moss_ReleaseCameraFrame(Moss_Capture*, Moss_Surface*) {}
MOSS_API bool Moss_GetCameraFormat(Moss_Capture*, Moss_CameraSpec*) { return false; }
MOSS_API Moss_CameraPermissionState Moss_GetCameraPermissionState(Moss_Capture*) { return Moss_CameraPermissionState::DENIED; }
MOSS_API Moss_PropertiesID Moss_GetCameraProperties(Moss_Capture*) { return 0; }
MOSS_API void Moss_CloseCamera(Moss_Capture*) {}
MOSS_API Moss_CameraID Moss_GetCameraID(Moss_Capture* camera) { return camera ? camera->id : 0; }
MOSS_API Moss_Capture* Moss_OpenCapture(Moss_CameraID id, const Moss_CameraSpec*) { auto* c = new Moss_Capture; c->id = id; return c; }

MOSS_API bool Moss_CopyFile(const char* src_path, const char* dst_path, bool overwrite)
{
    std::error_code ec;
    auto options = overwrite ? std::filesystem::copy_options::overwrite_existing : std::filesystem::copy_options::none;
    return std::filesystem::copy_file(src_path, dst_path, options, ec);
}
MOSS_API bool Moss_CreateDirectory(const char* path, bool recursive)
{
    std::error_code ec;
    return recursive ? std::filesystem::create_directories(path ? path : ".", ec) : std::filesystem::create_directory(path ? path : ".", ec);
}
MOSS_API bool Moss_RemovePath(const char* path, bool recursive)
{
    std::error_code ec;
    if (recursive) {
        std::filesystem::remove_all(path ? path : "", ec);
        return !ec;
    }
    return std::filesystem::remove(path ? path : "", ec);
}
MOSS_API bool Moss_RenamePath(const char* old_path, const char* new_path, bool overwrite)
{
    std::error_code ec;
    if (overwrite && std::filesystem::exists(new_path ? new_path : "", ec))
        std::filesystem::remove_all(new_path ? new_path : "", ec);
    std::filesystem::rename(old_path ? old_path : "", new_path ? new_path : "", ec);
    return !ec;
}
MOSS_API bool Moss_GetPathInfo(const char* path, Moss_PathInfo* out_info)
{
    if (!path || !out_info)
        return false;
    std::error_code ec;
    std::filesystem::directory_entry entry(path, ec);
    if (ec || !entry.exists(ec))
        return false;
    fill_path_info(entry, out_info);
    return true;
}
MOSS_API bool Moss_GetCurrentDirectory(char* out_path, int max_len)
{
    return write_string(out_path, max_len, std::filesystem::current_path().string());
}
MOSS_API bool Moss_GetBasePath(char* out_path, int max_len)
{
    return write_string(out_path, max_len, "/");
}
MOSS_API bool Moss_GetUserFolder(Moss_UserFolder, char* out_path, int max_len)
{
    return write_string(out_path, max_len, "/moss/user");
}
MOSS_API bool Moss_GetPrefPath(const char* org_name, const char* app_name, char* out_path, int max_len)
{
    const std::string root = std::string("/moss/user/") + (org_name ? org_name : "default") + "/" + (app_name ? app_name : "app");
    std::error_code ec;
    std::filesystem::create_directories(root, ec);
    return write_string(out_path, max_len, root + "/");
}
MOSS_API bool Moss_EnumerateDirectory(const char* path, bool recursive, Moss_DirectoryIterateFn callback, void* user_data)
{
    return enumerate_directory_impl(path, recursive, callback, user_data);
}
MOSS_API bool Moss_GlobDirectory(const char* pattern, Moss_DirectoryIterateFn callback, void* user_data)
{
    if (!pattern || !callback)
        return false;
    std::filesystem::path p(pattern);
    const std::filesystem::path parent = p.has_parent_path() ? p.parent_path() : std::filesystem::path(".");
    const std::string name_pattern = p.filename().string();
    std::error_code ec;
    for (std::filesystem::directory_iterator it(parent, ec), end; it != end; it.increment(ec)) {
        if (ec)
            return false;
        if (!wildcard_match(name_pattern.c_str(), it->path().filename().string(), false))
            continue;
        Moss_PathInfo info{};
        fill_path_info(*it, &info);
        if (!callback(&info, it->path().string().c_str(), user_data))
            break;
    }
    return !ec;
}

MOSS_API void Moss_ShowFileDialogWithProperties(Moss_DialogFileCallback, void*, Moss_Window*, const Moss_DialogFileFilter*, int, const char*) {}
MOSS_API void Moss_ShowOpenFolderDialog(Moss_DialogFileCallback, void*, Moss_Window*, const Moss_DialogFileFilter*, int, const char*, bool) {}
MOSS_API void Moss_ShowOpenFileDialog(Moss_DialogFileCallback, void*, Moss_Window*, const char*, bool) {}
MOSS_API void Moss_ShowSaveFileDialog(Moss_FileDialogType, Moss_DialogFileCallback, void*, Moss_PropertiesID) {}

MOSS_API bool Moss_CloseStorage(Moss_Storage* storage) { delete storage; return true; }
MOSS_API bool Moss_CopyStorageFile(Moss_Storage* storage, const char* oldpath, const char* newpath)
{
    return storage && Moss_CopyFile(normalize_storage_path(storage->root, oldpath).c_str(), normalize_storage_path(storage->root, newpath).c_str(), true);
}
MOSS_API bool Moss_CreateStorageDirectory(Moss_Storage* storage, const char* path)
{
    return storage && Moss_CreateDirectory(normalize_storage_path(storage->root, path).c_str(), true);
}
MOSS_API bool Moss_EnumerateStorageDirectory(Moss_Storage* storage, const char* path, Moss_EnumerateDirectoryCallback callback, void* userdata)
{
    return storage && callback && enumerate_directory_impl(normalize_storage_path(storage->root, path).c_str(), false, reinterpret_cast<Moss_DirectoryIterateFn>(callback), userdata);
}
MOSS_API bool Moss_GetStorageFileSize(Moss_Storage* storage, const char* path, uint64* length)
{
    if (!storage || !length)
        return false;
    std::error_code ec;
    const auto p = normalize_storage_path(storage->root, path);
    *length = std::filesystem::file_size(p, ec);
    return !ec;
}
MOSS_API uint64_t Moss_GetStorageSpaceRemaining(Moss_Storage*) { return std::numeric_limits<uint64_t>::max(); }
MOSS_API bool Moss_GetStoragePathInfo(Moss_Storage* storage, const char* path, Moss_PathInfo* info)
{
    return storage && Moss_GetPathInfo(normalize_storage_path(storage->root, path).c_str(), info);
}
MOSS_API char** Moss_GlobStorageDirectory(Moss_Storage* storage, const char* path, const char* pattern, Moss_GlobFlags flags, int* count)
{
    if (count) *count = 0;
    if (!storage || !pattern || !count)
        return nullptr;

    std::vector<std::string> matches;
    const std::filesystem::path root = normalize_storage_path(storage->root, path ? path : "");
    const bool recursive = (static_cast<uint32_t>(flags) & static_cast<uint32_t>(Moss_GlobFlags::Recursive)) != 0;
    const bool insensitive = (static_cast<uint32_t>(flags) & static_cast<uint32_t>(Moss_GlobFlags::CaseInsensitive)) != 0;
    const bool files_only = (static_cast<uint32_t>(flags) & static_cast<uint32_t>(Moss_GlobFlags::FilesOnly)) != 0;
    const bool dirs_only = (static_cast<uint32_t>(flags) & static_cast<uint32_t>(Moss_GlobFlags::DirectoriesOnly)) != 0;

    std::error_code ec;
    if (recursive) {
        for (std::filesystem::recursive_directory_iterator it(root, ec), end; it != end; it.increment(ec)) {
            if (ec) break;
            const bool is_dir = it->is_directory(ec);
            if ((files_only && is_dir) || (dirs_only && !is_dir)) continue;
            if (wildcard_match(pattern, it->path().filename().string(), insensitive))
                matches.push_back(it->path().string());
        }
    } else {
        for (std::filesystem::directory_iterator it(root, ec), end; it != end; it.increment(ec)) {
            if (ec) break;
            const bool is_dir = it->is_directory(ec);
            if ((files_only && is_dir) || (dirs_only && !is_dir)) continue;
            if (wildcard_match(pattern, it->path().filename().string(), insensitive))
                matches.push_back(it->path().string());
        }
    }

    if (matches.empty())
        return nullptr;

    char** result = static_cast<char**>(std::malloc(sizeof(char*) * matches.size()));
    if (!result)
        return nullptr;

    for (std::size_t i = 0; i < matches.size(); ++i) {
        result[i] = static_cast<char*>(std::malloc(matches[i].size() + 1));
        if (!result[i]) {
            for (std::size_t j = 0; j < i; ++j) std::free(result[j]);
            std::free(result);
            return nullptr;
        }
        std::memcpy(result[i], matches[i].c_str(), matches[i].size() + 1);
    }

    *count = static_cast<int>(matches.size());
    return result;
}
MOSS_API Moss_Storage* Moss_OpenFileStorage(const char* path)
{
    auto* storage = new Moss_Storage;
    storage->root = path ? path : "/";
    std::error_code ec;
    std::filesystem::create_directories(storage->root, ec);
    return storage;
}
MOSS_API Moss_Storage* Moss_OpenStorage(const Moss_Storage*, void*) { return nullptr; }
MOSS_API Moss_Storage* Moss_OpenTitleStorage(const char* override, Moss_PropertiesID)
{
    return Moss_OpenFileStorage(override && *override ? override : "/moss/title");
}
MOSS_API Moss_Storage* Moss_OpenUserStorage(const char* org, const char* app, Moss_PropertiesID)
{
    char path[1024]{};
    Moss_GetPrefPath(org, app, path, sizeof(path));
    return Moss_OpenFileStorage(path);
}
MOSS_API bool Moss_ReadStorageFile(Moss_Storage* storage, const char* path, void* destination, uint64 length)
{
    if (!storage || !path || !destination)
        return false;
    std::ifstream in(normalize_storage_path(storage->root, path), std::ios::binary);
    if (!in)
        return false;
    in.read(static_cast<char*>(destination), static_cast<std::streamsize>(length));
    return static_cast<uint64>(in.gcount()) == length;
}
MOSS_API bool Moss_RemoveStoragePath(Moss_Storage* storage, const char* path, bool recursive)
{
    return storage && Moss_RemovePath(normalize_storage_path(storage->root, path).c_str(), recursive);
}
MOSS_API bool Moss_RenameStoragePath(Moss_Storage* storage, const char* oldpath, const char* newpath)
{
    return storage && Moss_RenamePath(normalize_storage_path(storage->root, oldpath).c_str(), normalize_storage_path(storage->root, newpath).c_str(), true);
}
MOSS_API bool Moss_StorageReady(Moss_Storage* storage) { return storage != nullptr; }
MOSS_API bool Moss_WriteStorageFile(Moss_Storage* storage, const char* path, const void* source, uint64 length)
{
    if (!storage || !path || (!source && length != 0))
        return false;
    const std::string full = normalize_storage_path(storage->root, path);
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(full).parent_path(), ec);
    std::ofstream out(full, std::ios::binary | std::ios::trunc);
    if (!out)
        return false;
    out.write(static_cast<const char*>(source), static_cast<std::streamsize>(length));
    return static_cast<bool>(out);
}

MOSS_API void Moss_MakeContextCurrent(Moss_Window*) { (void)MossWeb::MakeWebGLContextCurrent(); }
MOSS_API void Moss_SwapBuffers() {}
MOSS_API void Moss_SwapBuffersInterval(int) {}
MOSS_API void* Moss_GetProcAddress(const char*) { return nullptr; }

MOSS_API Moss_Time Moss_GetTicks()
{
    return static_cast<Moss_Time>(now_ns());
}

MOSS_API double Moss_GetSeconds(Moss_Time time)
{
    return static_cast<double>(time) * 1.0e-9;
}

MOSS_API float Moss_GetMilliseconds(Moss_Time ticks)
{
    return static_cast<float>(static_cast<double>(Moss_GetTicks() - ticks) * 1.0e-6);
}

MOSS_API float Moss_GetMillisecondsAndReset(Moss_Time* ticks)
{
    if (!ticks)
        return 0.0f;
    const Moss_Time now = Moss_GetTicks();
    const float elapsed = static_cast<float>(static_cast<double>(now - *ticks) * 1.0e-6);
    *ticks = now;
    return elapsed;
}

MOSS_API void Moss_Yield(void) { emscripten_sleep(0); }
MOSS_API double Moss_Delay(double delay) { emscripten_sleep(static_cast<unsigned>(std::max(0.0, delay))); return delay; }

MOSS_API const char* Moss_LocalTime(void)
{
    static char buffer[32];
    EM_ASM({
        const d = new Date();
        const p = n => String(n).padStart(2, "0");
        const s = `${p(d.getHours())}:${p(d.getMinutes())}:${p(d.getSeconds())}`;
        stringToUTF8(s, $0, 32);
    }, buffer);
    return buffer;
}

MOSS_API const char* Moss_TimeStamp(void)
{
    static char buffer[32];
    EM_ASM({
        const d = new Date();
        const p = n => String(n).padStart(2, "0");
        const s = `${d.getFullYear()}-${p(d.getMonth()+1)}-${p(d.getDate())}_${p(d.getHours())}-${p(d.getMinutes())}-${p(d.getSeconds())}`;
        stringToUTF8(s, $0, 32);
    }, buffer);
    return buffer;
}

MOSS_API const char* Moss_TimeNow(void)
{
    static char buffer[32];
    EM_ASM({ stringToUTF8(String(Math.floor(Date.now() / 1000)), $0, 32); }, buffer);
    return buffer;
}

MOSS_API const char* Moss_CTimeNow(void)
{
    static char buffer[64];
    EM_ASM({ stringToUTF8(new Date().toString(), $0, 64); }, buffer);
    return buffer;
}

MOSS_API const char* Moss_TimeStampET(void) { return Moss_TimeStamp(); }
MOSS_API const char* Moss_TimeStampCT(void) { return Moss_TimeStamp(); }
MOSS_API const char* Moss_TimeStampMT(void) { return Moss_TimeStamp(); }
MOSS_API const char* Moss_TimeStampPT(void) { return Moss_TimeStamp(); }

MOSS_API const char* Moss_FormatTime(const char* fmt)
{
    return fmt ? fmt : "";
}

#endif // MOSS_PLATFORM_WASM
