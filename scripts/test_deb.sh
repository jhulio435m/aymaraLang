#!/bin/sh
set -e

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
PKG_NAME="aymaralang"
ARCH="amd64"
VERSION_FILE="${ROOT_DIR}/VERSION.txt"
VERSION="0.1.0"
BUILD_DIR="${ROOT_DIR}/build"
DIST_DIR="${ROOT_DIR}/dist"
BUILD_TYPE="${BUILD_TYPE:-Release}"

if [ -f "${VERSION_FILE}" ]; then
  VERSION=$(cat "${VERSION_FILE}" | tr -d '\n\r')
fi

if ! command -v dpkg-deb >/dev/null 2>&1; then
  echo "dpkg-deb no está disponible. Instala dpkg (Debian/Ubuntu) o dpkg-deb." >&2
  exit 1
fi

if [ ! -d "${DIST_DIR}" ] && [ ! -d "${BUILD_DIR}" ]; then
  if ! command -v cmake >/dev/null 2>&1; then
    echo "cmake no está disponible para generar el build necesario." >&2
    exit 1
  fi
  echo "No se encontró build ni dist. Generando build (BUILD_TYPE=${BUILD_TYPE})..."
  cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"
  cmake --build "${BUILD_DIR}" -j
fi

"${ROOT_DIR}/scripts/build_deb.sh"

DEB_PATH="${ROOT_DIR}/artifacts/${PKG_NAME}_${VERSION}_${ARCH}.deb"
if [ ! -f "${DEB_PATH}" ]; then
  echo "No se encontró el paquete generado en ${DEB_PATH}." >&2
  exit 1
fi

dpkg-deb --info "${DEB_PATH}" | grep -q "Package: ${PKG_NAME}"

CONTENTS=$(dpkg-deb --contents "${DEB_PATH}")
echo "${CONTENTS}" | grep -q "./opt/${PKG_NAME}/bin/aymc"
echo "${CONTENTS}" | grep -q "./opt/${PKG_NAME}/share/aymaraLang/runtime/"

echo "OK: paquete .deb válido en ${DEB_PATH}"
