#!/usr/bin/env bash
set -euo pipefail

sudo apt-get update
sudo apt-get install -y ccache

emcmake cmake -B build-web -S . \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=OFF \
  -DCMAKE_C_COMPILER_LAUNCHER=ccache \
  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache

cmake --build build-web --target studio -j"$(nproc)"

for artifact in studio.html studio.js studio.wasm; do
  test -f "build-web/${artifact}"
done

rm -rf build-web-publish
mkdir -p build-web-publish
cp build-web/studio.html build-web-publish/index.html
cp build-web/studio.js build-web-publish/
cp build-web/studio.wasm build-web-publish/

if [[ -f build-web/studio.data ]]; then
  cp build-web/studio.data build-web-publish/
fi

copy_icon() {
  local name="$1"
  if [[ -f "build-web/${name}" ]]; then
    cp "build-web/${name}" build-web-publish/
  elif [[ -f "web/${name}" ]]; then
    cp "web/${name}" build-web-publish/
  elif [[ -f "config/icons/${name}" ]]; then
    cp "config/icons/${name}" build-web-publish/
  fi
}

copy_icon favicon.svg
copy_icon favicon.ico
copy_icon apple-touch-icon.png
copy_icon woki-32.png
copy_icon woki-64.png
copy_icon woki-sad.svg

ccache -s
