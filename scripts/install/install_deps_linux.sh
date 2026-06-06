#!/usr/bin/env bash
set -euo pipefail

log() {
  printf '[INFO] %s\n' "$*"
}

warn() {
  printf '[WARN] %s\n' "$*" >&2
}

error() {
  printf '[ERROR] %s\n' "$*" >&2
}

usage() {
  cat <<'EOF'
Usage: install_deps_linux.sh [options]

Options:
  --check-only                  Solo valida dependencias (no instala)
  --package-manager <pm>        Forzar gestor: auto|apt|dnf|yum|pacman|zypper|apk
  --with-deb-tools              Instalar herramientas para empaquetado .deb (dpkg-deb, fakeroot)
  -h, --help                    Mostrar ayuda
EOF
}

CHECK_ONLY=0
PACKAGE_MANAGER="auto"
WITH_DEB_TOOLS=0

while [ "$#" -gt 0 ]; do
  case "$1" in
    --check-only)
      CHECK_ONLY=1
      ;;
    --package-manager)
      shift
      [ "$#" -gt 0 ] || {
        error "Falta valor para --package-manager"
        usage
        exit 1
      }
      PACKAGE_MANAGER="$1"
      ;;
    --with-deb-tools)
      WITH_DEB_TOOLS=1
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      error "Opcion desconocida: $1"
      usage
      exit 1
      ;;
  esac
  shift
done

require_sudo_if_needed() {
  if [ "$(id -u)" -eq 0 ]; then
    SUDO=()
    return 0
  fi

  if command -v sudo >/dev/null 2>&1; then
    SUDO=(sudo)
    return 0
  fi

  error "Se requiere privilegio root para instalar dependencias. Ejecuta como root o instala sudo."
  exit 1
}

detect_package_manager() {
  if [ "$PACKAGE_MANAGER" != "auto" ]; then
    case "$PACKAGE_MANAGER" in
      apt|dnf|yum|pacman|zypper|apk)
        ;;
      *)
        error "Gestor no soportado: $PACKAGE_MANAGER"
        exit 1
        ;;
    esac
    printf '%s\n' "$PACKAGE_MANAGER"
    return 0
  fi

  if command -v apt-get >/dev/null 2>&1; then
    printf 'apt\n'
    return 0
  fi
  if command -v dnf >/dev/null 2>&1; then
    printf 'dnf\n'
    return 0
  fi
  if command -v yum >/dev/null 2>&1; then
    printf 'yum\n'
    return 0
  fi
  if command -v pacman >/dev/null 2>&1; then
    printf 'pacman\n'
    return 0
  fi
  if command -v zypper >/dev/null 2>&1; then
    printf 'zypper\n'
    return 0
  fi
  if command -v apk >/dev/null 2>&1; then
    printf 'apk\n'
    return 0
  fi

  printf 'unknown\n'
  return 0
}

pm_update() {
  case "$PM" in
    apt)
      "${SUDO[@]}" apt-get update
      ;;
    dnf)
      "${SUDO[@]}" dnf makecache -y
      ;;
    yum)
      "${SUDO[@]}" yum makecache -y
      ;;
    pacman)
      "${SUDO[@]}" pacman -Sy --noconfirm
      ;;
    zypper)
      "${SUDO[@]}" zypper --non-interactive refresh
      ;;
    apk)
      "${SUDO[@]}" apk update
      ;;
    *)
      error "pm_update: gestor no soportado: $PM"
      exit 1
      ;;
  esac
}

pm_install() {
  local pkg="$1"
  case "$PM" in
    apt)
      DEBIAN_FRONTEND=noninteractive "${SUDO[@]}" apt-get install -y "$pkg"
      ;;
    dnf)
      "${SUDO[@]}" dnf install -y "$pkg"
      ;;
    yum)
      "${SUDO[@]}" yum install -y "$pkg"
      ;;
    pacman)
      "${SUDO[@]}" pacman -S --noconfirm --needed "$pkg"
      ;;
    zypper)
      "${SUDO[@]}" zypper --non-interactive install --no-recommends "$pkg"
      ;;
    apk)
      "${SUDO[@]}" apk add --no-cache "$pkg"
      ;;
    *)
      error "pm_install: gestor no soportado: $PM"
      exit 1
      ;;
  esac
}

install_any() {
  local label="$1"
  shift

  local candidate
  for candidate in "$@"; do
    if [ -z "$candidate" ]; then
      continue
    fi

    log "Instalando ${label} (${candidate})..."
    if pm_install "$candidate"; then
      log "${label} instalado (${candidate})."
      return 0
    fi
    warn "Fallo al instalar ${candidate} para ${label}, probando alternativa..."
  done

  error "No se pudo instalar ${label} con ninguna alternativa: $*"
  return 1
}

check_required_tools() {
  local missing=()
  local cmd
  for cmd in cmake nasm gcc g++; do
    if ! command -v "$cmd" >/dev/null 2>&1; then
      missing+=("$cmd")
    fi
  done

  if [ "${#missing[@]}" -gt 0 ]; then
    error "Faltan dependencias requeridas en PATH: ${missing[*]}"
    return 1
  fi

  if ! command -v pkg-config >/dev/null 2>&1; then
    missing+=("pkg-config")
  elif ! pkg-config --exists x11; then
    missing+=("x11-dev")
  fi

  if [ "${#missing[@]}" -gt 0 ]; then
    error "Faltan dependencias requeridas en PATH/sistema: ${missing[*]}"
    return 1
  fi

  log "Dependencias requeridas disponibles: cmake, nasm, gcc, g++, pkg-config, X11"
  return 0
}

check_deb_tools() {
  local missing=()
  local cmd
  for cmd in dpkg-deb fakeroot; do
    if ! command -v "$cmd" >/dev/null 2>&1; then
      missing+=("$cmd")
    fi
  done

  if [ "${#missing[@]}" -gt 0 ]; then
    warn "Faltan herramientas opcionales para .deb: ${missing[*]}"
    return 1
  fi

  log "Herramientas .deb disponibles: dpkg-deb, fakeroot"
  return 0
}

install_for_pm() {
  case "$PM" in
    apt)
      install_any "toolchain C/C++" build-essential
      install_any "CMake" cmake
      install_any "NASM" nasm
      install_any "pkg-config" pkg-config
      install_any "X11 headers" libx11-dev
      install_any "curl" curl
      install_any "ca-certificates" ca-certificates
      if [ "$WITH_DEB_TOOLS" -eq 1 ]; then
        install_any "dpkg-deb" dpkg-dev dpkg
        install_any "fakeroot" fakeroot
      fi
      ;;
    dnf)
      install_any "toolchain C/C++" gcc-c++ gcc
      install_any "make" make
      install_any "CMake" cmake
      install_any "NASM" nasm
      install_any "pkg-config" pkgconf-pkg-config pkgconf
      install_any "X11 headers" libX11-devel
      install_any "curl" curl
      install_any "ca-certificates" ca-certificates
      if [ "$WITH_DEB_TOOLS" -eq 1 ]; then
        install_any "dpkg-deb" dpkg
        install_any "fakeroot" fakeroot
      fi
      ;;
    yum)
      install_any "toolchain C/C++" gcc-c++ gcc
      install_any "make" make
      install_any "CMake" cmake cmake3
      install_any "NASM" nasm
      install_any "pkg-config" pkgconfig pkgconf-pkg-config pkgconf
      install_any "X11 headers" libX11-devel
      install_any "curl" curl
      install_any "ca-certificates" ca-certificates
      if [ "$WITH_DEB_TOOLS" -eq 1 ]; then
        install_any "dpkg-deb" dpkg
        install_any "fakeroot" fakeroot
      fi
      ;;
    pacman)
      install_any "toolchain C/C++" base-devel gcc
      install_any "CMake" cmake
      install_any "NASM" nasm
      install_any "pkg-config" pkgconf
      install_any "X11 headers" libx11
      install_any "curl" curl
      install_any "ca-certificates" ca-certificates
      if [ "$WITH_DEB_TOOLS" -eq 1 ]; then
        install_any "dpkg-deb" dpkg
        install_any "fakeroot" fakeroot
      fi
      ;;
    zypper)
      install_any "toolchain C/C++" gcc-c++ gcc
      install_any "make" make
      install_any "CMake" cmake
      install_any "NASM" nasm
      install_any "pkg-config" pkg-config pkgconf-pkg-config
      install_any "X11 headers" libX11-devel
      install_any "curl" curl
      install_any "ca-certificates" ca-certificates
      if [ "$WITH_DEB_TOOLS" -eq 1 ]; then
        install_any "dpkg-deb" dpkg
        install_any "fakeroot" fakeroot
      fi
      ;;
    apk)
      install_any "toolchain C/C++" build-base gcc g++
      install_any "CMake" cmake
      install_any "NASM" nasm
      install_any "pkg-config" pkgconf
      install_any "X11 headers" libx11-dev
      install_any "curl" curl
      install_any "ca-certificates" ca-certificates
      if [ "$WITH_DEB_TOOLS" -eq 1 ]; then
        install_any "dpkg-deb" dpkg
        install_any "fakeroot" fakeroot
      fi
      ;;
    *)
      error "Gestor no soportado: $PM"
      exit 1
      ;;
  esac
}

PM="$(detect_package_manager)"
if [ "$PM" = "unknown" ]; then
  error "No se detecto un gestor soportado (apt/dnf/yum/pacman/zypper/apk)."
  error "Instala manualmente: cmake, nasm, gcc, g++, pkg-config, headers/lib de X11."
  exit 1
fi

log "Gestor detectado: $PM"

if [ "$CHECK_ONLY" -eq 1 ]; then
  check_required_tools
  if [ "$WITH_DEB_TOOLS" -eq 1 ]; then
    check_deb_tools
  fi
  log "Check completado."
  exit 0
fi

require_sudo_if_needed
pm_update
install_for_pm

check_required_tools
if [ "$WITH_DEB_TOOLS" -eq 1 ]; then
  check_deb_tools
fi

log "Done. You can now build with:"
log "  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release"
log "  cmake --build build -j"
