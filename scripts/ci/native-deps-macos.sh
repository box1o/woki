#!/usr/bin/env bash
set -euo pipefail

brew install ccache libarchive lld llvm

llvm_bin="$(brew --prefix llvm)/bin"
llvm_prefix="$(brew --prefix llvm)"
lld_bin="$(brew --prefix lld)/bin"
{
  echo "WOKI_WASM_CLANG=${llvm_bin}/clang"
  echo "WOKI_WASM_LD=${lld_bin}/wasm-ld"
  echo "WOKI_LLVM_PREFIX=${llvm_prefix}"
  echo "CMAKE_PREFIX_PATH=$(brew --prefix libarchive)"
} >> "${GITHUB_ENV}"
