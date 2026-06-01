#include "woki/core.hpp"

int main(int argc, char* argv[]) {
    slog::Configure();
    slog::Info("Starting application");
    slog::Critical("Critical error occurred");

    return 0;
}
