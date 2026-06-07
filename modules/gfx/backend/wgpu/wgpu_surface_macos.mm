#include "wgpu_surface.hpp"

#include "../../include/woki/window/window.hpp"

#if defined(__APPLE__) && !defined(__EMSCRIPTEN__)

#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <Foundation/Foundation.h>
#include <QuartzCore/CAMetalLayer.h>

namespace woki::api::wgpu {

WGPUSurface CreateNativeSurfaceCocoa(WGPUInstance instance, Window* window) {
    if (instance == nullptr || window == nullptr) {
        return nullptr;
    }

    auto* glfw_window = static_cast<GLFWwindow*>(window->GetNativeHandle());
    if (glfw_window == nullptr) {
        return nullptr;
    }

    NSWindow* ns_window = glfwGetCocoaWindow(glfw_window);
    if (ns_window == nullptr) {
        return nullptr;
    }

    id metal_layer = [CAMetalLayer layer];
    [ns_window.contentView setWantsLayer:YES];
    [ns_window.contentView setLayer:metal_layer];

    WGPUSurfaceDescriptor descriptor = WGPU_SURFACE_DESCRIPTOR_INIT;
    WGPUSurfaceSourceMetalLayer source = WGPU_SURFACE_SOURCE_METAL_LAYER_INIT;
    source.layer = metal_layer;
    descriptor.nextInChain = &source.chain;
    return wgpuInstanceCreateSurface(instance, &descriptor);
}

} // namespace woki::api::wgpu

#endif
