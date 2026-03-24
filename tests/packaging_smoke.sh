#!/bin/bash
set -e

if ! command -v cmake >/dev/null; then
  echo "[test] cmake not found; skipping packaging smoke test"
  exit 0
fi

if ! command -v cpack >/dev/null; then
  echo "[test] cpack not found; skipping packaging smoke test"
  exit 0
fi

build_dir="$(mktemp -d -t aym-cpack-XXXXXX)"
out_dir="${build_dir}/cpack-out"
trap 'rm -rf "${build_dir}"' EXIT

cmake -S . -B "${build_dir}" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF -DAYM_ENABLE_UNIT_TESTS=OFF >/dev/null
cpack -G TGZ -B "${out_dir}" --config "${build_dir}/CPackConfig.cmake" >/dev/null

ls "${out_dir}"/*.tar.gz >/dev/null

echo "[test] packaging smoke test passed"
