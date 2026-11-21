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

## Publicar una versión

La publicación está automatizada. Desde `main`, limpio y sincronizado con
`origin/main`:

```bash
scripts/release.sh 1.1.0
```

El script actualiza `VERSION.txt`, crea el commit `release: v1.1.0` y la
etiqueta `v1.1.0`, y los sube. Al llegar la etiqueta, el workflow
`.github/workflows/release.yml`:

1. Comprueba que la etiqueta coincide con `VERSION.txt`.
2. Ejecuta el CI completo (`ci.yml`): compilación, pruebas e instaladores en
   Linux y Windows.
3. Crea la release de GitHub con `AymaraLang-Setup.exe`,
   `AymaraLang-Setup.msi`, `aymaralang_<versión>_amd64.deb` y
   `SHA256SUMS.txt`.
4. Despliega la web (`web/`) en Cloudflare con los enlaces de esa release
   (`scripts/sync-release.mjs`).

Si un paso falla, la etiqueta queda creada: corrige y relanza el workflow
desde la pestaña *Actions* (la publicación reemplaza los archivos de la
release existente).

### Secretos necesarios

El despliegue de la web necesita dos secretos del repositorio
(*Settings → Secrets and variables → Actions*):

- `CLOUDFLARE_API_TOKEN`: token con la plantilla *Edit Cloudflare Workers*.
- `CLOUDFLARE_ACCOUNT_ID`: ID de la cuenta de Cloudflare.

Sin ellos la release se publica igual, pero el job *Desplegar web* falla con
un aviso explícito.

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
