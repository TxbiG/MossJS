#include <MossJS/MossJS_Subsystems.hpp>
#include <Moss/Moss_Renderer.h>

#include <emscripten/bind.h>

namespace MossJS {

NativeRenderer::NativeRenderer(Application& application)
{
    Moss_Window* window = application.nativeWindow();
    if (window)
        renderer_ = Moss_CreateRenderer(window);
}

NativeRenderer::~NativeRenderer()
{
    destroy();
}

bool NativeRenderer::valid() const
{
    return renderer_ != nullptr;
}

void NativeRenderer::beginFrame()
{
    if (renderer_)
        Moss_RendererBeginFrame(renderer_);
}

void NativeRenderer::endFrame()
{
    if (renderer_)
        Moss_RendererEndFrame(renderer_);
}

void NativeRenderer::destroy()
{
    if (renderer_) {
        Moss_RendererDestroy(renderer_);
        renderer_ = nullptr;
    }
}

SubsystemCapabilities getSubsystemCapabilities()
{
    return {};
}

}

EMSCRIPTEN_BINDINGS(MossJS_Subsystems)
{
    using namespace emscripten;

    value_object<MossJS::SubsystemCapabilities>("SubsystemCapabilities")
        .field("audio", &MossJS::SubsystemCapabilities::audio)
        .field("physics", &MossJS::SubsystemCapabilities::physics)
        .field("xr", &MossJS::SubsystemCapabilities::xr)
        .field("gpu", &MossJS::SubsystemCapabilities::gpu)
        .field("gui", &MossJS::SubsystemCapabilities::gui)
        .field("renderer", &MossJS::SubsystemCapabilities::renderer)
        .field("navigation", &MossJS::SubsystemCapabilities::navigation);

    class_<MossJS::NativeRenderer>("NativeRenderer")
        .constructor<MossJS::Application&>()
        .function("valid", &MossJS::NativeRenderer::valid)
        .function("beginFrame", &MossJS::NativeRenderer::beginFrame)
        .function("endFrame", &MossJS::NativeRenderer::endFrame)
        .function("destroy", &MossJS::NativeRenderer::destroy);

    function("subsystemCapabilities", &MossJS::getSubsystemCapabilities);
}
