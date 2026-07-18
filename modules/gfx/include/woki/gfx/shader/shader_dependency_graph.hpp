#pragma once

#include "../resource/resource_types.hpp"

#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace woki::gfx {

class ShaderDependencyGraph final {
public:
    void Update(
        ShaderHandle shader, std::string source_name, std::span<const std::string> dependencies);

    [[nodiscard]] bool Remove(ShaderHandle shader);

    [[nodiscard]] std::vector<ShaderHandle> AffectedBy(std::string_view changed_source) const;
    [[nodiscard]] std::vector<ShaderHandle> AffectedBy(
        std::span<const std::string> changed_sources) const;

    [[nodiscard]] std::span<const std::string> Dependencies(ShaderHandle shader) const noexcept;
    [[nodiscard]] std::size_t ShaderCount() const noexcept;
    [[nodiscard]] bool Empty() const noexcept;

    void Clear();

private:
    struct ShaderDependencies final {
        std::string source_name{};
        std::vector<std::string> dependencies{};
    };

    void AddReverseEdge(std::string_view source_name, ShaderHandle shader);
    void RemoveReverseEdge(std::string_view source_name, ShaderHandle shader);

    std::unordered_map<ShaderHandle, ShaderDependencies> shaders_{};
    std::unordered_map<std::string, std::unordered_set<ShaderHandle>> dependents_{};
};

} // namespace woki::gfx
