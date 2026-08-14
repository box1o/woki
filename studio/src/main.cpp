#include <woki/core.hpp>
#include <woki/math.hpp>

using namespace wk;

Result<void> DoSomething() {
    // Simulate an error
    return Err(ErrorCode::InvalidState, "Something went wrong");
}


int main(int argc, char* argv[]) {
    slog::Configure("studio");
    slog::Info("Starting Woki Studio...");

    slog::Info("Hello {}", 123);
    slog::Warn("Something is {}", "wrong");
    slog::Error("Error code {}", 42);

    TRY(DoSomething());
    return 0;
}
