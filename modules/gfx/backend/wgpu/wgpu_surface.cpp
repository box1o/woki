#include "wgpu_surface.hpp"

#include "common.hpp"
#include "wgpu_adapter.hpp"

#include "../../include/woki/api/device.hpp"
#include "../../include/woki/window/window.hpp"

#include <GLFW/glfw3.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#define GLFW_EXPOSE_NATIVE_EMSCRIPTEN
#ifndef GLFW_PLATFORM_EMSCRIPTEN
#define GLFW_PLATFORM_EMSCRIPTEN 0
#endif
#else
#if defined(_WIN32) || defined(_WIN64)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__APPLE__) || defined(__MACH__)
#define GLFW_EXPOSE_NATIVE_COCOA
#elif defined(__linux__)
#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_WAYLAND
#endif
#include <GLFW/glfw3native.h>
#endif

#ifdef None
#undef None
#endif
#ifdef Always
#undef Always
#endif
#ifdef Success
#undef Success
#endif
#ifdef Bool
#undef Bool
#endif
#ifdef Status
#undef Status
#endif
#ifdef True
#undef True
#endif
#ifdef False
#undef False
#endif

namespace woki::api::wgpu {

WgpuSurfaceImpl::WgpuSurfaceImpl(WGPUInstance instance, WGPUSurface surface, Window* window)
    : instance_(instance)
    , surface_(surface)
    , window_(window) {
    WOKI_ASSERT(instance_ != nullptr);
    WOKI_ASSERT(surface_ != nullptr);
}

WgpuSurfaceImpl::~WgpuSurfaceImpl() {
    ReleaseCurrentTexture();

    if (surface_ != nullptr) {
        wgpuSurfaceRelease(surface_);
        surface_ = nullptr;
    }

    if (instance_ != nullptr) {
        wgpuInstanceRelease(instance_);
        instance_ = nullptr;
    }
}

SurfaceCapabilities WgpuSurfaceImpl::GetCapabilities(const Adapter& adapter) const {
    SurfaceCapabilities capabilities{};
    if (surface_ == nullptr) {
        return capabilities;
    }

    const auto* wgpu_adapter = dynamic_cast<const WgpuAdapterImpl*>(&adapter);
    if (wgpu_adapter == nullptr) {
        return capabilities;
    }

    WGPUSurfaceCapabilities native_capabilities = WGPU_SURFACE_CAPABILITIES_INIT;
    if (wgpuSurfaceGetCapabilities(surface_, wgpu_adapter->GetWgpuHandle(), &native_capabilities)
        != WGPUStatus_Success) {
        return capabilities;
    }

    capabilities.usages = detail::FromWgpuTextureUsage(native_capabilities.usages);
    capabilities.formats.reserve(native_capabilities.formatCount);
    for (size_t i = 0; i < native_capabilities.formatCount; ++i) {
        capabilities.formats.push_back(detail::FromWgpuTextureFormat(native_capabilities.formats[i]));
    }

    capabilities.present_modes.reserve(native_capabilities.presentModeCount);
    for (size_t i = 0; i < native_capabilities.presentModeCount; ++i) {
        capabilities.present_modes.push_back(detail::FromWgpuPresentMode(native_capabilities.presentModes[i]));
    }

    capabilities.alpha_modes.reserve(native_capabilities.alphaModeCount);
    for (size_t i = 0; i < native_capabilities.alphaModeCount; ++i) {
        capabilities.alpha_modes.push_back(
            detail::FromWgpuCompositeAlphaMode(native_capabilities.alphaModes[i]));
    }

    wgpuSurfaceCapabilitiesFreeMembers(native_capabilities);
    return capabilities;
}

SurfaceTexture WgpuSurfaceImpl::GetCurrentTexture() {
    SurfaceTexture result{};
    if (surface_ == nullptr) {
        return result;
    }

    ReleaseCurrentTexture();

    WGPUSurfaceTexture native_texture = WGPU_SURFACE_TEXTURE_INIT;
    wgpuSurfaceGetCurrentTexture(surface_, &native_texture);
    result.status = detail::FromWgpuSurfaceTextureStatus(native_texture.status);
    result.suboptimal =
        native_texture.status == WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal;

    if (native_texture.texture == nullptr) {
        return result;
    }

    current_texture_ = native_texture.texture;
    current_view_ = wgpuTextureCreateView(current_texture_, nullptr);
    if (current_view_ == nullptr) {
        wgpuTextureRelease(current_texture_);
        current_texture_ = nullptr;
        result.status = SurfaceGetCurrentTextureStatus::kError;
        return result;
    }

    result.handles.instance = instance_;
    result.handles.surface = surface_;
    result.handles.resource = static_cast<void*>(current_view_);
    return result;
}

NativeHandles WgpuSurfaceImpl::GetNativeHandles() const noexcept {
    NativeHandles handles{};
    handles.instance = instance_;
    handles.surface = surface_;
    handles.resource = static_cast<void*>(current_view_);
    return handles;
}

void WgpuSurfaceImpl::Configure(const SurfaceConfiguration& config) {
    if (surface_ == nullptr || config.device == nullptr) {
        return;
    }

    WOKI_ASSERT(config.width > 0);
    WOKI_ASSERT(config.height > 0);

    const auto handles = config.device->GetNativeHandles();
    if (handles.device == nullptr) {
        return;
    }

    std::vector<WGPUTextureFormat> view_formats;
    view_formats.reserve(config.view_formats.size());
    for (const auto format : config.view_formats) {
        view_formats.push_back(detail::ToWgpuTextureFormat(format));
    }

    WGPUSurfaceConfiguration native_config = WGPU_SURFACE_CONFIGURATION_INIT;
    native_config.device = static_cast<WGPUDevice>(handles.device);
    native_config.format = detail::ToWgpuTextureFormat(config.format);
    native_config.usage = detail::ToWgpuTextureUsage(config.usage);
    native_config.width = config.width;
    native_config.height = config.height;
    native_config.viewFormatCount = view_formats.size();
    native_config.viewFormats = view_formats.empty() ? nullptr : view_formats.data();
    native_config.alphaMode = detail::ToWgpuCompositeAlphaMode(config.alpha_mode);
    native_config.presentMode = detail::ToWgpuPresentMode(config.present_mode);
    wgpuSurfaceConfigure(surface_, &native_config);

    current_config_ = config;
}

void WgpuSurfaceImpl::Unconfigure() {
    if (surface_ != nullptr) {
        ReleaseCurrentTexture();
        wgpuSurfaceUnconfigure(surface_);
    }
}

Status WgpuSurfaceImpl::Present() {
    if (surface_ == nullptr) {
        return Status::kError;
    }

#ifdef __EMSCRIPTEN__
    // The browser presents the current surface texture as part of the RAF-driven frame loop.
    ReleaseCurrentTexture();
    return Status::kSuccess;
#else
    const auto status = wgpuSurfacePresent(surface_);
    ReleaseCurrentTexture();
    return status == WGPUStatus_Success ? Status::kSuccess : Status::kError;
#endif
}

void WgpuSurfaceImpl::SetLabel(std::string_view label) {
    if (surface_ != nullptr) {
        wgpuSurfaceSetLabel(surface_, detail::ToWgpuStringView(label));
    }
}

WGPUSurface WgpuSurfaceImpl::GetWgpuHandle() const noexcept { return surface_; }

Window* WgpuSurfaceImpl::GetWindow() const noexcept { return window_; }

void WgpuSurfaceImpl::ReleaseCurrentTexture() noexcept {
    if (current_view_ != nullptr) {
        wgpuTextureViewRelease(current_view_);
        current_view_ = nullptr;
    }

    if (current_texture_ != nullptr) {
        wgpuTextureRelease(current_texture_);
        current_texture_ = nullptr;
    }
}

WGPUSurface CreateNativeSurface(WGPUInstance instance, Window* window) {
    if (instance == nullptr || window == nullptr) {
        return nullptr;
    }

    WOKI_ASSERT(window->GetNativeHandle() != nullptr);

#ifdef __EMSCRIPTEN__
    WGPUSurfaceDescriptor descriptor = WGPU_SURFACE_DESCRIPTOR_INIT;
    WGPUEmscriptenSurfaceSourceCanvasHTMLSelector canvas =
        WGPU_EMSCRIPTEN_SURFACE_SOURCE_CANVAS_HTML_SELECTOR_INIT;
    canvas.selector = detail::ToWgpuStringView("canvas");
    descriptor.nextInChain = &canvas.chain;
    return wgpuInstanceCreateSurface(instance, &descriptor);
#else
    auto* glfw_window = static_cast<GLFWwindow*>(window->GetNativeHandle());
    if (glfw_window == nullptr) {
        return nullptr;
    }

    const int platform = glfwGetPlatform();

#if defined(GLFW_EXPOSE_NATIVE_COCOA)
    if (platform == GLFW_PLATFORM_COCOA) {
        return CreateNativeSurfaceCocoa(instance, window);
    }
#endif

#if defined(GLFW_EXPOSE_NATIVE_X11) || defined(GLFW_EXPOSE_NATIVE_WAYLAND) || defined(GLFW_EXPOSE_NATIVE_WIN32)
    WGPUSurfaceDescriptor descriptor = WGPU_SURFACE_DESCRIPTOR_INIT;

#if defined(GLFW_EXPOSE_NATIVE_X11)
    if (platform == GLFW_PLATFORM_X11) {
        WGPUSurfaceSourceXlibWindow source = WGPU_SURFACE_SOURCE_XLIB_WINDOW_INIT;
        source.display = glfwGetX11Display();
        source.window = static_cast<uint64_t>(glfwGetX11Window(glfw_window));
        if (source.display != nullptr && source.window != 0) {
            descriptor.nextInChain = &source.chain;
            return wgpuInstanceCreateSurface(instance, &descriptor);
        }
    }
#endif

#if defined(GLFW_EXPOSE_NATIVE_WAYLAND)
    if (platform == GLFW_PLATFORM_WAYLAND) {
        WGPUSurfaceSourceWaylandSurface source = WGPU_SURFACE_SOURCE_WAYLAND_SURFACE_INIT;
        source.display = glfwGetWaylandDisplay();
        source.surface = glfwGetWaylandWindow(glfw_window);
        if (source.display != nullptr && source.surface != nullptr) {
            descriptor.nextInChain = &source.chain;
            return wgpuInstanceCreateSurface(instance, &descriptor);
        }
    }
#endif

#if defined(GLFW_EXPOSE_NATIVE_WIN32)
    if (platform == GLFW_PLATFORM_WIN32) {
        WGPUSurfaceSourceWindowsHWND source = WGPU_SURFACE_SOURCE_WINDOWS_HWND_INIT;
        source.hwnd = glfwGetWin32Window(glfw_window);
        source.hinstance = GetModuleHandle(nullptr);
        if (source.hwnd != nullptr && source.hinstance != nullptr) {
            descriptor.nextInChain = &source.chain;
            return wgpuInstanceCreateSurface(instance, &descriptor);
        }
    }
#endif
#endif

    return nullptr;
#endif
}

} // namespace woki::api::wgpu
