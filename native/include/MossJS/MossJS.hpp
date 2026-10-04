#pragma once

#include <Moss/Moss_Platform.h>

#include <emscripten/val.h>

struct Moss_Renderer;

#include <string>

namespace MossJS {

class Application {
public:
    explicit Application(emscripten::val options);
    ~Application();

    void attachCanvas(emscripten::val canvas);
    emscripten::val canvas() const;

    void resize();
    void beginFrame();
    void endFrame();

    void fullscreen();
    void exitFullscreen();
    void pointerLock();
    void exitPointerLock();

    bool isFullscreen() const;
    bool isPointerLocked() const;
    bool isFocused() const;
    bool shouldClose() const;

    void close();
    void setTitle(const std::string& title);
    void setCursor(const std::string& cursor);
    void focus();
    void on(const std::string& event, emscripten::val callback);
    void off(const std::string& event);

    int width() const;
    int height() const;
    float devicePixelRatio() const;
    Moss_Window* nativeWindow() const;

private:
    emscripten::val canvas_ = emscripten::val::null();
    Moss_Window* window_ = nullptr;
    int width_ = 1280;
    int height_ = 720;
};

class Gamepad {
public:
    explicit Gamepad(int index);
    ~Gamepad();

    bool valid() const;
    bool connected() const;
    int index() const;
    Moss_GamepadID id() const;
    std::string name() const;
    std::string mapping() const;
    bool buttonPressed(Moss_GamepadButton button) const;
    bool buttonJustPressed(Moss_GamepadButton button) const;
    bool buttonJustReleased(Moss_GamepadButton button) const;
    float axis(GamepadAxis axis) const;
    bool rumble(float low, float high, uint32_t durationMs) const;

private:
    Moss_Gamepad* gamepad_ = nullptr;
};

class Storage {
public:
    explicit Storage(const std::string& root);
    ~Storage();

    bool ready() const;
    void close();
    bool writeFile(const std::string& path, emscripten::val data);
    emscripten::val readFile(const std::string& path) const;

private:
    Moss_Storage* storage_ = nullptr;
};

emscripten::val getMousePosition();
emscripten::val getLocale();

}
