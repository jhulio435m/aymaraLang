#!/bin/sh
set -e

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
DIST_DIR="${ROOT_DIR}/dist"
BUILD_DIR="${ROOT_DIR}/build"
ARTIFACTS_DIR="${ROOT_DIR}/artifacts"
PKG_NAME="aymaralang"
ARCH="amd64"
ICON_ICO_FILE="${ROOT_DIR}/assets/logo.ico"
ICON_PNG_FILE="${ROOT_DIR}/assets/logo.png"
MIME_XML_FILE="${ROOT_DIR}/packaging/deb/aymaralang.xml"
BUILD_TYPE="${BUILD_TYPE:-Release}"

if ! command -v dpkg-deb >/dev/null 2>&1; then
  echo "dpkg-deb no está disponible." >&2
  echo "Instala herramientas .deb con: bash scripts/install/install_deps_linux.sh --with-deb-tools" >&2
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

if [ ! -f "${DIST_DIR}/bin/aymc" ] || [ -f "${DIST_DIR}/bin/aymc.exe" ]; then
  echo "Dist inválido para Linux en ${DIST_DIR}/bin." >&2
  echo "Se esperaba 'aymc' (sin .exe). Regenera dist desde Linux/WSL." >&2
  exit 1
fi
if [ ! -f "${ICON_ICO_FILE}" ]; then
  echo "No se encontró ${ICON_ICO_FILE}. Verifica el icono en assets/." >&2
  exit 1
fi
if [ ! -f "${ICON_PNG_FILE}" ]; then
  echo "No se encontró ${ICON_PNG_FILE}. Verifica el icono PNG en assets/." >&2
  exit 1
fi
if [ ! -f "${MIME_XML_FILE}" ]; then
  echo "No se encontró ${MIME_XML_FILE}. Verifica el descriptor MIME en packaging/deb/." >&2
  exit 1
fi
RUNTIME_SRC_DIR="${ROOT_DIR}/runtime"
RUNTIME_DIST_DIR="${DIST_DIR}/share/aymaraLang/runtime"
if [ ! -d "${RUNTIME_DIST_DIR}" ]; then
  if [ ! -d "${RUNTIME_SRC_DIR}" ]; then
    echo "No se encontró ${RUNTIME_SRC_DIR}. Verifica los archivos runtime." >&2
    exit 1
  fi
  echo "No se encontró runtime en dist. Copiando ${RUNTIME_SRC_DIR} a ${RUNTIME_DIST_DIR}..."
  mkdir -p "${RUNTIME_DIST_DIR}"
  cp -R "${RUNTIME_SRC_DIR}/." "${RUNTIME_DIST_DIR}/"
fi

VERSION_FILE="${ROOT_DIR}/VERSION.txt"
VERSION="0.1.0"
if [ -f "${VERSION_FILE}" ]; then
  VERSION=$(cat "${VERSION_FILE}" | tr -d '\n\r')
fi

STAGING_ROOT=$(mktemp -d "${TMPDIR:-/tmp}/${PKG_NAME}_pkg.XXXXXX")
cleanup() {
  rm -rf "${STAGING_ROOT}"
}
trap cleanup EXIT INT TERM

PKG_DIR="${STAGING_ROOT}/${PKG_NAME}_${VERSION}_${ARCH}"
DEBIAN_DIR="${PKG_DIR}/DEBIAN"

mkdir -p "${DEBIAN_DIR}"
mkdir -p "${PKG_DIR}/opt/${PKG_NAME}"
mkdir -p "${PKG_DIR}/usr/share/applications"
mkdir -p "${PKG_DIR}/usr/share/icons/hicolor/256x256/apps"
mkdir -p "${PKG_DIR}/usr/share/icons/hicolor/256x256/mimetypes"
mkdir -p "${PKG_DIR}/usr/share/mime/packages"
mkdir -p "${PKG_DIR}/usr/share/pixmaps"

cp -R "${DIST_DIR}/." "${PKG_DIR}/opt/${PKG_NAME}/"
cp "${ICON_ICO_FILE}" "${PKG_DIR}/usr/share/pixmaps/${PKG_NAME}.ico"
cp "${ICON_PNG_FILE}" "${PKG_DIR}/usr/share/icons/hicolor/256x256/apps/${PKG_NAME}.png"
cp "${ICON_PNG_FILE}" "${PKG_DIR}/usr/share/icons/hicolor/256x256/mimetypes/text-x-aymara.png"
cp "${ROOT_DIR}/packaging/deb/aymaralang.desktop" "${PKG_DIR}/usr/share/applications/aymaralang.desktop"
cp "${MIME_XML_FILE}" "${PKG_DIR}/usr/share/mime/packages/aymaralang.xml"

CONTROL_TEMPLATE="${ROOT_DIR}/packaging/deb/control"
sed "s/^Version:.*/Version: ${VERSION}/" "${CONTROL_TEMPLATE}" > "${DEBIAN_DIR}/control"
cp "${ROOT_DIR}/packaging/deb/postinst" "${DEBIAN_DIR}/postinst"
cp "${ROOT_DIR}/packaging/deb/postrm" "${DEBIAN_DIR}/postrm"
# Force LF for maintainer scripts; CRLF breaks execution in dpkg.
sed -i 's/\r$//' "${DEBIAN_DIR}/postinst" "${DEBIAN_DIR}/postrm"

chmod 755 "${DEBIAN_DIR}/postinst" "${DEBIAN_DIR}/postrm"
find "${PKG_DIR}" -type d -exec chmod 755 {} \;

mkdir -p "${ARTIFACTS_DIR}"
dpkg-deb --build "${PKG_DIR}" "${ARTIFACTS_DIR}/${PKG_NAME}_${VERSION}_${ARCH}.deb"

echo "Paquete generado en: ${ARTIFACTS_DIR}/${PKG_NAME}_${VERSION}_${ARCH}.deb"
