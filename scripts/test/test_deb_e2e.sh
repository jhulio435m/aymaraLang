#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
PKG_NAME="aymaralang"
ARCH="amd64"
VERSION_FILE="${ROOT_DIR}/VERSION.txt"
VERSION="0.1.0"
BUILD_DIR="${ROOT_DIR}/build-linux"
DIST_DIR="${ROOT_DIR}/dist"
TMP_BASICOS="/tmp/aym_ci_basicos.$$"
TMP_FLOW="/tmp/aym_ci_flow.$$"
OUT_BASICOS="/tmp/aym_ci_basicos.out.$$"
OUT_FLOW="/tmp/aym_ci_flow.out.$$"

if [[ -f "${VERSION_FILE}" ]]; then
  VERSION="$(tr -d '\n\r' < "${VERSION_FILE}")"
fi

DEB_PATH="${ROOT_DIR}/artifacts/${PKG_NAME}_${VERSION}_${ARCH}.deb"

SUDO=""
if [[ "${EUID}" -ne 0 ]]; then
  if command -v sudo >/dev/null 2>&1; then
    SUDO="sudo"
  else
    echo "Se requiere sudo para instalar/purgar el paquete .deb." >&2
    exit 1
  fi
fi

cleanup() {
  rm -f "${TMP_BASICOS}" "${TMP_FLOW}" "${OUT_BASICOS}" "${OUT_FLOW}"
  ${SUDO} dpkg --purge "${PKG_NAME}" >/dev/null 2>&1 || true
}
trap cleanup EXIT INT TERM

if ${SUDO} dpkg -s "${PKG_NAME}" >/dev/null 2>&1; then
  ${SUDO} dpkg --purge "${PKG_NAME}" >/dev/null 2>&1 || true
fi

rm -rf "${DIST_DIR}" "${BUILD_DIR}"

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
cmake --build "${BUILD_DIR}" -j
cmake --install "${BUILD_DIR}" --prefix "${DIST_DIR}"

bash "${ROOT_DIR}/scripts/test/test_deb.sh"

if [[ ! -f "${DEB_PATH}" ]]; then
  echo "No se encontró el paquete .deb: ${DEB_PATH}" >&2
  exit 1
fi

${SUDO} dpkg -i "${DEB_PATH}"

if [[ ! -x /opt/aymaralang/bin/aymc ]]; then
  echo "No se encontró /opt/aymaralang/bin/aymc tras instalar el paquete." >&2
  exit 1
fi

/opt/aymaralang/bin/aymc -o "${TMP_BASICOS}" "${ROOT_DIR}/samples/fundamentos/basicos.aym"
"${TMP_BASICOS}" > "${OUT_BASICOS}"
if ! grep -Fq "kamisaraki" "${OUT_BASICOS}"; then
  echo "Smoke test basicos.aym no produjo salida esperada." >&2
  exit 1
fi

/opt/aymaralang/bin/aymc -o "${TMP_FLOW}" "${ROOT_DIR}/samples/aymara_flow.aym"
"${TMP_FLOW}" > "${OUT_FLOW}"
if ! grep -Fq "jach'a" "${OUT_FLOW}"; then
  echo "Smoke test aymara_flow.aym no produjo salida esperada." >&2
  exit 1
fi

${SUDO} dpkg --purge "${PKG_NAME}"

if ${SUDO} dpkg -s "${PKG_NAME}" >/dev/null 2>&1; then
  echo "El paquete ${PKG_NAME} sigue instalado tras purge." >&2
  exit 1
fi

if [[ -e /opt/aymaralang/bin/aymc ]] || [[ -e /usr/bin/aymc ]] || [[ -L /usr/bin/aymc ]]; then
  echo "Persisten binarios/enlaces de aymc tras purge." >&2
  exit 1
fi

echo "OK: instalador Linux (.deb) validado (build + install + smoke + purge)."
