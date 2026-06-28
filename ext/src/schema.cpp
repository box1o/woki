#include "wokiext/cli.hpp"

#include <fstream>
#include <iostream>

#ifndef WOKI_MANIFEST_SCHEMA_PATH
#define WOKI_MANIFEST_SCHEMA_PATH "modules/extension/schema/manifest.schema.json"
#endif

namespace wokiext {

Status Schema() {
    std::ifstream input(WOKI_MANIFEST_SCHEMA_PATH, std::ios::binary);
    if (!input.good()) {
        std::cerr << "Failed to read manifest schema: " << WOKI_MANIFEST_SCHEMA_PATH << '\n';
        return Status::Error;
    }

    std::cout << input.rdbuf() << '\n';
    return Status::Ok;
}

} // namespace wokiext
