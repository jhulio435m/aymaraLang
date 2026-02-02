# Instaladores y binarios

Este documento describe cómo generar instaladores para Windows, Linux y macOS,
además de cómo producir binarios listos para distribución con CPack.

## Requisitos generales

- CMake >= 3.15
- Compilador C++17 (`g++`, `clang++` o MSVC)
- `nasm` y `gcc`/`ld` para el ensamblado y enlace en Linux/Windows (MinGW)

## Flujo base (todas las plataformas)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Los binarios compilados quedarán en `build/bin/aymc`.

## Windows (instalador tipo asistente con NSIS)

1. Instala CMake y el generador de NSIS (https://nsis.sourceforge.io/).
2. Compila con CMake:

   ```powershell
   cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
   cmake --build build -j
   ```

3. Genera el instalador:

   ```powershell
   cd build
   cpack -G NSIS
   ```

El instalador `.exe` ofrece un asistente gráfico similar al de Python/Java,
permite instalar en `Program Files`, crea accesos en el menú Inicio y puede
añadir `aymc` al `PATH`.

## Linux (binarios y paquetes)

1. Compila con CMake.
2. Genera paquetes:

   ```bash
   cd build
   cpack -G TGZ
   cpack -G DEB
   cpack -G RPM
   ```

Los paquetes generados contienen `bin/aymc` y los archivos de runtime en
`share/aymaraLang/runtime`.

## macOS (DMG)

1. Compila con CMake usando `clang++` (Homebrew LLVM recomendado).
2. Genera el DMG:

   ```bash
   cd build
   cpack -G DragNDrop
   ```

El DMG empaqueta el binario `aymc` y el runtime.
