#include <MossJS/MossJS.hpp>
#include <MossJS/Moss_Web.h>
#include <Moss/Moss_Platform.h>
#include <Moss/Moss_Renderer.h>

#include <emscripten/bind.h>
#include <emscripten/emscripten.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

emscripten::val& framebuffer_callback() { static emscripten::val v = emscripten::val::undefined(); return v; }
emscripten::val& window_size_callback() { static emscripten::val v = emscripten::val::undefined(); return v; }
emscripten::val& window_resize_callback() { static emscripten::val v = emscripten::val::undefined(); return v; }
emscripten::val& window_position_callback() { static emscripten::val v = emscripten::val::undefined(); return v; }
emscripten::val& window_focus_callback() { static emscripten::val v = emscripten::val::undefined(); return v; }
emscripten::val& content_scale_callback() { static emscripten::val v = emscripten::val::undefined(); return v; }
emscripten::val& monitor_callback() { static emscripten::val v = emscripten::val::undefined(); return v; }

void on_framebuffer_resize(int width, int height) { auto& f = framebuffer_callback(); if (!f.isUndefined()) f(width, height); }
void on_window_size(int width, int height) { auto& f = window_size_callback(); if (!f.isUndefined()) f(width, height); }
void on_window_resize(int width, int height) { auto& f = window_resize_callback(); if (!f.isUndefined()) f(width, height); }
void on_window_position(int x, int y) { auto& f = window_position_callback(); if (!f.isUndefined()) f(x, y); }
void on_window_focus(bool focused) { auto& f = window_focus_callback(); if (!f.isUndefined()) f(focused); }
void on_content_scale(float x, float y) { auto& f = content_scale_callback(); if (!f.isUndefined()) f(x, y); }
void on_monitor(const char* name, bool connected) { auto& f = monitor_callback(); if (!f.isUndefined()) f(name ? name : "", connected); }

void clear_callbacks()
{
    Moss_SetFramebufferResizeCallback(nullptr);
    Moss_SetWindowSizeCallback(nullptr);
    Moss_SetWindowResizeCallback(nullptr);
    Moss_SetWindowPositionCallback(nullptr);
    Moss_SetWindowFocusCallback(nullptr);
    Moss_SetWindowContentScaleCallback(nullptr);
    Moss_SetMonitorCallback(nullptr);
    framebuffer_callback() = emscripten::val::undefined();
    window_size_callback() = emscripten::val::undefined();
    window_resize_callback() = emscripten::val::undefined();
    window_position_callback() = emscripten::val::undefined();
    window_focus_callback() = emscripten::val::undefined();
    content_scale_callback() = emscripten::val::undefined();
    monitor_callback() = emscripten::val::undefined();
}

}

namespace MossJS {

Application::Application(emscripten::val options)
{
    std::string title = "Moss";

    if (!options.isUndefined() && !options.isNull()) {
        if (!options["width"].isUndefined())
            width_ = options["width"].as<int>();
        if (!options["height"].isUndefined())
            height_ = options["height"].as<int>();
        if (!options["title"].isUndefined())
            title = options["title"].as<std::string>();
        if (!options["canvas"].isUndefined())
            attachCanvas(options["canvas"]);
    }

    if (width_ <= 0 || height_ <= 0)
        throw std::invalid_argument("MossJS Application width and height must be positive");

    window_ = Moss_CreateWindow(title.c_str(), width_, height_, nullptr, nullptr);
    if (!window_)
        throw std::runtime_error("Moss_CreateWindow failed for the Web platform");
}

Application::~Application()
{
    clear_callbacks();
    close();
}

void Application::attachCanvas(emscripten::val canvas)
{
    if (canvas.isNull() || canvas.isUndefined())
        throw std::invalid_argument("MossJS Application requires a canvas");

    canvas_ = canvas;

    if (canvas_["id"].isUndefined() || canvas_["id"].as<std::string>().empty())
        canvas_.set("id", "moss");

    const std::string id = canvas_["id"].as<std::string>();
    MossWeb::SetCanvas((std::string("#") + id).c_str());
    MossWeb::SetCanvasSize(width_, height_);
    if (!MossWeb::CreateWebGLContext())
        throw std::runtime_error("MossJS could not create a WebGL2 context");
}

emscripten::val Application::canvas() const
{
    return canvas_;
}

void Application::resize()
{
    MossWeb::ResizeCanvasToDisplaySize();
    width_ = MossWeb::GetCanvasWidth();
    height_ = MossWeb::GetCanvasHeight();
}

void Application::beginFrame()
{
    Moss_PollEvents();
}

void Application::endFrame()
{
    MossWeb::EndFrame();
}

void Application::fullscreen() { MossWeb::RequestFullscreen(); }
void Application::exitFullscreen() { MossWeb::ExitFullscreen(); }
void Application::pointerLock() { MossWeb::RequestPointerLock(); }
void Application::exitPointerLock() { MossWeb::ExitPointerLock(); }
bool Application::isFullscreen() const { return MossWeb::IsFullscreen(); }
bool Application::isPointerLocked() const { return MossWeb::IsPointerLocked(); }
bool Application::isFocused() const { return MossWeb::IsCanvasFocused(); }
bool Application::shouldClose() const { return !window_ || Moss_ShouldWindowClose(window_); }

void Application::close()
{
    if (window_) {
        Moss_TerminateWindow(window_);
        window_ = nullptr;
    }
}

void Application::setTitle(const std::string& title)
{
    if (window_)
        Moss_SetWindowTitle(window_, title.c_str());
}

void Application::setCursor(const std::string& cursor)
{
    MossWeb::SetCanvasCursor(cursor.c_str());
}

void Application::focus()
{
    MossWeb::SetCanvasFocus();
}

void Application::on(const std::string& event, emscripten::val callback)
{
    if (!callback.isFunction())
        throw std::invalid_argument("MossJS Application.on requires a function");

    if (event == "framebufferResize") { framebuffer_callback() = callback; Moss_SetFramebufferResizeCallback(&on_framebuffer_resize); return; }
    if (event == "resize") { window_size_callback() = callback; Moss_SetWindowSizeCallback(&on_window_size); return; }
    if (event == "windowResize") { window_resize_callback() = callback; Moss_SetWindowResizeCallback(&on_window_resize); return; }
    if (event == "position") { window_position_callback() = callback; Moss_SetWindowPositionCallback(&on_window_position); return; }
    if (event == "focus") { window_focus_callback() = callback; Moss_SetWindowFocusCallback(&on_window_focus); return; }
    if (event == "contentScale") { content_scale_callback() = callback; Moss_SetWindowContentScaleCallback(&on_content_scale); return; }
    if (event == "monitor") { monitor_callback() = callback; Moss_SetMonitorCallback(&on_monitor); return; }

    throw std::invalid_argument("unknown MossJS application event: " + event);
}

void Application::off(const std::string& event)
{
    if (event == "framebufferResize") { framebuffer_callback() = emscripten::val::undefined(); Moss_SetFramebufferResizeCallback(nullptr); return; }
    if (event == "resize") { window_size_callback() = emscripten::val::undefined(); Moss_SetWindowSizeCallback(nullptr); return; }
    if (event == "windowResize") { window_resize_callback() = emscripten::val::undefined(); Moss_SetWindowResizeCallback(nullptr); return; }
    if (event == "position") { window_position_callback() = emscripten::val::undefined(); Moss_SetWindowPositionCallback(nullptr); return; }
    if (event == "focus") { window_focus_callback() = emscripten::val::undefined(); Moss_SetWindowFocusCallback(nullptr); return; }
    if (event == "contentScale") { content_scale_callback() = emscripten::val::undefined(); Moss_SetWindowContentScaleCallback(nullptr); return; }
    if (event == "monitor") { monitor_callback() = emscripten::val::undefined(); Moss_SetMonitorCallback(nullptr); return; }

    throw std::invalid_argument("unknown MossJS application event: " + event);
}

int Application::width() const { return width_; }
int Application::height() const { return height_; }
float Application::devicePixelRatio() const { return MossWeb::GetDevicePixelRatio(); }
Moss_Window* Application::nativeWindow() const { return window_; }

Gamepad::Gamepad(int index)
{
    if (index < 0)
        return;
    gamepad_ = Moss_OpenGamepad(static_cast<Moss_GamepadID>(index));
}

Gamepad::~Gamepad()
{
    if (gamepad_)
        Moss_CloseGamepad(gamepad_);
}

bool Gamepad::valid() const { return gamepad_ != nullptr; }
bool Gamepad::connected() const { return gamepad_ && Moss_GamepadConnected(gamepad_); }
int Gamepad::index() const { return gamepad_ ? Moss_GetGamepadPlayerIndex(gamepad_) : -1; }
Moss_GamepadID Gamepad::id() const { return gamepad_ ? Moss_GetGamepadID(gamepad_) : 0; }
std::string Gamepad::name() const { const char* p = gamepad_ ? Moss_GetGamepadName(gamepad_) : nullptr; return p ? p : ""; }
std::string Gamepad::mapping() const { const char* p = gamepad_ ? Moss_GetGamepadMapping(gamepad_) : nullptr; return p ? p : ""; }
bool Gamepad::buttonPressed(Moss_GamepadButton button) const { return gamepad_ && Moss_IsGamepadButtonPressed(gamepad_, button); }
bool Gamepad::buttonJustPressed(Moss_GamepadButton button) const { return gamepad_ && Moss_IsGamepadButtonJustPressed(gamepad_, button); }
bool Gamepad::buttonJustReleased(Moss_GamepadButton button) const { return gamepad_ && Moss_IsGamepadButtonJustReleased(gamepad_, button); }
float Gamepad::axis(GamepadAxis axisValue) const { return gamepad_ ? Moss_GetGamepadAxis(gamepad_, axisValue) : 0.0f; }
bool Gamepad::rumble(float low, float high, uint32_t durationMs) const
{
    if (!gamepad_)
        return false;
    low = std::max(0.0f, std::min(1.0f, low));
    high = std::max(0.0f, std::min(1.0f, high));
    return Moss_RumbleGamepad(
        gamepad_,
        static_cast<uint16_t>(low * 65535.0f),
        static_cast<uint16_t>(high * 65535.0f),
        durationMs
    );
}

Storage::Storage(const std::string& root)
    : storage_(Moss_OpenFileStorage(root.c_str()))
{
}

Storage::~Storage()
{
    close();
}

bool Storage::ready() const { return storage_ && Moss_StorageReady(storage_); }

void Storage::close()
{
    if (storage_) {
        Moss_CloseStorage(storage_);
        storage_ = nullptr;
    }
}

bool Storage::writeFile(const std::string& path, emscripten::val data)
{
    if (!storage_ || data.isNull() || data.isUndefined())
        return false;

    const uint64_t length = data["byteLength"].as<uint64_t>();
    if (length > std::numeric_limits<uint32_t>::max())
        return false;

    if (length == 0)
        return Moss_WriteStorageFile(storage_, path.c_str(), nullptr, 0);

    auto* bytes = static_cast<uint8_t*>(std::malloc(static_cast<size_t>(length)));
    if (!bytes)
        return false;

    emscripten::val heap = emscripten::val::module_property("HEAPU8");
    heap.call<void>("set", data, reinterpret_cast<uintptr_t>(bytes));

    const bool ok = Moss_WriteStorageFile(storage_, path.c_str(), bytes, length);
    std::free(bytes);
    return ok;
}

emscripten::val Storage::readFile(const std::string& path) const
{
    const auto null = emscripten::val::null();
    if (!storage_)
        return null;

    uint64_t length = 0;
    if (!Moss_GetStorageFileSize(storage_, path.c_str(), &length))
        return null;
    if (length > std::numeric_limits<uint32_t>::max())
        return null;

    const size_t size = static_cast<size_t>(length);
    auto* bytes = static_cast<uint8_t*>(std::malloc(size == 0 ? 1 : size));
    if (!bytes)
        return null;

    if (size != 0 && !Moss_ReadStorageFile(storage_, path.c_str(), bytes, length)) {
        std::free(bytes);
        return null;
    }

    emscripten::val result = emscripten::val::global("Uint8Array").new_(static_cast<unsigned>(size));
    if (size != 0) {
        emscripten::val heap = emscripten::val::module_property("HEAPU8");
        result.call<void>("set", heap.call<emscripten::val>("subarray", reinterpret_cast<uintptr_t>(bytes), reinterpret_cast<uintptr_t>(bytes) + size));
    }

    std::free(bytes);
    return result;
}

emscripten::val getPrimaryMonitor()
{
    Moss_Monitor* monitor = Moss_MonitorGetPrimary();
    emscripten::val result = emscripten::val::object();
    if (!monitor)
        return result;

    int x = 0, y = 0, widthMm = 0, heightMm = 0;
    float scaleX = 1.0f, scaleY = 1.0f;
    Moss_MonitorGetPosition(monitor, &x, &y);
    Moss_MonitorGetPhysicalSize(monitor, &widthMm, &heightMm);
    Moss_MonitorGetContentScale(monitor, &scaleX, &scaleY);

    result.set("name", Moss_MonitorGetName(monitor) ? Moss_MonitorGetName(monitor) : "");
    result.set("x", x);
    result.set("y", y);
    result.set("physicalWidth", widthMm);
    result.set("physicalHeight", heightMm);
    result.set("scaleX", scaleX);
    result.set("scaleY", scaleY);
    return result;
}

emscripten::val getMousePosition()
{
    int x = 0;
    int y = 0;
    Moss_GetMousePosition(&x, &y);
    emscripten::val result = emscripten::val::object();
    result.set("x", x);
    result.set("y", y);
    return result;
}

emscripten::val getLocale()
{
    Moss_Locale* locale = Moss_GetLocale();
    emscripten::val result = emscripten::val::object();
    result.set("country", locale && locale->country ? locale->country : "");
    result.set("language", locale && locale->language ? locale->language : "");
    return result;
}

}

EMSCRIPTEN_BINDINGS(MossJS)
{
    using namespace emscripten;

    enum_<Keyboard>("Keyboard")
        .value("KEY_0", Keyboard::KEY_0).value("KEY_1", Keyboard::KEY_1).value("KEY_2", Keyboard::KEY_2).value("KEY_3", Keyboard::KEY_3).value("KEY_4", Keyboard::KEY_4)
        .value("KEY_5", Keyboard::KEY_5).value("KEY_6", Keyboard::KEY_6).value("KEY_7", Keyboard::KEY_7).value("KEY_8", Keyboard::KEY_8).value("KEY_9", Keyboard::KEY_9)
        .value("KEY_A", Keyboard::KEY_A).value("KEY_B", Keyboard::KEY_B).value("KEY_C", Keyboard::KEY_C).value("KEY_D", Keyboard::KEY_D).value("KEY_E", Keyboard::KEY_E)
        .value("KEY_F", Keyboard::KEY_F).value("KEY_G", Keyboard::KEY_G).value("KEY_H", Keyboard::KEY_H).value("KEY_I", Keyboard::KEY_I).value("KEY_J", Keyboard::KEY_J)
        .value("KEY_K", Keyboard::KEY_K).value("KEY_L", Keyboard::KEY_L).value("KEY_M", Keyboard::KEY_M).value("KEY_N", Keyboard::KEY_N).value("KEY_O", Keyboard::KEY_O)
        .value("KEY_P", Keyboard::KEY_P).value("KEY_Q", Keyboard::KEY_Q).value("KEY_R", Keyboard::KEY_R).value("KEY_S", Keyboard::KEY_S).value("KEY_T", Keyboard::KEY_T)
        .value("KEY_U", Keyboard::KEY_U).value("KEY_V", Keyboard::KEY_V).value("KEY_W", Keyboard::KEY_W).value("KEY_X", Keyboard::KEY_X).value("KEY_Y", Keyboard::KEY_Y).value("KEY_Z", Keyboard::KEY_Z)
        .value("KEY_APOSTROPHE", Keyboard::KEY_APOSTROPHE).value("KEY_BACKSLASH", Keyboard::KEY_BACKSLASH).value("KEY_COMMA", Keyboard::KEY_COMMA).value("KEY_EQUAL", Keyboard::KEY_EQUAL)
        .value("KEY_GRAVE_ACCENT", Keyboard::KEY_GRAVE_ACCENT).value("KEY_LEFT_BRACKET", Keyboard::KEY_LEFT_BRACKET).value("KEY_MINUS", Keyboard::KEY_MINUS).value("KEY_PERIOD", Keyboard::KEY_PERIOD)
        .value("KEY_RIGHT_BRACKET", Keyboard::KEY_RIGHT_BRACKET).value("KEY_SEMICOLON", Keyboard::KEY_SEMICOLON).value("KEY_SLASH", Keyboard::KEY_SLASH).value("KEY_WORLD_1", Keyboard::KEY_WORLD_1).value("KEY_WORLD_2", Keyboard::KEY_WORLD_2)
        .value("KEY_BACKSPACE", Keyboard::KEY_BACKSPACE).value("KEY_DELETE", Keyboard::KEY_DELETE).value("KEY_END", Keyboard::KEY_END).value("KEY_ENTER", Keyboard::KEY_ENTER).value("KEY_ESCAPE", Keyboard::KEY_ESCAPE)
        .value("KEY_HOME", Keyboard::KEY_HOME).value("KEY_INSERT", Keyboard::KEY_INSERT).value("KEY_MENU", Keyboard::KEY_MENU).value("KEY_PAGE_DOWN", Keyboard::KEY_PAGE_DOWN).value("KEY_PAGE_UP", Keyboard::KEY_PAGE_UP)
        .value("KEY_PAUSE", Keyboard::KEY_PAUSE).value("KEY_SPACE", Keyboard::KEY_SPACE).value("KEY_TAB", Keyboard::KEY_TAB).value("KEY_CAPS_LOCK", Keyboard::KEY_CAPS_LOCK).value("KEY_NUM_LOCK", Keyboard::KEY_NUM_LOCK).value("KEY_SCROLL_LOCK", Keyboard::KEY_SCROLL_LOCK)
        .value("KEY_F1", Keyboard::KEY_F1).value("KEY_F2", Keyboard::KEY_F2).value("KEY_F3", Keyboard::KEY_F3).value("KEY_F4", Keyboard::KEY_F4).value("KEY_F5", Keyboard::KEY_F5).value("KEY_F6", Keyboard::KEY_F6)
        .value("KEY_F7", Keyboard::KEY_F7).value("KEY_F8", Keyboard::KEY_F8).value("KEY_F9", Keyboard::KEY_F9).value("KEY_F10", Keyboard::KEY_F10).value("KEY_F11", Keyboard::KEY_F11).value("KEY_F12", Keyboard::KEY_F12)
        .value("KEY_F13", Keyboard::KEY_F13).value("KEY_F14", Keyboard::KEY_F14).value("KEY_F15", Keyboard::KEY_F15).value("KEY_F16", Keyboard::KEY_F16).value("KEY_F17", Keyboard::KEY_F17).value("KEY_F18", Keyboard::KEY_F18)
        .value("KEY_F19", Keyboard::KEY_F19).value("KEY_F20", Keyboard::KEY_F20).value("KEY_F21", Keyboard::KEY_F21).value("KEY_F22", Keyboard::KEY_F22).value("KEY_F23", Keyboard::KEY_F23).value("KEY_F24", Keyboard::KEY_F24)
        .value("KEY_LEFT_ALT", Keyboard::KEY_LEFT_ALT).value("KEY_LEFT_CONTROL", Keyboard::KEY_LEFT_CONTROL).value("KEY_LEFT_SHIFT", Keyboard::KEY_LEFT_SHIFT).value("KEY_LEFT_SUPER", Keyboard::KEY_LEFT_SUPER)
        .value("KEY_PRINT_SCREEN", Keyboard::KEY_PRINT_SCREEN).value("KEY_RIGHT_ALT", Keyboard::KEY_RIGHT_ALT).value("KEY_RIGHT_CONTROL", Keyboard::KEY_RIGHT_CONTROL).value("KEY_RIGHT_SHIFT", Keyboard::KEY_RIGHT_SHIFT).value("KEY_RIGHT_SUPER", Keyboard::KEY_RIGHT_SUPER)
        .value("KEY_DOWN", Keyboard::KEY_DOWN).value("KEY_LEFT", Keyboard::KEY_LEFT).value("KEY_RIGHT", Keyboard::KEY_RIGHT).value("KEY_UP", Keyboard::KEY_UP)
        .value("KEY_KP_0", Keyboard::KEY_KP_0).value("KEY_KP_1", Keyboard::KEY_KP_1).value("KEY_KP_2", Keyboard::KEY_KP_2).value("KEY_KP_3", Keyboard::KEY_KP_3).value("KEY_KP_4", Keyboard::KEY_KP_4)
        .value("KEY_KP_5", Keyboard::KEY_KP_5).value("KEY_KP_6", Keyboard::KEY_KP_6).value("KEY_KP_7", Keyboard::KEY_KP_7).value("KEY_KP_8", Keyboard::KEY_KP_8).value("KEY_KP_9", Keyboard::KEY_KP_9)
        .value("KEY_KP_ADD", Keyboard::KEY_KP_ADD).value("KEY_KP_DECIMAL", Keyboard::KEY_KP_DECIMAL).value("KEY_KP_DIVIDE", Keyboard::KEY_KP_DIVIDE).value("KEY_KP_ENTER", Keyboard::KEY_KP_ENTER)
        .value("KEY_KP_EQUAL", Keyboard::KEY_KP_EQUAL).value("KEY_KP_MULTIPLY", Keyboard::KEY_KP_MULTIPLY).value("KEY_KP_SUBTRACT", Keyboard::KEY_KP_SUBTRACT);

    enum_<Mouse>("Mouse")
        .value("LEFT", Mouse::LEFT).value("RIGHT", Mouse::RIGHT).value("MIDDLE", Mouse::MIDDLE)
        .value("BUTTON_4", Mouse::BUTTON_4).value("BUTTON_5", Mouse::BUTTON_5).value("BUTTON_6", Mouse::BUTTON_6)
        .value("BUTTON_7", Mouse::BUTTON_7).value("BUTTON_8", Mouse::BUTTON_8);

    enum_<Moss_GamepadButton>("GamepadButton")
        .value("INVALID", Moss_GamepadButton::INVALID)
        .value("SOUTH", Moss_GamepadButton::SOUTH).value("EAST", Moss_GamepadButton::EAST).value("WEST", Moss_GamepadButton::WEST).value("NORTH", Moss_GamepadButton::NORTH)
        .value("BACK", Moss_GamepadButton::BACK).value("GUIDE", Moss_GamepadButton::GUIDE).value("START", Moss_GamepadButton::START)
        .value("LEFT_STICK", Moss_GamepadButton::LEFT_STICK).value("RIGHT_STICK", Moss_GamepadButton::RIGHT_STICK)
        .value("LEFT_SHOULDER", Moss_GamepadButton::LEFT_SHOULDER).value("RIGHT_SHOULDER", Moss_GamepadButton::RIGHT_SHOULDER)
        .value("DPAD_UP", Moss_GamepadButton::DPAD_UP).value("DPAD_DOWN", Moss_GamepadButton::DPAD_DOWN).value("DPAD_LEFT", Moss_GamepadButton::DPAD_LEFT).value("DPAD_RIGHT", Moss_GamepadButton::DPAD_RIGHT)
        .value("MISC1", Moss_GamepadButton::MISC1).value("RIGHT_PADDLE1", Moss_GamepadButton::RIGHT_PADDLE1).value("LEFT_PADDLE1", Moss_GamepadButton::LEFT_PADDLE1)
        .value("RIGHT_PADDLE2", Moss_GamepadButton::RIGHT_PADDLE2).value("LEFT_PADDLE2", Moss_GamepadButton::LEFT_PADDLE2).value("TOUCHPAD", Moss_GamepadButton::TOUCHPAD)
        .value("MISC2", Moss_GamepadButton::MISC2).value("MISC3", Moss_GamepadButton::MISC3).value("MISC4", Moss_GamepadButton::MISC4)
        .value("MISC5", Moss_GamepadButton::MISC5).value("MISC6", Moss_GamepadButton::MISC6);

    enum_<GamepadAxis>("GamepadAxis")
        .value("LEFT_X", GamepadAxis::LEFT_X).value("LEFT_Y", GamepadAxis::LEFT_Y).value("RIGHT_X", GamepadAxis::RIGHT_X).value("RIGHT_Y", GamepadAxis::RIGHT_Y)
        .value("LEFT_TRIGGER", GamepadAxis::LEFT_TRIGGER).value("RIGHT_TRIGGER", GamepadAxis::RIGHT_TRIGGER)
        .value("TOUCHPAD_X", GamepadAxis::TOUCHPAD_X).value("TOUCHPAD_Y", GamepadAxis::TOUCHPAD_Y)
        .value("GYRO_X", GamepadAxis::GYRO_X).value("GYRO_Y", GamepadAxis::GYRO_Y).value("GYRO_Z", GamepadAxis::GYRO_Z);

    class_<MossJS::Application>("Application")
        .constructor<val>()
        .function("attachCanvas", &MossJS::Application::attachCanvas)
        .function("canvas", &MossJS::Application::canvas)
        .function("resize", &MossJS::Application::resize)
        .function("beginFrame", &MossJS::Application::beginFrame)
        .function("endFrame", &MossJS::Application::endFrame)
        .function("fullscreen", &MossJS::Application::fullscreen)
        .function("exitFullscreen", &MossJS::Application::exitFullscreen)
        .function("pointerLock", &MossJS::Application::pointerLock)
        .function("exitPointerLock", &MossJS::Application::exitPointerLock)
        .function("isFullscreen", &MossJS::Application::isFullscreen)
        .function("isPointerLocked", &MossJS::Application::isPointerLocked)
        .function("isFocused", &MossJS::Application::isFocused)
        .function("shouldClose", &MossJS::Application::shouldClose)
        .function("close", &MossJS::Application::close)
        .function("setTitle", &MossJS::Application::setTitle)
        .function("setCursor", &MossJS::Application::setCursor)
        .function("focus", &MossJS::Application::focus)
        .function("on", &MossJS::Application::on)
        .function("off", &MossJS::Application::off)
        .function("width", &MossJS::Application::width)
        .function("height", &MossJS::Application::height)
        .function("devicePixelRatio", &MossJS::Application::devicePixelRatio);

    class_<MossJS::Gamepad>("Gamepad")
        .constructor<int>()
        .function("valid", &MossJS::Gamepad::valid)
        .function("connected", &MossJS::Gamepad::connected)
        .function("index", &MossJS::Gamepad::index)
        .function("id", &MossJS::Gamepad::id)
        .function("name", &MossJS::Gamepad::name)
        .function("mapping", &MossJS::Gamepad::mapping)
        .function("buttonPressed", &MossJS::Gamepad::buttonPressed)
        .function("buttonJustPressed", &MossJS::Gamepad::buttonJustPressed)
        .function("buttonJustReleased", &MossJS::Gamepad::buttonJustReleased)
        .function("axis", &MossJS::Gamepad::axis)
        .function("rumble", &MossJS::Gamepad::rumble);

    class_<MossJS::Storage>("StorageHandle")
        .constructor<std::string>()
        .function("ready", &MossJS::Storage::ready)
        .function("close", &MossJS::Storage::close)
        .function("writeFile", &MossJS::Storage::writeFile)
        .function("readFile", &MossJS::Storage::readFile);

    function("getTicks", []() -> int64_t { return Moss_GetTicks(); });
    function("getSeconds", [](int64_t time) -> double { return Moss_GetSeconds(time); });
    function("getMilliseconds", [](int64_t ticks) -> float { return Moss_GetMilliseconds(ticks); });
    function("getDeltaMilliseconds", []() -> float {
        static Moss_Time last = Moss_GetTicks();
        return Moss_GetMillisecondsAndReset(&last);
    });
    function("getWindowWidth", []() -> int { return Moss_GetWindowWidth(); });
    function("getWindowHeight", []() -> int { return Moss_GetWindowHeight(); });
    function("getDevicePixelRatio", []() -> float { return MossWeb::GetDevicePixelRatio(); });
    function("pollEvents", &Moss_PollEvents);

    function("isKeyPressed", &Moss_IsKeyPressed);
    function("isKeyReleased", &Moss_IsReleased);
    function("isKeyJustPressed", &Moss_IsKeyJustPressed);
    function("isKeyJustReleased", &Moss_IsKeyJustReleased);
    function("isMousePressed", &Moss_IsMousePressed);
    function("isMouseReleased", &Moss_IsMouseReleased);
    function("isMouseJustPressed", &Moss_IsMouseJustPressed);
    function("isMouseJustReleased", &Moss_IsMouseJustReleased);
    function("getMousePosition", &MossJS::getMousePosition);
    function("setMousePosition", &Moss_SetMousePosition);
    function("setMouseVisible", &Moss_SetMouseVisible);
    function("gamepadCount", &Moss_GetNumGamepads);
    function("updateGamepads", &Moss_UpdateGamepads);
    function("cpuCores", &Moss_GetAvailableCPUCores);
    function("cpuCacheLineSize", &Moss_GetCPUCacheLineSize);
    function("systemRAM", &Moss_GetSystemRAM);
    function("openURL", &Moss_OpenURL);
    function("locale", &MossJS::getLocale);
    function("primaryMonitor", &MossJS::getPrimaryMonitor);
    function("localTime", []() -> std::string { return Moss_LocalTime(); });
    function("timeStamp", []() -> std::string { return Moss_TimeStamp(); });
    function("timeNow", []() -> std::string { return Moss_TimeNow(); });
    function("cTimeNow", []() -> std::string { return Moss_CTimeNow(); });
}

