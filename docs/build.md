# Compilación e instalación desde fuente

Guía técnica para construir AymaraLang desde el repositorio.

## Requisitos

- CMake >= 3.15
- Compilador C++17 (`g++`, `clang++` o MSVC)
- NASM
- GCC/LD o MinGW según plataforma

## Compilación recomendada

### Linux

```bash
cmake -S . -B build-linux -DCMAKE_BUILD_TYPE=Release
cmake --build build-linux -j
cmake --install build-linux --prefix dist
```

### Windows (Visual Studio 2022)

```powershell
pwsh -File .\scripts\build\build_dist.ps1 -BuildDir build-win -Config Release
```

Este script configura, compila e instala en `dist/`.

## Binarios esperados

En `dist/bin/`:

- `aymc` o `aymc.exe`
- `aym` o `aym.exe`

## Empaquetado

### Windows

```powershell
pwsh -File .\scripts\build\build_msi.ps1
pwsh -File .\scripts\build\build_nsis.ps1
```

### Linux (.deb)

```bash
bash scripts/install/install_deps_linux.sh --with-deb-tools
bash scripts/build/build_deb.sh
```

## Validaciones

### Linux

```bash
bash scripts/test/test_deb.sh
bash scripts/test/test_deb_e2e.sh
bash scripts/test/test_samples.sh
```

### Windows

- Validar build local con `aymc --help` y compilación de un `sample`.
- Validar instaladores con instalación, smoke y desinstalación.

## Estructura de salida

- `dist/`: árbol instalable.
- `artifacts/`: paquetes de distribución (MSI, EXE, DEB).
- `build-*`: directorios de compilación.
