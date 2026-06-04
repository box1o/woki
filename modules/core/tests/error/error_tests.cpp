#include <catch2/catch_test_macros.hpp>

#include <woki/core.hpp>

TEST_CASE("Error categorizes codes") {
    const woki::Error file_error(woki::ErrorCode::FileNotFound, "missing");
    REQUIRE(file_error.Type() == woki::ErrorType::FileSystem);

    const woki::Error graphics_error(woki::ErrorCode::GraphicsInitFailed, "gpu");
    REQUIRE(graphics_error.Type() == woki::ErrorType::Graphics);
}

TEST_CASE("Result ok and err helpers work") {
    auto ok_value = woki::Ok(42);
    REQUIRE(ok_value.has_value());
    REQUIRE(*ok_value == 42);

    auto err_value = woki::Err(woki::ErrorCode::InvalidArgument, "bad input");
    REQUIRE(err_value.error().Code() == woki::ErrorCode::InvalidArgument);
}

TEST_CASE("Error to-string helpers work") {
    REQUIRE(std::string_view(woki::ToString(woki::ErrorType::Core)) == "Core");
    REQUIRE(std::string_view(woki::ToString(woki::ErrorCode::UnknownError)) == "UnknownError");
}
