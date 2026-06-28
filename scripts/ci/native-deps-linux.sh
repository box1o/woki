#!/usr/bin/env bash
set -euo pipefail

sudo apt-get update
sudo apt-get install -y \
  ccache \
  clang \
  libarchive-dev \
  libwayland-dev \
  libxkbcommon-dev \
  xorg-dev

echo "WOKI_WASM_CLANG=clang" >> "${GITHUB_ENV}"
