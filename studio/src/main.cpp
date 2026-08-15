#include <woki/core.hpp>
#include <woki/math.hpp>
#include <woki/rhi.hpp>

using namespace wk;

int main(int argc, char* argv[]) {
    slog::Configure("studio");
    slog::Info("Starting Woki Studio...");
    test();

    return 0;
}
