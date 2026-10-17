#include <wk/core.hpp>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "core/application.hpp"

using namespace wk;

int main(int argc, char* argv[]) {
    slog::configure("woki", slog::Level::Debug, "woki.log");
    slog::info("Starting up....");

    studio::application app;

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg([](void* arg) {
        static_cast<studio::application*>(arg)->run();
    }, &app, 0, 1);
#else
    while (app.run());
#endif

    return 0;
}
