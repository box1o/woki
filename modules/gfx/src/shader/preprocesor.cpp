#include "../../include/woki/shader/preprocessor.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <string_view>
#include <system_error>

namespace woki::gfx {
namespace {

[[nodiscard]] std::filesystem::path NormalizePath(const std::filesystem::path& path) {
    std::error_code error;
    auto normalized = std::filesystem::weakly_canonical(path, error);

    if (error) {
        return path.lexically_normal();
    }

    return normalized;
}

[[nodiscard]] Result<std::string> ReadFile(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);

    if (!stream.is_open()) {
        return Err(
            ErrorCode::FileReadError, "Failed to open shader file: " + path.generic_string());
    }

    std::string source{
        std::istreambuf_iterator<char>(stream),
        std::istreambuf_iterator<char>(),
    };

    if (stream.bad()) {
        return Err(
            ErrorCode::FileReadError, "Failed to read shader file: " + path.generic_string());
    }

    return Ok(std::move(source));
}

[[nodiscard]] std::string_view TrimLeft(std::string_view value) noexcept {
    const auto iterator = std::find_if(value.begin(), value.end(), [](const char character) {
        return std::isspace(static_cast<unsigned char>(character)) == 0;
    });

    value.remove_prefix(static_cast<std::size_t>(iterator - value.begin()));

    return value;
}

[[nodiscard]] Result<std::string_view> ParseInclude(std::string_view line) {
    line = TrimLeft(line);

    constexpr std::string_view directive{"#include"};

    if (!line.starts_with(directive)) {
        return Ok(std::string_view{});
    }

    line.remove_prefix(directive.size());
    line = TrimLeft(line);

    if (line.size() < 2 || line.front() != '"') {
        return Err(ErrorCode::ParseInvalidFormat, "Shader include must use #include \"path\"");
    }

    line.remove_prefix(1);

    const std::size_t closing_quote = line.find('"');

    if (closing_quote == std::string_view::npos || closing_quote == 0) {
        return Err(ErrorCode::ParseInvalidFormat, "Shader include path is invalid");
    }

    const std::string_view include_path = line.substr(0, closing_quote);

    line.remove_prefix(closing_quote + 1);
    line = TrimLeft(line);

    if (!line.empty() && !line.starts_with("//")) {
        return Err(ErrorCode::ParseUnexpectedToken, "Unexpected text after shader include");
    }

    return Ok(include_path);
}

} // namespace

Result<PreprocessedShader> ShaderPreprocessor::Process( const std::filesystem::path& source_path) const {
    if (source_path.empty()) {
        return Err(ErrorCode::InvalidArgument, "Shader source path cannot be empty");
    }

    const auto normalized_path = NormalizePath(source_path);

    ProcessContext context{};
    context.include_stack.push_back(normalized_path);

    auto source = ProcessFile(normalized_path, context);

    if (!source) {
        return Err(source.error());
    }

    return Ok(PreprocessedShader{
        .source = std::move(*source),
        .dependencies = std::move(context.dependencies),
    });
}

Result<std::string> ShaderPreprocessor::ProcessFile(
    const std::filesystem::path& source_path, ProcessContext& context) const {
    auto file_source = ReadFile(source_path);

    if (!file_source) {
        return Err(file_source.error());
    }

    std::string output{};
    output.reserve(file_source->size());

    std::size_t offset = 0;

    while (offset < file_source->size()) {
        const std::size_t line_end = file_source->find('\n', offset);

        const bool has_newline = line_end != std::string::npos;

        const std::size_t line_length =
            has_newline ? line_end - offset : file_source->size() - offset;

        std::string_view line{
            file_source->data() + offset,
            line_length,
        };

        if (!line.empty() && line.back() == '\r') {
            line.remove_suffix(1);
        }

        auto include_name = ParseInclude(line);

        if (!include_name) {
            return Err(include_name.error());
        }

        if (include_name->empty()) {
            output.append(line);

            if (has_newline) {
                output.push_back('\n');
            }
        } else {
            const auto include_path =
                NormalizePath(source_path.parent_path() / std::filesystem::path{*include_name});

            if (std::find(context.include_stack.begin(), context.include_stack.end(),
                    include_path) != context.include_stack.end()) {
                return Err(ErrorCode::ParseInvalidFormat,
                    "Circular shader include detected: " + include_path.generic_string());
            }

            const std::string include_key = include_path.generic_string();

            if (!context.included.contains(include_key)) {
                context.included.insert(include_key);
                context.dependencies.push_back(include_path);
                context.include_stack.push_back(include_path);

                auto included_source = ProcessFile(include_path, context);

                context.include_stack.pop_back();

                if (!included_source) {
                    return Err(included_source.error());
                }

                output.append(*included_source);

                if (has_newline && (output.empty() || output.back() != '\n')) {
                    output.push_back('\n');
                }
            }
        }

        if (!has_newline) {
            break;
        }

        offset = line_end + 1;
    }

    return Ok(std::move(output));
}

} // namespace woki::gfx
