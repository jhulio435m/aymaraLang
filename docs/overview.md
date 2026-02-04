# Visión general

AymaraLang (`.aym`) es un lenguaje de programación con sintaxis en aymara y un
compilador (`aymc`) implementado en **C++17**. El compilador genera ejecutables
nativos (con extensión `.exe` en Windows). El runtime acompaña al binario para
resolver funciones estándar.

## Identidad y alcance actual

| Elemento | Detalle |
| --- | --- |
| Compilador | `aymc` |
| Lenguaje | AymaraLang (`.aym`) |
| Salida | Binario nativo (sin extensión en Unix, `.exe` en Windows) |
| Paradigma | Imperativo con soporte de funciones y clases |
| Tipado | Estático, fuerte |
| Backend principal | NASM + enlazado por GCC/LD (o MinGW) |
| Backend opcional | LLVM (si se compila con soporte) |

## Principios del proyecto

- **Cercanía cultural:** palabras clave en aymara y ejemplos contextualizados.
- **Compilación nativa:** no requiere VM para ejecutar los binarios generados.
- **Modularidad:** el compilador separa lexer, parser, semántica y codegen.
- **Documentación de investigación:** incluye ingeniería de sistemas y LaTeX.

## Sintaxis base

- Bloques con `{}` y fin de sentencia con `;`.
- Palabras clave en aymara (`jisa`, `taki`, `lurawi`, etc.).
- Comentarios `//` y `/* */`.

## Contexto del sistema

```mermaid
flowchart LR
    A[Persona usuaria] -->|Escribe .aym| B[Editor/CLI]
    B --> C[Compilador aymc]
    C --> D[Binario nativo]
    D --> E[Runtime]
```

La salida nativa evita una máquina virtual, y el costo dominante se concentra en
las fases de análisis. En términos simplificados, si $n$ es el número de
tokens, el parseo LL se mantiene en $O(n)$.

---

**Siguiente:** [Primeros pasos](language.md)
