#!/usr/bin/env bash
set -euo pipefail

asset_suffix="${1:?Dawn asset suffix required (e.g. ubuntu-latest-Release.tar.gz)}"
dawn_prefix="${2:-$(pwd)/3rdparty/.deps/dawn-install}"
tmpdir="$(mktemp -d)"
archive="${tmpdir}/dawn-prebuilt.tar.gz"
trap 'rm -rf "${tmpdir}"' EXIT

rm -rf "${dawn_prefix}"
mkdir -p "${dawn_prefix}"

gh release download -R "${DAWN_RELEASE_REPO:-google/dawn}" \
  --pattern "*${asset_suffix}" \
  --output "${archive}"

tar -xzf "${archive}" -C "${dawn_prefix}" --strip-components=1

dawn_config="$(find "${dawn_prefix}" -path '*/cmake/Dawn/DawnConfig.cmake' -print -quit)"
if [[ -z "${dawn_config}" ]]; then
  echo "Downloaded Dawn archive does not contain DawnConfig.cmake" >&2
  exit 1
fi

{
  echo "DAWN_PREFIX=${dawn_prefix}"
  echo "Dawn_DIR=$(dirname "${dawn_config}")"
} >> "${GITHUB_ENV}"
