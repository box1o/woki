#!/usr/bin/env bash
set -euo pipefail

brew install ccache libarchive llvm lld

llvm_bin="$(brew --prefix llvm)/bin"
llvm_prefix="$(brew --prefix llvm)"
wasm_ld="$(command -v wasm-ld)"
{
  echo "WOKI_WASM_CLANG=${llvm_bin}/clang"
  echo "WOKI_LLVM_PREFIX=${llvm_prefix}"
  echo "WOKI_WASM_LD=${wasm_ld}"
  echo "CMAKE_PREFIX_PATH=$(brew --prefix libarchive)"
} >> "${GITHUB_ENV}"
