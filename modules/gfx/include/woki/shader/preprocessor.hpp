#pragma once

#include <filesystem>
#include <string>
#include <unordered_set>
#include <vector>

#include <woki/core.hpp>

namespace woki::gfx {

struct PreprocessedShader final {
    std::string source{};
    std::vector<std::filesystem::path> dependencies{};
};

class ShaderPreprocessor final {
public:
    [[nodiscard]] Result<PreprocessedShader> Process(const std::filesystem::path& source_path) const;

private:
    struct ProcessContext final {
        std::vector<std::filesystem::path> dependencies{};
        std::vector<std::filesystem::path> include_stack{};
        std::unordered_set<std::string> included{};
    };

    [[nodiscard]] Result<std::string> ProcessFile( const std::filesystem::path& source_path, ProcessContext& context) const;
};

} // namespace woki::gfx
