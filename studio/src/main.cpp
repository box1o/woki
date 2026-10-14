
#include <wk/core.hpp>

#include "core/application.hpp"

using namespace wk;

int main(int argc, char* argv[]) {
    slog::configure("woki", slog::Level::Debug, "woki.log");
    slog::info("Starting up....");

    studio::application app;
    while (app.run());

    return 0;
}
