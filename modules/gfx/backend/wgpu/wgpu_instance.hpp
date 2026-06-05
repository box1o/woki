#pragma once
#include "../../include/woki/api/instance.hpp"

#include <webgpu/webgpu.h>

namespace woki::api::wgpu {

class WgpuInstanceImpl final : public Instance {
public:
    explicit WgpuInstanceImpl(const InstanceDesc& desc);
    ~WgpuInstanceImpl() override;


private:
};
} // namespace woki::api::wgpu
