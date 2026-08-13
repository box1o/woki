#include <woki/core.hpp>
#include <woki/math.hpp>

int main(int argc, char* argv[]) {
    slog::Configure("studio");
    slog::Info("Starting Woki Studio...");

    slog::Info("Hello {}", 123);
    slog::Warn("Something is {}", "wrong");
    slog::Error("Error code {}", 42);
    return 0;
}
