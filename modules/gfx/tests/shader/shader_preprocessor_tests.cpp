#include <catch2/catch_test_macros.hpp>

#include <string>
#include <string_view>
#include <unordered_map>

#include <woki/gfx/shaders.hpp>

namespace {

class MemoryIncludeProvider final : public woki::gfx::ShaderIncludeProvider {
public:
    std::unordered_map<std::string, std::string> files{};

    [[nodiscard]] woki::Result<woki::gfx::ShaderInclude> Load(
        const std::string_view requested_name, std::string_view) const override {
        const auto iterator = files.find(std::string(requested_name));
        if (iterator == files.end()) {
            return woki::Err(woki::ErrorCode::FileNotFound, "Shader include was not found");
        }

        return woki::Ok(woki::gfx::ShaderInclude{
            .canonical_name = iterator->first,
            .source = iterator->second,
        });
    }
};

} // namespace

TEST_CASE("Shader preprocessor expands reusable shader functions") {
    MemoryIncludeProvider provider;
    provider.files["lighting/common.wgsl"] =
        "fn saturate(value: f32) -> f32 { return clamp(value, 0.0, 1.0); }\n";
    const woki::gfx::ShaderPreprocessor preprocessor(provider);

    const auto result = preprocessor.Process(
        "#include \"lighting/common.wgsl\"\n@fragment fn main() {}\n", "materials/pbr.wgsl");

    REQUIRE(result.has_value());
    REQUIRE(result->source.find("fn saturate") != std::string::npos);
    REQUIRE(result->source.find("@fragment fn main") != std::string::npos);
    REQUIRE(result->dependencies == std::vector<std::string>{"lighting/common.wgsl"});
}

TEST_CASE("Shader preprocessor resolves nested includes once") {
    MemoryIncludeProvider provider;
    provider.files["math.wgsl"] = "fn square(x: f32) -> f32 { return x * x; }\n";
    provider.files["lighting.wgsl"] = "#include \"math.wgsl\"\nfn light() {}\n";
    const woki::gfx::ShaderPreprocessor preprocessor(provider);

    const auto result =
        preprocessor.Process("#include \"lighting.wgsl\"\n#include \"math.wgsl\"\n", "main.wgsl");

    REQUIRE(result.has_value());
    REQUIRE(result->dependencies == std::vector<std::string>{"lighting.wgsl", "math.wgsl"});
    REQUIRE(result->source.find("square") == result->source.rfind("square"));
}

TEST_CASE("Shader preprocessor rejects include cycles") {
    MemoryIncludeProvider provider;
    provider.files["a.wgsl"] = "#include \"b.wgsl\"\n";
    provider.files["b.wgsl"] = "#include \"a.wgsl\"\n";
    const woki::gfx::ShaderPreprocessor preprocessor(provider);

    const auto result = preprocessor.Process("#include \"a.wgsl\"\n", "main.wgsl");

    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error().Code() == woki::ErrorCode::ParseInvalidFormat);
    REQUIRE(result.error().Message().find("cycle") != std::string_view::npos);
}

TEST_CASE("Shader preprocessor reports missing and malformed includes") {
    MemoryIncludeProvider provider;
    const woki::gfx::ShaderPreprocessor preprocessor(provider);

    const auto missing = preprocessor.Process("#include \"missing.wgsl\"\n");
    const auto malformed = preprocessor.Process("#include missing.wgsl\n");

    REQUIRE_FALSE(missing.has_value());
    REQUIRE(missing.error().Code() == woki::ErrorCode::FileNotFound);
    REQUIRE_FALSE(malformed.has_value());
    REQUIRE(malformed.error().Code() == woki::ErrorCode::ParseInvalidFormat);
}
