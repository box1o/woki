#include <woki/gfx/shader/shader_preprocessor.hpp>

#include <algorithm>
#include <cctype>
#include <string>
#include <unordered_set>
#include <utility>

namespace woki::gfx {
namespace {

struct ProcessContext final {
    const ShaderIncludeProvider& provider;
    const ShaderPreprocessOptions& options;
    std::vector<std::string> stack{};
    std::vector<std::string> dependencies{};
    std::unordered_set<std::string> included{};
};

[[nodiscard]] std::string_view TrimLeft(std::string_view value) noexcept {
    const auto iterator = std::ranges::find_if(value, [](const char character) {
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
        return Err(ErrorCode::ParseInvalidFormat, "Shader include has an invalid path");
    }

    const std::string_view name = line.substr(0, closing_quote);
    line.remove_prefix(closing_quote + 1);
    line = TrimLeft(line);
    if (!line.empty() && !line.starts_with("//")) {
        return Err(ErrorCode::ParseUnexpectedToken, "Unexpected text after shader include");
    }

    return Ok(name);
}

[[nodiscard]] std::string CycleMessage(
    const std::vector<std::string>& stack, const std::string_view repeated) {
    std::string message{"Shader include cycle: "};
    for (const auto& name : stack) {
        message.append(name).append(" -> ");
    }
    message.append(repeated);
    return message;
}

[[nodiscard]] Result<std::string> ProcessSource(ProcessContext& context,
    const std::string_view source, const std::string_view source_name, const u32 depth) {
    if (depth > context.options.max_include_depth) {
        return Err(ErrorCode::ValidationOutOfRange, "Shader include depth limit exceeded");
    }

    std::string output{};
    std::size_t offset = 0;
    while (offset < source.size()) {
        const std::size_t line_end = source.find('\n', offset);
        const bool has_newline = line_end != std::string_view::npos;
        const std::size_t count = has_newline ? line_end - offset : source.size() - offset;
        const std::string_view line = source.substr(offset, count);

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
            auto include = context.provider.Load(*include_name, source_name);
            if (!include) {
                return Err(include.error());
            }
            if (include->canonical_name.empty()) {
                return Err(ErrorCode::ValidationInvalidState,
                    "Shader include provider returned an empty name");
            }

            if (std::ranges::find(context.stack, include->canonical_name) != context.stack.end()) {
                return Err(ErrorCode::ParseInvalidFormat,
                    CycleMessage(context.stack, include->canonical_name));
            }

            const bool already_included = context.included.contains(include->canonical_name);
            if (!context.options.include_once || !already_included) {
                if (!already_included) {
                    context.included.insert(include->canonical_name);
                    context.dependencies.push_back(include->canonical_name);
                }
                context.stack.push_back(include->canonical_name);
                auto processed =
                    ProcessSource(context, include->source, include->canonical_name, depth + 1);
                context.stack.pop_back();
                if (!processed) {
                    return Err(processed.error());
                }
                output.append(*processed);
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

} // namespace

ShaderPreprocessor::ShaderPreprocessor(
    const ShaderIncludeProvider& include_provider, const ShaderPreprocessOptions options) noexcept
    : include_provider_(&include_provider), options_(options) {}

Result<PreprocessedShader> ShaderPreprocessor::Process(
    const std::string_view source, const std::string_view source_name) const {
    WOKI_ASSERT(include_provider_ != nullptr);

    ProcessContext context{.provider = *include_provider_, .options = options_};
    if (!source_name.empty()) {
        context.stack.emplace_back(source_name);
    }

    auto processed = ProcessSource(context, source, source_name, 0);
    if (!processed) {
        return Err(processed.error());
    }

    return Ok(PreprocessedShader{
        .source = std::move(*processed),
        .dependencies = std::move(context.dependencies),
    });
}

} // namespace woki::gfx
