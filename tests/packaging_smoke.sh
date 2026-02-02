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

build_dir="build-cpack-test"
out_dir="${build_dir}/cpack-out"

cmake -S . -B "${build_dir}" -DCMAKE_BUILD_TYPE=Release >/dev/null
cpack -G TGZ -B "${out_dir}" --config "${build_dir}/CPackConfig.cmake" >/dev/null

ls "${out_dir}"/*.tar.gz >/dev/null
rm -rf "${build_dir}"

echo "[test] packaging smoke test passed"
