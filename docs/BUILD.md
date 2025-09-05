Build instructions

- Prerequisites:
  - Linux: `g++` (>=8), `nasm`, `gcc` (for linking), `cmake` (>=3.15)
  - Windows: MinGW-w64 (`g++`), `nasm`, `cmake` (or use `build.bat`)

- CMake (cross‑platform):
  - Linux/macOS:
    - `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`
    - `cmake --build build -j`
    - Binary at `build/bin/aymc`
  - Windows (MinGW Makefiles):
    - `cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release`
    - `cmake --build build -j`
    - Binary at `build/bin/aymc.exe`

- Legacy:
  - Linux: `make`
  - Windows: `build.bat`

