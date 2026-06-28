$ErrorActionPreference = 'Stop'

cmake -B build -S . `
  -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5 `
  -DBUILD_TESTING=ON `
  -DCMAKE_PREFIX_PATH="$env:DAWN_PREFIX" `
  -DDawn_DIR="$env:Dawn_DIR" `
  -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" `
  -DVCPKG_TARGET_TRIPLET=x64-windows

cmake --build build --target woki_tests woki_extensions studio -j 4
ctest --test-dir build --output-on-failure
