#pragma once

// IWYU pragma: private, include "woki/ext/ext.hpp"

#include <filesystem>

namespace woki::ext {

[[nodiscard]] bool HasPathTraversal(const std::filesystem::path& path);
[[nodiscard]] bool IsSafeRelativePath(const std::filesystem::path& path);

} // namespace woki::ext
