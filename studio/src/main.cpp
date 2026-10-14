#include <wk/core.hpp>

using namespace wk;


int main (int argc, char *argv[]) {
    slog::configure("woki", slog::Level::Debug , "woki.log");
    slog::info("Starting up....");

    return 0;
}
