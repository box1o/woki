#include <catch2/catch_test_macros.hpp>

#include <woki/core.hpp>

TEST_CASE("Environment helper can read PATH-like variables when present") {
#if defined(_WIN32)
    constexpr std::string_view kVariable = "PATH";
#else
    constexpr std::string_view kVariable = "HOME";
#endif

    REQUIRE(woki::env::Has(kVariable));
    auto value = woki::env::Get(kVariable);
    REQUIRE(value.has_value());
    REQUIRE_FALSE(value->empty());
}
