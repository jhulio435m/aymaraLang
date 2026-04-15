# Compilación e instalación desde fuente

Guía técnica para construir AymaraLang desde el repositorio.

## Requisitos

- CMake >= 3.15
- Compilador C++17 (`g++`, `clang++` o MSVC)
- NASM
- GCC/LD o MinGW según plataforma
- En Linux, headers y librería de X11 (`pkg-config --exists x11`)

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

Este script configura, compila e instala en `dist/`, embebe la toolchain
portable de Windows y, si hay certificado configurado, firma automáticamente
`dist\bin\aym.exe` y `dist\bin\aymc.exe`.

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

Los scripts de empaquetado también firman automáticamente `AymaraLang-Setup.msi`
y `AymaraLang-Setup.exe` si encuentran configuración de firma.

Configuración soportada para firma automática:

- `AYM_SIGN_PFX_PATH`
- `AYM_SIGN_PFX_PASSWORD`
- `AYM_SIGN_CERT_THUMBPRINT`
- `AYM_SIGN_TIMESTAMP_URL`
- `AYM_SIGNTOOL_PATH`
- `AYM_SIGN_CERT_MACHINE_STORE=1`

Para exigir firma y abortar si falta certificado:

```powershell
pwsh -File .\scripts\build\build_dist.ps1 -Config Release -RequireSigning
pwsh -File .\scripts\build\build_nsis.ps1 -DistDir dist -OutputDir artifacts\release-windows -RequireSigning
pwsh -File .\scripts\build\build_msi.ps1 -DistDir dist -OutputDir artifacts\release-windows -RequireSigning
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
- `dist-bundled/`: variante local con toolchain Windows embebida.
- `artifacts/`: paquetes de distribución (MSI, EXE, DEB).
- `artifacts/release-upload/`: carpeta preparada para subir archivos finales al
  GitHub Release.
- `build-*`: directorios de compilación.
