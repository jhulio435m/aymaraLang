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
7. **Ensamblado y Enlace** – Uso de `nasm` y `ld` para crear `.ayn`.

> ⚙️ Futuras mejoras incluirán soporte para LLVM como backend opcional.

---

## 🧪 Ejemplo de Código

### `hola.aym`
```aymara
yatiyawi Hola:
    lurayiri ninchaña():
        willt’aña("Kamisaraki!");

jaqichawi:
    P = Hola()
    P.ninchaña()
````

### Compilación y Ejecución

```bash
$ aymc hola.aym -o hola.ayn
$ ./hola.ayn
Kamisaraki!
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

```

¿Te gustaría que genere también un diagrama de arquitectura del compilador?
```
