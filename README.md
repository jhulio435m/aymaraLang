# AymaraLang (`aym`) 🇵🇪

**AymaraLang** es un lenguaje de programación moderno basado en la lengua originaria aymara. Su compilador, `aymc`, ha sido desarrollado desde cero en **C++17**, y permite generar ejecutables nativos `.ayn`. El proyecto busca promover la inclusión tecnológica, la educación y la preservación lingüística.

Palabras clave principales del lenguaje:

- `jach’a`, `lliphiphi`, `qillqa`, `chuymani` – tipos primitivos (int, float, string, bool)
- `willt’aña` – salida por pantalla
- `input` – lectura de consola
- `luräwi` / `kutiyana` – definición de funciones y retorno
- `si`, `sino`, `mientras`, `haceña`, `para`, `tantachaña`

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
6. **Generación de Código** – Código ensamblador x86_64.
7. **Ensamblado y Enlace** – Uso de `nasm` y `gcc` para crear `.ayn`.

Las estructuras ahora incluyen `else`, ciclos `for` y funciones simples.

Las condiciones y bucles ahora se ejecutan en tiempo de ejecución gracias a un
AST más completo, análisis semántico y generación de código en ensamblador.

> ⚙️ Futuras mejoras incluirán soporte para LLVM como backend opcional.

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

### Uso en Windows

1. Instalar [MinGW-w64](https://www.mingw-w64.org/) y la versión para Windows de `nasm`
2. Ejecutar `build.bat` para compilar `aymc.exe`.
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


