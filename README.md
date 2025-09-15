# AymaraLang (`aym`) 🇵🇪

**AymaraLang** es un lenguaje de programación moderno basado en la lengua originaria aymara. Su compilador, `aymc`, ha sido desarrollado desde cero en **C++17**, y permite generar ejecutables nativos `.ayn`. El proyecto busca promover la inclusión tecnológica, la educación y la preservación lingüística.

Palabras clave principales del lenguaje:

- `jach’a`, `lliphiphi`, `qillqa`, `chuymani` – tipos primitivos (int, float, string, bool)
- `willt’aña` – salida por pantalla
- `input` – lectura de consola
- `luräwi` / `kutiyana` – definición de funciones y retorno
- `si`, `sino`, `mientras`, `haceña`, `para`, `tantachaña`
- `apu` – importación de módulos desde otros archivos

---

## 📚 Documentación

- [Gramática formal del lenguaje](docs/grammar.md)

---

## 🔠 Identidad del Lenguaje

| Elemento                | Valor                              |
|-------------------------|------------------------------------|
| **Nombre del lenguaje** | `aym`                              |
| **Compilador**          | `aymc`                             |
| **Extensión fuente**    | `.aym`                             |
| **Ejecutable final**    | `.ayn`                             |
| **Inspiración**         | Python (sintaxis), C++ (backend)   |
| **Paradigmas**          | Imperativo, orientado a objetos    |
| **Tipado**              | Estático, fuerte                   |

---

## 🎯 Objetivos del Proyecto

- Crear un lenguaje expresivo utilizando aymara como idioma principal.
- Implementar un compilador autónomo y multiplataforma sin dependencias externas.
- Promover la enseñanza de programación en comunidades originarias.
- Servir como base cultural y técnica para proyectos educativos y lingüísticos.

---

## 🏗 Arquitectura del Compilador

El compilador `aymc` está estructurado en varias etapas clásicas de diseño de compiladores:

1. **Análisis Léxico** – Tokenización de entrada `.aym`.
2. **Análisis Sintáctico** – Construcción del árbol de derivación según la gramática LL(k).
3. **Construcción del AST** – Representación semántica abstracta.
4. **Análisis Semántico** – Tipado, resolución de símbolos, validaciones.
5. **Optimización Intermedia** – (opcional) Reescritura del AST para mejoras.
6. **Generación de Código** – Código ensamblador x86_64 o LLVM IR (backend experimental).
7. **Ensamblado y Enlace** – Uso de `nasm` y `gcc` para crear `.ayn`.

Las estructuras ahora incluyen `else`, ciclos `for` y funciones simples.

Las condiciones y bucles ahora se ejecutan en tiempo de ejecución gracias a un
AST más completo, análisis semántico y generación de código en ensamblador.

> ⚙️ El backend LLVM está disponible de forma experimental mediante `aymc --llvm`.

---

## Características del Lenguaje

AymaraLang incluye un conjunto de construcciones inspiradas en Python pero con
palabras clave en aymara. Entre ellas:

```aymara
// variables
jach’a contador = 3;
qillqa saludo = "kamisaraki";

// condicional
si (contador > 0) {
    willt’aña(saludo);
}

// bucle for
para i en range(0, 3) {
    willt’aña(i);
}

luräwi inc(n) {
    kutiyana n + 1;
}
```

Las funciones integradas `input()` y `willt’aña()` permiten entrada/salida
sencilla y `tantachaña` ofrece un control tipo `switch`.

## 🧪 Ejemplo de Código

### `hola.aym`
```aymara
willt’aña("Kamisaraki!");
```

### `ops.aym`
```aymara
willt’aña(3 + 4 * 2);
```

### `condloop.aym`
```aymara
si (1) {
    willt’aña("cond");
}

mientras (3) {
    willt’aña("loop");
}
```

### `vars.aym`
```aymara
x = 5;
y = x * 2 + 3;
willt’aña(y);
```

### `recursion.aym`
```aymara
luräwi fact(n) {
    si (n == 0) {
        kutiyana(1);
    }
    kutiyana(n * fact(n - 1));
}

willt’aña(fact(5));
```

### `module_demo.aym`
```aymara
apu "modules/aritmetica";

jach’a base = 10;
jach’a incremento = 5;
willt’aña("suma: " + suma(base, incremento));
willt’aña("resta: " + resta(base, incremento));
```


### Compilación y Ejecución

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

## 🛠️ Construcción del compilador

### Backend LLVM experimental

El backend basado en LLVM IR se puede invocar añadiendo la bandera `--llvm` al compilador.
Genera un archivo `.ll` con un módulo LLVM que describe de forma resumida el programa analizado.

```bash
$ ./bin/aymc --llvm samples/hola.aym
$ cat build/hola.ll
```

El IR generado imprime por consola un resumen del AST, útil para validar la integración con LLVM antes de ampliar el backend.

### Linux

1. Dependencias: `g++` (>= 10), `make`, `nasm`, `gcc`/`ld` para el enlace final y `llvm-config` con las bibliotecas de desarrollo de LLVM (>= 14).
2. Ejecuta `make` para compilar el binario en `bin/aymc`.
3. (Opcional) Lanza `make test` para correr el paquete de pruebas automatizadas.

### macOS

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

### Windows (resumen)

1. Instala [MinGW-w64](https://www.mingw-w64.org/) (GCC 9 o superior) y `nasm` para Windows.
2. Ejecuta `build.bat` desde la terminal de MinGW para generar `aymc.exe`.
3. Consulta la sección [**Uso en Windows**](#uso-en-windows) para más detalles sobre la ejecución.

### Selección de plataforma

`aymc` permite elegir el sistema operativo de destino. De forma predeterminada se
usa la plataforma actual, pero pueden forzarse los siguientes modos:

- `--windows` – genera un ejecutable de Windows (`.exe`).
- `--linux` – genera un ejecutable para Linux.

#### Ejemplo `--windows`

```bash
$ ./bin/aymc --windows samples/hola.aym
```

#### Ejemplo `--linux`

```bash
$ ./bin/aymc --linux samples/hola.aym
```

### Semilla de números aleatorios

El compilador acepta `--seed <valor>` para fijar la semilla del generador de
números aleatorios tanto en el intérprete como en los ejecutables generados.
Esto permite reproducir secuencias de `random()` de manera determinista.

```bash
$ ./bin/aymc --seed 42 samples/random.aym
$ ./bin/random
```

### Uso en Windows

1. Instalar [MinGW-w64](https://www.mingw-w64.org/) (GCC 9 o superior) y la versión para Windows de `nasm`.
2. Ejecutar `build.bat` para compilar `aymc.exe`.
   Asegúrate de que `g++ --version` reporte 9 o superior, de lo contrario
   el encabezado `<filesystem>` no estará disponible.
3. Compilar un archivo `.aym` con:

```cmd
> aymc archivo.aym
```

Se generará `bin\archivo.exe` que podrá ejecutarse con doble clic o desde la consola.

### Modo REPL

El compilador incluye un modo interactivo que permite ejecutar código línea por línea:

```bash
$ ./bin/aymc --repl
AymaraLang REPL - escribe código línea por línea (escribe 'salir' para terminar)
aym> jach’a x = 5;
aym> x + 2
7
aym> salir
```


### Errores comunes

- **Variable no declarada:** usar una variable sin declararla mostrará un mensaje `Error: variable 'x' no declarada`.
- **`break` fuera de ciclo:** si se usa `break` fuera de `mientras`, `para` o `tantachaña` se emitirá `Error: 'break' fuera de un ciclo o switch`.

---

## 🧰 Tecnologías y Herramientas

* **Lenguaje:** C++17
* **Arquitectura objetivo:** x86\_64 Linux/Windows
* **Assembler:** NASM
* **Linker:** GNU LD / GCC (MinGW)
* **Sistema de construcción:** Make (Linux) / `build.bat` (Windows)
* **IDE recomendados:** CLion, VSCode, Vim
* **Control de versiones:** Git + GitHub
* **Tests:** `make test`

---

## 📁 Estructura del Repositorio

```
/aym/
├── compiler/        # Código fuente de 'aymc'
│   ├── lexer/       # Analizador léxico
│   ├── parser/      # Analizador sintáctico
│   ├── ast/         # Representación del AST
│   ├── codegen/     # Generador de código
│   └── utils/       # Utilidades comunes
├── runtime/         # Biblioteca estándar mínima
├── samples/         # Ejemplos en .aym
├── tests/           # Tests automatizados
├── docs/            # Documentación técnica
├── build/           # Archivos generados
├── Makefile
└── README.md
```

---

## 🗓 Cronograma de Desarrollo

| Semana | Hito                                      |
| ------ | ----------------------------------------- |
| 1      | Diseño completo del lenguaje y gramática  |
| 2-3    | Implementación de Lexer y Parser          |
| 4      | Construcción del AST y sistema de tipos   |
| 5-6    | Generación de código + ensamblado inicial |
| 7-8    | Soporte para clases, ciclos y condiciones |
| 9-10   | Manejador de errores y mejoras de CLI     |
| 11     | Librería estándar mínima (`runtime/`)     |
| 12     | Documentación, empaquetado y publicación  |

---

## 📜 Licencia

Este proyecto se distribuye bajo la **licencia MIT**.

---

## 📣 Contribuciones

Este proyecto es abierto a toda colaboración. Nos interesa especialmente:

* Hablantes nativos de aymara
* Desarrolladores C++ con experiencia en compiladores
* Educadores y promotores de software libre

> ✨ ¡Únete al desarrollo y forma parte del cambio tecnológico-cultural!


