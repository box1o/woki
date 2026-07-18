#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include <woki/core.hpp>

namespace woki::gfx {

struct ShaderInclude final {
    std::string canonical_name{};
    std::string source{};
};

class ShaderIncludeProvider {
public:
    virtual ~ShaderIncludeProvider() = default;

    [[nodiscard]] virtual Result<ShaderInclude> Load(
        std::string_view requested_name, std::string_view including_source) const = 0;

protected:
    ShaderIncludeProvider() = default;
};

class FileShaderIncludeProvider final : public ShaderIncludeProvider {
public:
    explicit FileShaderIncludeProvider(std::vector<std::filesystem::path> search_paths = {});

    void AddSearchPath(std::filesystem::path search_path);

    [[nodiscard]] const std::vector<std::filesystem::path>& SearchPaths() const noexcept;

    [[nodiscard]] Result<ShaderInclude> Load(
        std::string_view requested_name, std::string_view including_source) const override;

private:
    std::vector<std::filesystem::path> search_paths_{};
};

struct ShaderPreprocessOptions final {
    u32 max_include_depth{64};
    bool include_once{true};
};

struct PreprocessedShader final {
    std::string source{};
    std::vector<std::string> dependencies{};
};

class ShaderPreprocessor final {
public:
    explicit ShaderPreprocessor(const ShaderIncludeProvider& include_provider,
        ShaderPreprocessOptions options = {}) noexcept;

    [[nodiscard]] Result<PreprocessedShader> Process(
        std::string_view source, std::string_view source_name = {}) const;

private:
    const ShaderIncludeProvider* include_provider_{nullptr};
    ShaderPreprocessOptions options_{};
};

} // namespace woki::gfx
