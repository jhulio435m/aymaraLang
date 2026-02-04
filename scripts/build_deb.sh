#!/bin/sh
set -e

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
DIST_DIR="${ROOT_DIR}/dist"
BUILD_DIR="${ROOT_DIR}/build"
ARTIFACTS_DIR="${ROOT_DIR}/artifacts"
PKG_NAME="aymaralang"
ARCH="amd64"
ICON_FILE="${ROOT_DIR}/assets/logo.ico"
BUILD_TYPE="${BUILD_TYPE:-Release}"

if ! command -v dpkg-deb >/dev/null 2>&1; then
  echo "dpkg-deb no está disponible. Instala dpkg (Debian/Ubuntu) o dpkg-deb." >&2
  exit 1
fi

if [ ! -d "${DIST_DIR}" ]; then
  if ! command -v cmake >/dev/null 2>&1; then
    echo "No se encontró ${DIST_DIR} y cmake no está disponible para generar dist." >&2
    exit 1
  fi
  if [ ! -d "${BUILD_DIR}" ]; then
    echo "No se encontró ${DIST_DIR} ni ${BUILD_DIR}. Ejecuta: cmake -S . -B build -DCMAKE_BUILD_TYPE=${BUILD_TYPE}" >&2
    exit 1
  fi
  echo "No se encontró ${DIST_DIR}. Generando dist desde ${BUILD_DIR} (BUILD_TYPE=${BUILD_TYPE})..."
  cmake --install "${BUILD_DIR}" --config "${BUILD_TYPE}" --prefix "${DIST_DIR}"
fi
if [ ! -f "${ICON_FILE}" ]; then
  echo "No se encontró ${ICON_FILE}. Verifica el icono en assets/." >&2
  exit 1
fi

VERSION_FILE="${ROOT_DIR}/VERSION.txt"
VERSION="0.1.0"
if [ -f "${VERSION_FILE}" ]; then
  VERSION=$(cat "${VERSION_FILE}" | tr -d '\n\r')
fi

PKG_DIR="${ARTIFACTS_DIR}/${PKG_NAME}_${VERSION}_${ARCH}"
DEBIAN_DIR="${PKG_DIR}/DEBIAN"

rm -rf "${PKG_DIR}"
mkdir -p "${DEBIAN_DIR}"
mkdir -p "${PKG_DIR}/opt/${PKG_NAME}"
mkdir -p "${PKG_DIR}/usr/share/applications"
mkdir -p "${PKG_DIR}/usr/share/pixmaps"

cp -R "${DIST_DIR}/." "${PKG_DIR}/opt/${PKG_NAME}/"
cp "${ICON_FILE}" "${PKG_DIR}/usr/share/pixmaps/${PKG_NAME}.ico"
cp "${ROOT_DIR}/packaging/deb/aymaralang.desktop" "${PKG_DIR}/usr/share/applications/aymaralang.desktop"

CONTROL_TEMPLATE="${ROOT_DIR}/packaging/deb/control"
sed "s/^Version:.*/Version: ${VERSION}/" "${CONTROL_TEMPLATE}" > "${DEBIAN_DIR}/control"
cp "${ROOT_DIR}/packaging/deb/postinst" "${DEBIAN_DIR}/postinst"
cp "${ROOT_DIR}/packaging/deb/postrm" "${DEBIAN_DIR}/postrm"

chmod 755 "${DEBIAN_DIR}/postinst" "${DEBIAN_DIR}/postrm"

mkdir -p "${ARTIFACTS_DIR}"
dpkg-deb --build "${PKG_DIR}" "${ARTIFACTS_DIR}/${PKG_NAME}_${VERSION}_${ARCH}.deb"

echo "Paquete generado en: ${ARTIFACTS_DIR}/${PKG_NAME}_${VERSION}_${ARCH}.deb"
