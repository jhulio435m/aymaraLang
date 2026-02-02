# Compilación, instalación y uso

Si necesitas una guía corta con comandos mínimos, revisa
[Instrucciones rápidas](build-quick.md).

## Compilación y ejecución rápida

```bash
$ ./bin/aymc samples/hola.aym
$ ./bin/hola
Kamisaraki!
```

```bash
$ ./bin/aymc samples/ops.aym
$ ./bin/ops
11
```

```bash
$ ./bin/aymc samples/condloop.aym
$ ./bin/condloop
cond
loop
loop
loop
```

```bash
$ ./bin/aymc samples/vars.aym
$ ./bin/vars
13
```

```bash
$ ./bin/aymc samples/recursion.aym
$ ./bin/recursion
120
```

```bash
$ ./bin/aymc samples/runtime.aym
$ ./bin/runtime
inicio
3
2
1
```

## Instaladores y distribución

Para crear instaladores en Windows, paquetes en Linux o un DMG en macOS usando
CMake + CPack, revisa la guía en [`docs/installers.md`](installers.md).
Incluye pasos para generar instaladores NSIS, paquetes `.deb`/`.rpm` y un DMG.

## Linux

1. Dependencias: `g++` (>= 10), `make`, `nasm`, `gcc`/`ld` para el enlace final y `llvm-config` con las bibliotecas de desarrollo de LLVM (>= 14).
2. Ejecuta `make` para compilar el binario en `bin/aymc`.
3. (Opcional) Lanza `make test` para correr el paquete de pruebas automatizadas.

## macOS

1. Instala las *Command Line Tools* de Xcode:

   ```bash
   xcode-select --install
   ```

   Se recomienda instalar LLVM mediante Homebrew (`brew install llvm`) para habilitar el backend experimental.

2. Instala [Homebrew](https://brew.sh/) si aún no lo tienes disponible.
3. Con Homebrew, instala las dependencias necesarias:

   ```bash
   brew install llvm nasm cmake make
   ```

4. Asegura que el `clang++` de Homebrew esté en tu `PATH` (ajusta la ruta según Apple Silicon `/opt/homebrew` o Intel `/usr/local`):

   ```bash
   export PATH="$(brew --prefix llvm)/bin:$PATH"
   export LDFLAGS="-L$(brew --prefix llvm)/lib"
   export CPPFLAGS="-I$(brew --prefix llvm)/include"
   ```

5. Compila el proyecto usando el *Makefile* específico para macOS:

   ```bash
   make -f Makefile.macos
   ```

   El binario `bin/aymc` quedará listo para compilar programas `.aym` (los ejecutables generados siguen siendo ELF o PE según el modo seleccionado).
6. Como alternativa, puedes usar CMake con `clang++`:

   ```bash
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=$(brew --prefix llvm)/bin/clang++
   cmake --build build -j
   ```

## Windows (resumen)

1. Instala [MinGW-w64](https://www.mingw-w64.org/) (GCC 9 o superior) y `nasm` para Windows.
2. Ejecuta `build.bat` desde la terminal de MinGW para generar `aymc.exe`.
3. Consulta la sección [**Uso en Windows**](#uso-en-windows) para más detalles sobre la ejecución.

## Selección de plataforma

`aymc` permite elegir el sistema operativo de destino. De forma predeterminada se
usa la plataforma actual, pero pueden forzarse los siguientes modos:

- `--windows` – genera un ejecutable de Windows (`.exe`).
- `--linux` – genera un ejecutable para Linux.

### Ejemplo `--windows`

```bash
$ ./bin/aymc --windows samples/hola.aym
```

### Ejemplo `--linux`

```bash
$ ./bin/aymc --linux samples/hola.aym
```

## Semilla de números aleatorios

El compilador acepta `--seed <valor>` para fijar la semilla del generador de
números aleatorios tanto en el intérprete como en los ejecutables generados.
Esto permite reproducir secuencias de `random()` de manera determinista.

```bash
$ ./bin/aymc --seed 42 samples/random.aym
$ ./bin/random
```

## Uso en Windows

1. Instalar [MinGW-w64](https://www.mingw-w64.org/) (GCC 9 o superior) y la versión para Windows de `nasm`.
2. Ejecutar `build.bat` para compilar `aymc.exe`.
   Asegúrate de que `g++ --version` reporte 9 o superior, de lo contrario
   el encabezado `<filesystem>` no estará disponible.
3. Compilar un archivo `.aym` con:

```cmd
> aymc archivo.aym
```

Se generará `bin\archivo.exe` que podrá ejecutarse con doble clic o desde la consola.

---

**Anterior:** [REPL](repl.md) | **Siguiente:** [Instaladores](installers.md)
