# Compilación e instalación

Esta guía resume los comandos recomendados para compilar e instalar
AymaraLang en las plataformas soportadas.

## Requisitos

- **Linux:** `g++` (>=8), `nasm`, `gcc` (para enlazar), `cmake` (>=3.15).
- **Windows:** MinGW-w64 (`g++`), `nasm`, `cmake` (o usa `build.bat`).

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

## Uso del compilador

### Linux/macOS

```bash
./build/bin/aymc samples/basics/hola.aym
./build/bin/hola
```

### Windows

```cmd
build\\bin\\aymc.exe samples\\basics\\hola.aym
bin\\hola.exe
```

## Alternativas legacy

- **Linux:** `make`
- **Windows:** `build.bat`
