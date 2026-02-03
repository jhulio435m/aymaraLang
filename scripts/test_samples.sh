#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
COMPILER="${BUILD_DIR}/bin/aymc"

if [[ ! -x "${COMPILER}" ]]; then
  cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
  cmake --build "${BUILD_DIR}" -j
fi

if [[ ! -x "${COMPILER}" ]]; then
  echo "No se encontro el compilador en ${COMPILER}" >&2
  exit 1
fi

mapfile -t SAMPLE_FILES < <(find "${ROOT_DIR}/samples" -type f -name "*.aym" ! -path "*/errors/*" | sort)

run_target() {
  local target_flag="$1"
  local label="$2"
  echo "==> Compilando samples para ${label}"
  for sample in "${SAMPLE_FILES[@]}"; do
    local rel_path="${sample#${ROOT_DIR}/}"
    local base_name
    base_name="$(basename "${sample}" .aym)"
    local out_dir="${BUILD_DIR}/sample_outputs/${label}/$(dirname "${rel_path}")"
    mkdir -p "${out_dir}"
    "${COMPILER}" "${target_flag}" -o "${out_dir}/${base_name}" "${sample}"
  done
}

if ! command -v nasm >/dev/null 2>&1; then
  echo "nasm no esta disponible en PATH; no se pueden compilar samples." >&2
  exit 1
fi

if ! command -v gcc >/dev/null 2>&1; then
  echo "gcc no esta disponible en PATH; no se pueden compilar samples." >&2
  exit 1
fi

run_target "--linux" "Linux"

uname_out="$(uname -s || true)"
case "${uname_out}" in
  MINGW*|MSYS*|CYGWIN*)
    run_target "--windows" "Windows"
    ;;
  *)
    if command -v x86_64-w64-mingw32-gcc >/dev/null 2>&1; then
      echo "Detectado toolchain MinGW en Linux, pero el compilador usa gcc directamente."
      echo "Configura gcc hacia MinGW para ejecutar pruebas Windows."
    else
      echo "Toolchain de Windows no detectado; pruebas Windows omitidas."
    fi
    ;;
esac
