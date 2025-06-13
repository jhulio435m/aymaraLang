# AymaraLang (`aym`) 🇧🇴

**AymaraLang** es un lenguaje de programación moderno basado en la lengua originaria aymara. Su compilador, `aymc`, ha sido desarrollado desde cero en **C++17**, y permite generar ejecutables nativos `.ayn`. El proyecto busca promover la inclusión tecnológica, la educación y la preservación lingüística.

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

### `ops.aym`
```aymara
willt’aña(3 + 4 * 2);
```

### Compilación y Ejecución

```bash
$ ./bin/aymc samples/hola.aym
$ ./build/out
Kamisaraki!
```

```bash
$ ./bin/aymc samples/ops.aym
$ ./build/out
11
```

```bash
$ ./bin/aymc samples/condloop.aym
$ ./build/out
cond
loop
loop
loop
```

```bash
$ ./bin/aymc samples/vars.aym
$ ./build/out
13
```

```bash
$ ./bin/aymc samples/runtime.aym
$ ./build/out
inicio
3
2
1
```

---

## 🧰 Tecnologías y Herramientas

* **Lenguaje:** C++17
* **Arquitectura objetivo:** x86\_64 Linux
* **Assembler:** NASM
* **Linker:** GNU LD
* **Sistema de construcción:** Make
* **IDE recomendados:** CLion, VSCode, Vim
* **Control de versiones:** Git + GitHub
* **Tests:** Google Test (planificado)

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

**Por definir.** Sugerencias:

* MIT: abierta, sencilla, ideal para educación.
* GPLv3: mayor protección de libertad de software.

---

## 📣 Contribuciones

Este proyecto es abierto a toda colaboración. Nos interesa especialmente:

* Hablantes nativos de aymara
* Desarrolladores C++ con experiencia en compiladores
* Educadores y promotores de software libre

> ✨ ¡Únete al desarrollo y forma parte del cambio tecnológico-cultural!


