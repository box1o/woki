#include "wokiext/cli.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

namespace wokiext {

namespace {

[[nodiscard]] std::string Quote(const std::filesystem::path& path) {
    std::string text = path.string();
    std::string out = "'";
    for (const char ch : text) {
        if (ch == '\'') {
            out += "'\\''";
        } else {
            out.push_back(ch);
        }
    }
    out.push_back('\'');
    return out;
}

} // namespace

Status Build(const BuildOptions& options) {
    const std::filesystem::path root = std::filesystem::absolute(options.path).lexically_normal();
    const std::filesystem::path build_dir = root / "build";

    if (!std::filesystem::is_regular_file(root / "CMakeLists.txt")) {
        std::cerr << "Extension project is missing CMakeLists.txt: " << root << '\n';
        return Status::Error;
    }

    const std::string configure = "cmake -B " + Quote(build_dir) + " -S " + Quote(root) +
                                  " -DCMAKE_BUILD_TYPE=" + options.config;
    if (std::system(configure.c_str()) != 0) {
        return Status::Error;
    }

    const std::string build = "cmake --build " + Quote(build_dir);
    if (std::system(build.c_str()) != 0) {
        return Status::Error;
    }

    return Status::Ok;
}

} // namespace wokiext
