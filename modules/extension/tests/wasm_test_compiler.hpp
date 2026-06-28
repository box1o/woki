#pragma once

#include <cstdlib>
#include <filesystem>
#include <string>
#include <string_view>

inline std::string woki_test_wasm_clang() {
#ifdef WOKI_TEST_WASM_CLANG
    return WOKI_TEST_WASM_CLANG;
#endif
    if (const char* from_env = std::getenv("WOKI_WASM_CLANG")) {
        return from_env;
    }
    return "clang";
}

inline std::string woki_test_wasm_clang_dir() {
    const std::string clang = woki_test_wasm_clang();
    if (const auto slash = clang.rfind('/'); slash != std::string::npos) {
        return clang.substr(0, slash);
    }
    if (const auto slash = clang.rfind('\\'); slash != std::string::npos) {
        return clang.substr(0, slash);
    }
    return {};
}

inline std::string woki_test_wasm_llvm_lib_dir() {
#ifdef WOKI_TEST_LLVM_PREFIX
    return std::string(WOKI_TEST_LLVM_PREFIX) + "/lib";
#endif
    if (const char* from_env = std::getenv("WOKI_LLVM_PREFIX")) {
        return std::string(from_env) + "/lib";
    }
    const std::string dir = woki_test_wasm_clang_dir();
    if (dir.empty()) {
        return {};
    }
    return dir + "/../lib";
}

inline std::string woki_test_wasm_toolchain_flags() {
    const std::string dir = woki_test_wasm_clang_dir();
    if (dir.empty()) {
        return {};
    }
#if defined(_WIN32)
    return " -B\"" + dir + "\"";
#else
    return " -B" + dir;
#endif
}

inline std::string woki_test_wasm_compile_prefix() {
    const std::string bin_dir = woki_test_wasm_clang_dir();
    if (bin_dir.empty()) {
        return {};
    }
#if defined(_WIN32)
    return "set PATH=" + bin_dir + ";%PATH%&& ";
#else
    std::string prefix;
    const std::string lib_dir = woki_test_wasm_llvm_lib_dir();
    if (!lib_dir.empty()) {
        prefix += "DYLD_LIBRARY_PATH=\"" + lib_dir + ":$DYLD_LIBRARY_PATH\" ";
    }
    std::string path_dirs = bin_dir;
#ifdef WOKI_TEST_WASM_LD
    path_dirs = std::string(WOKI_TEST_WASM_LD);
    if (const auto slash = path_dirs.rfind('/'); slash != std::string::npos) {
        path_dirs = path_dirs.substr(0, slash);
    }
    path_dirs += ":" + bin_dir;
#else
    if (const char* wasm_ld = std::getenv("WOKI_WASM_LD")) {
        std::string ld_dir = wasm_ld;
        if (const auto slash = ld_dir.rfind('/'); slash != std::string::npos) {
            path_dirs = ld_dir.substr(0, slash) + ":" + bin_dir;
        }
    }
#endif
    prefix += "PATH=\"" + path_dirs + ":$PATH\" ";
    return prefix;
#endif
}

inline std::string woki_test_wasm_ld_path() {
#ifdef WOKI_TEST_WASM_LD
    return "\"" + std::string(WOKI_TEST_WASM_LD) + "\"";
#endif
    if (const char* from_env = std::getenv("WOKI_WASM_LD")) {
        return "\"" + std::string(from_env) + "\"";
    }
    const std::string bin_dir = woki_test_wasm_clang_dir();
    if (bin_dir.empty()) {
        return {};
    }
#if defined(_WIN32)
    return "\"" + bin_dir + "\\wasm-ld.exe\"";
#else
    return "\"" + bin_dir + "/wasm-ld\"";
#endif
}

inline std::string woki_test_wasm_wl_exports(std::string_view export_flags) {
    std::string wl_exports;
    wl_exports.reserve(export_flags.size() + 16);
    for (std::string_view view = export_flags; !view.empty();) {
        constexpr std::string_view export_prefix = "--export=";
        if (view.starts_with(export_prefix)) {
            view.remove_prefix(export_prefix.size());
            const auto end = view.find_first_of(" \t");
            wl_exports += "-Wl,--export=";
            wl_exports.append(view.substr(0, end));
            wl_exports += ' ';
            view.remove_prefix(end == std::string_view::npos ? view.size() : end + 1);
            continue;
        }
        const auto end = view.find_first_of(" \t");
        wl_exports.append(view.substr(0, end));
        wl_exports += ' ';
        view.remove_prefix(end == std::string_view::npos ? view.size() : end + 1);
    }
    return wl_exports;
}

inline bool woki_test_compile_wasm(
    const std::string& source_path, const std::string& wasm_path, std::string_view export_flags) {
    namespace fs = std::filesystem;

    const std::string clang = woki_test_wasm_clang();
    const std::string bin_dir = woki_test_wasm_clang_dir();
    const std::string wl_exports = woki_test_wasm_wl_exports(export_flags);

    if (bin_dir.empty()) {
        const std::string command = clang +
                                    " --target=wasm32-unknown-unknown -nostdlib -fno-builtin "
                                    "-Wl,--no-entry -Wl,--export-memory " +
                                    wl_exports + "-o \"" + wasm_path + "\" \"" + source_path + "\"";
        return std::system(command.c_str()) == 0;
    }

    const std::string wasm_ld = woki_test_wasm_ld_path();
    const std::string wasm_ld_unquoted = [] {
        std::string path = woki_test_wasm_ld_path();
        if (path.size() >= 2 && path.front() == '"' && path.back() == '"') {
            path = path.substr(1, path.size() - 2);
        }
        return path;
    }();

    if (!wasm_ld.empty() && fs::exists(wasm_ld_unquoted)) {
        fs::path obj_path{wasm_path};
        obj_path.replace_extension(".o");

        const std::string compile = woki_test_wasm_compile_prefix() + clang +
                                    woki_test_wasm_toolchain_flags() +
                                    " --target=wasm32-unknown-unknown -nostdlib -fno-builtin -c \"" +
                                    source_path + "\" -o \"" + obj_path.string() + "\"";
        if (std::system(compile.c_str()) != 0) {
            return false;
        }

        const std::string link = woki_test_wasm_compile_prefix() + wasm_ld + " -o \"" + wasm_path +
                                 "\" \"" + obj_path.string() +
                                 "\" --no-entry --allow-undefined --export-memory " +
                                 std::string(export_flags);

        const bool ok = std::system(link.c_str()) == 0;
        std::error_code ec;
        fs::remove(obj_path, ec);
        return ok;
    }

    const std::string command = woki_test_wasm_compile_prefix() + clang +
                                woki_test_wasm_toolchain_flags() +
                                " --target=wasm32-unknown-unknown -nostdlib -fno-builtin "
                                "-Wl,--no-entry -Wl,--export-memory " +
                                wl_exports + "-o \"" + wasm_path + "\" \"" + source_path + "\"";
    return std::system(command.c_str()) == 0;
}
