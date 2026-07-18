#include <woki/gfx/shader/shader_preprocessor.hpp>

#include <fstream>
#include <iterator>
#include <system_error>
#include <utility>

namespace woki::gfx {
namespace {

[[nodiscard]] std::filesystem::path NormalizePath(const std::filesystem::path& path) {
    std::error_code error;
    auto normalized = std::filesystem::weakly_canonical(path, error);
    return error ? path.lexically_normal() : normalized;
}

[[nodiscard]] Result<ShaderInclude> ReadInclude(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream.is_open()) {
        return Err(ErrorCode::FileReadError, "Failed to open shader include");
    }

    std::string source{std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
    if (stream.bad()) {
        return Err(ErrorCode::FileReadError, "Failed to read shader include");
    }

    const auto canonical_path = NormalizePath(path);
    return Ok(ShaderInclude{
        .canonical_name = canonical_path.generic_string(),
        .source = std::move(source),
    });
}

} // namespace

FileShaderIncludeProvider::FileShaderIncludeProvider(
    std::vector<std::filesystem::path> search_paths) {
    search_paths_.reserve(search_paths.size());
    for (auto& search_path : search_paths) {
        AddSearchPath(std::move(search_path));
    }
}

void FileShaderIncludeProvider::AddSearchPath(std::filesystem::path search_path) {
    search_paths_.push_back(NormalizePath(search_path));
}

const std::vector<std::filesystem::path>& FileShaderIncludeProvider::SearchPaths() const noexcept {
    return search_paths_;
}

Result<ShaderInclude> FileShaderIncludeProvider::Load(
    const std::string_view requested_name, const std::string_view including_source) const {
    if (requested_name.empty()) {
        return Err(ErrorCode::InvalidArgument, "Shader include path cannot be empty");
    }

    const std::filesystem::path requested{requested_name};
    std::vector<std::filesystem::path> candidates{};
    candidates.reserve(search_paths_.size() + 2);

    if (requested.is_absolute()) {
        candidates.push_back(requested);
    } else {
        if (!including_source.empty()) {
            const std::filesystem::path including_path{including_source};
            if (including_path.has_parent_path()) {
                candidates.push_back(including_path.parent_path() / requested);
            }
        }
        for (const auto& search_path : search_paths_) {
            candidates.push_back(search_path / requested);
        }
        candidates.push_back(requested);
    }

    std::error_code error;
    for (const auto& candidate : candidates) {
        if (std::filesystem::is_regular_file(candidate, error) && !error) {
            return ReadInclude(candidate);
        }
        error.clear();
    }

    return Err(ErrorCode::FileNotFound, "Shader include was not found in configured search paths");
}

} // namespace woki::gfx
