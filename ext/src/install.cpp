#include "wokiext/cli.hpp"

#include <woki/ext/ext.hpp>

#include <chrono>
#include <filesystem>
#include <iostream>

namespace wokiext {

namespace {

namespace fs = std::filesystem;

[[nodiscard]] woki::Result<woki::ext::Roots> RootsFromOption(const fs::path& root) {
    if (root.empty()) {
        return woki::ext::DefaultRoots();
    }

    const fs::path normalized = fs::absolute(root).lexically_normal();
    woki::ext::Roots roots;
    roots.extensions = normalized / "extensions";
    roots.data = normalized / "ext-data";
    roots.cache = normalized / "cache" / "ext";
    return woki::Ok(std::move(roots));
}

} // namespace

Status Install(const InstallOptions& options) {
    auto roots = RootsFromOption(options.root);
    if (!roots) {
        std::cerr << roots.error().Message() << '\n';
        return Status::Error;
    }

    const fs::path path = fs::absolute(options.path).lexically_normal();
    fs::path install_path = path;
    fs::path temp_archive;

    if (fs::is_directory(path)) {
        if (options.force) {
            auto manifest = woki::ext::LoadManifest(path / "manifest.yaml");
            if (!manifest) {
                std::cerr << manifest.error().Message() << '\n';
                return Status::Error;
            }
            if (fs::exists(roots->extensions / manifest->id)) {
                (void)Remove(RemoveOptions{.id = manifest->id, .root = options.root});
            }
        }

        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        temp_archive =
            fs::temp_directory_path() / ("wokiext-install-" + std::to_string(stamp) + ".wokiext");
        if (Bundle(BundleOptions{.path = path, .out_file = temp_archive}) != Status::Ok) {
            return Status::Error;
        }
        install_path = temp_archive;
    }

    woki::Result<woki::ext::PackageLayout> installed =
        woki::ext::InstallArchive(install_path, *roots);

    if (!temp_archive.empty()) {
        std::error_code error;
        fs::remove(temp_archive, error);
    }

    if (!installed) {
        std::cerr << installed.error().Message() << '\n';
        return Status::Error;
    }

    std::cout << "Installed extension: " << installed->install_root << '\n';
    return Status::Ok;
}

} // namespace wokiext
