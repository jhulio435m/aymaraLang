#!/usr/bin/env bash
set -euo pipefail

log() {
  printf '%s\n' "$*"
}

if [[ "${EUID:-$(id -u)}" -eq 0 ]]; then
  SUDO=""
else
  SUDO="sudo"
fi

if command -v apt-get >/dev/null 2>&1; then
  log "Detected apt-get (Debian/Ubuntu). Installing dependencies..."
  $SUDO apt-get update
  $SUDO apt-get install -y \
    build-essential \
    cmake \
    nasm \
    llvm \
    libffi-dev \
    libedit-dev \
    zlib1g-dev \
    libzstd-dev \
    libxml2-dev \
    libcurl4-openssl-dev
elif command -v dnf >/dev/null 2>&1; then
  log "Detected dnf (Fedora/RHEL). Installing dependencies..."
  $SUDO dnf install -y \
    gcc-c++ \
    make \
    cmake \
    nasm \
    llvm \
    llvm-devel \
    libffi-devel \
    libedit-devel \
    zlib-devel \
    zstd-devel \
    libxml2-devel \
    libcurl-devel
elif command -v pacman >/dev/null 2>&1; then
  log "Detected pacman (Arch). Installing dependencies..."
  $SUDO pacman -Syu --noconfirm \
    base-devel \
    cmake \
    nasm \
    llvm \
    libffi \
    libedit \
    zlib \
    zstd \
    libxml2 \
    curl
else
  log "Unsupported package manager. Install dependencies manually:"
  log " - C++ compiler (g++)"
  log " - cmake"
  log " - nasm"
  log " - llvm"
  log " - libffi"
  log " - libedit"
  log " - zlib"
  log " - zstd"
  log " - libxml2"
  log " - libcurl"
  exit 1
fi

log "Done. You can now build with:"
log "  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release"
log "  cmake --build build -j"
