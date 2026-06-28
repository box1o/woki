#include "wokiext/cli.hpp"

int main(int argc, const char* const* argv) {
    return wokiext::Run(std::span<const char* const>(argv, static_cast<std::size_t>(argc)));
}
