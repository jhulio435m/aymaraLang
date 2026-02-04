# Compilación e instalación

Esta guía resume los comandos recomendados para compilar e instalar
AymaraLang en las plataformas soportadas.

## Requisitos

- **Linux:** `g++` (>=8), `nasm`, `gcc` (para enlazar), `cmake` (>=3.15).
- **Windows:** MinGW-w64 (`g++`), `nasm`, `cmake` (o usa `build.bat`).

### Instalar dependencias en Linux (script)

El repositorio incluye un script para instalar dependencias en distribuciones
basadas en **apt**, **dnf** o **pacman**. Ejecuta desde la raíz del proyecto:

```bash
bash scripts/install_deps_linux.sh
```

Si prefieres darle permisos de ejecución:

```bash
chmod +x scripts/install_deps_linux.sh
./scripts/install_deps_linux.sh
```

> Nota: el script usa `sudo` si no eres `root`.

### Instalar dependencias en Windows (script)

Ejecuta el script de PowerShell desde la raíz del proyecto:

```powershell
./scripts/install_deps_windows.ps1
```

El script intenta usar `winget`, `choco` o `scoop` para instalar CMake, NASM,
NSIS, WiX Toolset y LLVM (opcional). Si no encuentra gestor de paquetes,
mostrará la lista de dependencias a instalar manualmente.

### Generar ejecutables y dist (Windows)

Para empaquetar el compilador y todos los recursos en `dist/`:

```powershell
./scripts/build_dist.ps1 -Config Release
```

Si necesitas compilar con LLVM, habilítalo y especifica la ruta donde está
`LLVMConfig.cmake`:

```powershell
./scripts/build_dist.ps1 -Config Release -EnableLLVM -LLVMDir "C:\\Program Files\\LLVM\\lib\\cmake\\llvm"
```

El ejecutable resultante queda en `dist\bin\aymc.exe`.

### Generar instaladores de Windows (MSI y EXE)

Una vez que `dist/` está listo, genera los instaladores **por separado**. No
hay un script `build_all.ps1`; ejecuta el que corresponda al instalador que
necesitas:

```powershell
./scripts/build_msi.ps1
./scripts/build_nsis.ps1
```

El MSI se guarda en `artifacts/AymaraLang-Setup.msi` y el EXE en
`artifacts/AymaraLang-Setup.exe`.

> Nota: ambos instaladores usan `assets/logo.ico` como icono. El build de NSIS
> puede tardar varios minutos dependiendo del antivirus y del tamaño de `dist/`.
> Si el MSI no genera el archivo final, revisa primero los errores de WiX en la
> salida (la compilación puede fallar y no producir ningún `.msi`).

### Generar paquete `.deb` (Linux)

1. Construye e instala en `dist/`:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
cmake --install build --prefix dist
```

2. Empaqueta el `.deb`:

```bash
bash scripts/build_deb.sh
```

El paquete se guarda en `artifacts/`.

### Generar el paquete `.tar.gz` para GitHub

Para que los usuarios descarguen el código fuente desde GitHub (por ejemplo,
adjuntándolo a un Release), puedes generar un tarball con `git archive`:

```bash
git archive --format=tar.gz --prefix=aymaraLang/ -o aymaraLang.tar.gz HEAD
```

El archivo `aymaraLang.tar.gz` puede subirse como **asset** de un Release en
GitHub.

## CMake (recomendado, multiplataforma)

### Linux/macOS

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Binario resultante: `build/bin/aymc`.

### Windows (MinGW Makefiles)

```bash
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Binario resultante: `build/bin/aymc.exe`.

> Nota: si usas el generador de Visual Studio, el binario queda en
> `build\bin\Release\aymc.exe` (o en la carpeta que corresponda a la
> configuración seleccionada).

## Uso del compilador

### Linux/macOS

```bash
./build/bin/aymc samples/aymara_flow.aym
./samples/aymara_flow
```

Por defecto, el ejecutable generado se guarda en la misma carpeta del archivo
`.aym` de entrada. Puedes cambiarlo con `-o` si lo necesitas.

### Windows

```cmd
build\\bin\\aymc.exe samples\\aymara_flow.aym
samples\\aymara_flow.exe
```

### Distribuir solo el binario del compilador

Si vas a compartir únicamente el binario de `aymc`, asegúrate de incluir la
carpeta `runtime` junto al ejecutable. El compilador busca esos archivos
relativos a su propia ubicación para poder enlazar los programas generados.

## Alternativas legacy

- **Linux:** `make`
- **Windows:** `build.bat`
