#pragma once

#include <MossJS/MossJS.hpp>

#include <cstdint>
#include <string>

namespace MossJS {

class NativeRenderer {
public:
    explicit NativeRenderer(Application& application);
    ~NativeRenderer();

    bool valid() const;
    void beginFrame();
    void endFrame();
    void destroy();

private:
    Moss_Renderer* renderer_ = nullptr;
};

struct SubsystemCapabilities {
    bool audio = true;
    bool physics = true;
    bool xr = true;
    bool gpu = true;
    bool gui = true;
    bool renderer = true;
    bool navigation = false;
};

SubsystemCapabilities getSubsystemCapabilities();

}
