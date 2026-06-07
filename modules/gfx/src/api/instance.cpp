#include "../../include/woki/api/instance.hpp"
#include "woki/core.hpp"

#include "../../backend/wgpu/wgpu_instance.hpp"

namespace woki::api {

scope<Instance> Instance::Create(const InstanceDesc& desc) {
    Backend backend = desc.backend;
    if (backend == Backend::kAuto) {
        backend = Backend::kWebGpu;
    }

    if (backend != Backend::kWebGpu) {
        slog::Error("gfx::Instance only supports the WebGPU backend right now");
        return nullptr;
    }

    auto instance = createScope<wgpu::WgpuInstanceImpl>(desc);
    // if (!instance->IsValid()) {
    //     return nullptr;
    // }

    if (!instance->IsValid()) {
        return nullptr;
    }

    return instance;
}

} // namespace woki::api
