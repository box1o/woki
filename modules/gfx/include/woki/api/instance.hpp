#pragma once

#include "woki/api/descriptors.hpp"
#include <woki/core.hpp>


namespace woki::api {



class Instance {
public:
    virtual ~Instance() = default;
    [[nodiscard]] static scope<Instance> Create(const InstanceDesc& desc = {});
protected:
    Instance() = default;
};

} // namespace woki
