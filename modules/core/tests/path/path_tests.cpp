#include <catch2/catch_test_macros.hpp>

#include <woki/core.hpp>

TEST_CASE("Working and temporary directories resolve") {
    auto working = woki::paths::WorkingDirectory();
    auto temporary = woki::paths::TemporaryDirectory();

    REQUIRE(working.has_value());
    REQUIRE(temporary.has_value());
    REQUIRE_FALSE(working->empty());
    REQUIRE_FALSE(temporary->empty());
}

TEST_CASE("Path join and normalize work") {
    const auto joined = woki::paths::Join("foo", "bar");
    REQUIRE(joined.filename() == "bar");

    auto normalized = woki::paths::Normalize(joined);
    REQUIRE(normalized.has_value());
}

TEST_CASE("App paths resolve for a sample app") {
    auto app_paths = woki::paths::AppPaths("woki-test");
    REQUIRE(app_paths.has_value());
    REQUIRE_FALSE(app_paths->working.empty());
    REQUIRE_FALSE(app_paths->temporary.empty());
    REQUIRE_FALSE(app_paths->logs.empty());
}

TEST_CASE("Logs directory resolves for app name") {
    auto logs = woki::paths::LogsDirectory("woki-test");
    REQUIRE(logs.has_value());
    REQUIRE(logs->filename() == "logs");
}
