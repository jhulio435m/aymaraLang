# Hoja de ruta: de AymaraLang a "nivel C++"

Fecha: 2026-11-20. Análisis de brechas hecho leyendo el código de `compiler/` y `runtime/` y con sondas compiladas con `bin/aymc`.

**Alcance y límites de este análisis.** Se hizo con tiempo muy acotado. Cada afirmación lleva una de estas marcas:

- **[V]** verificado en código (se cita archivo:línea) o por sonda ejecutada.
- **[NV]** no verificado: inferido de nombres de archivos, flags o ausencia en una búsqueda parcial. Debe confirmarse antes de planificar sobre ello.

No se leyeron completos: `codegen_builtins.cpp`, `codegen_emit_sections.cpp`, `semantic.cpp`, `semantic_expr.cpp`, `parser_expr.cpp`, `module_resolver*.cpp`, `runtime.c`, ni `docs/`. Las celdas que dependen de esos archivos están marcadas [NV].

Elementos que otros agentes están implementando ahora se marcan **en curso (2026-11-20)**: asignación compuesta, operadores bit a bit, do-while, constantes; más builtins de cadenas/listas/matemática/archivos; plegado de constantes en AST + peephole; correcciones de fallos del compilador; parámetros por defecto / métodos abstractos / lambdas.

---

## 1. Resumen ejecutivo

1. AymaraLang es hoy un lenguaje de scripting compilado a NASM x86-64, no un lenguaje de sistemas. La distancia a C++ es estructural, no de detalle.
2. El sistema de tipos tiene cuatro tipos primitivos (`Int, Float, Bool, String`, `compiler/builtins/builtins.h:10-15`) más lista (`taqa`) y mapa (`mapa`) como palabras clave (`compiler/lexer/lexer.cpp:249-255`). No hay enteros con tamaño, sin signo, `char`, punteros, referencias ni tipos definidos por el usuario con layout fijo.
3. Los objetos son mapas de cadenas: el constructor llama a `aym_map_new` y registra campos y punteros a métodos con `aym_map_set` (`compiler/codegen/codegen_class.cpp:110-142`); la búsqueda es lineal con `strcmp` (`runtime/runtime_maps_strings.c:12`). El despacho de métodos cuesta O(n) comparaciones de cadenas.
4. No hay gestión de memoria: no se encontró GC ni conteo de referencias; `aym_str_concat` hace `malloc` en cada concatenación (`runtime/runtime_maps_strings.c:194`) y los únicos `free` están en rutas de error y en `aym_array_free` manual (`runtime/runtime_arrays.c:50-54`). Un programa de larga duración pierde memoria sin límite.
5. Las excepciones usan `setjmp/longjmp` (`runtime/runtime_exceptions.c:52,73`), sin destructores ni limpieza de recursos al desenrollar.
6. El código generado es de máquina de pila: todo pasa por `rax/rbx` con `push/pop` y variables en `[rbp-N]` (`compiler/codegen/codegen_expr.cpp:152,250,270-272`). No hay asignación de registros ni niveles `-O` en el driver (`compiler/utils/driver.cpp:56-268`).
7. Errores de ejecución básicos no se controlan: división por cero termina en SIGFPE y recursión sin fin en SIGSEGV (sondas, sección 4).
8. No hay genéricos, interfaces, sobrecarga de operadores, herencia múltiple, hilos ni red (ningún nodo AST ni símbolo de runtime correspondiente).
9. Lo que sí existe y es sólido como base: clases con herencia simple, `static`/`private`, comprobación de firmas al redefinir (`compiler/semantic/semantic_class.cpp:125-160`), try/catch/finally, importaciones con alias, manifiesto y lockfile con semver, diagnósticos con códigos y salida JSON.
10. Prioridad honesta: primero seguridad de ejecución y memoria, después tipos y representación de objetos, y solo entonces rendimiento y genericidad. Alcanzar paridad real con C++ no es un objetivo realista a corto plazo; alcanzar un lenguaje compilado seguro y útil sí lo es.

---

## 2. Tabla comparativa por área

Esfuerzo: S (días), M (1-3 semanas), L (1-3 meses), XL (más de 3 meses). Prioridad: P0 (bloqueante), P1, P2, P3.

### Tipos

| Aspecto | C++ | AymaraLang hoy | Brecha | Esf. | Prio. |
|---|---|---|---|---|---|
| Tipado estático | Completo | [V] Anotaciones `jakhüwi`, `aru`, `taqa`, `mapa`, `chiqa` (`lexer.cpp:249-257`); enum de 4 tipos (`builtins.h:10-15`) | Listas y mapas no llevan tipo de elemento en el enum; objetos sin tipo propio | L | P1 |
| Enteros vs. flotantes | Distintos, con conversiones definidas | [V] `Float` existe en el enum; [NV] en `codegen_expr.cpp` no aparecen `xmm`/`cvt*`, la aritmética vista es entera (`:270-272`) | Confirmar cómo se compila la aritmética flotante | M | P1 |
| Enteros con tamaño / sin signo | `int8_t`…`uint64_t` | [V] Un único entero de 64 bits con signo (`imul`/`idiv` sobre `rax`) | Ausente | M | P2 |
| `char` | Sí | [V] No hay palabra clave ni tipo | Ausente | S | P2 |
| Punteros / referencias | Sí | [V] No hay nodo AST ni token | Ausente; decidir si se quieren (ver fase 3) | XL | P3 |
| `const` | Sí | en curso (2026-11-20) | — | S | P1 |
| Inferencia (`auto`) | Sí | [NV] `VarDeclStmt` lleva tipo; no se comprobó si es opcional | Confirmar | S | P2 |
| Conversiones explícitas | `static_cast` etc. | [NV] no revisado en builtins | Confirmar | S | P2 |
| Structs / enums / unions | Sí | [V] No hay nodos AST (`ast.h:10-45`) | Ausente | M | P1 |

### Memoria

| Aspecto | C++ | AymaraLang hoy | Brecha | Esf. | Prio. |
|---|---|---|---|---|---|
| Pila / heap | Control explícito | [V] Escalares en `[rbp-N]`; cadenas, listas, mapas y objetos siempre en heap | Sin valores compuestos en pila | L | P2 |
| Propiedad / RAII / destructores | Sí | [V] No hay destructor en `ClassStmt` (`ast.h:480-527`) | Ausente | L | P1 |
| Liberación | Determinista | [V] Solo `array_free` manual (`builtins.h:31`, `runtime_arrays.c:50-54`); cadenas y mapas nunca se liberan | Fugas sistemáticas | L | P0 |
| GC / refcount | Opcional (`shared_ptr`) | [V] No se encontró en `runtime/*.c` | Ausente | L | P0 |

### Funciones

| Aspecto | C++ | AymaraLang hoy | Brecha | Esf. | Prio. |
|---|---|---|---|---|---|
| Sobrecarga | Sí | [V] Constructores por aridad (`semantic_class.cpp:105`); [NV] funciones libres | Sin sobrecarga por tipo | M | P2 |
| Argumentos por defecto | Sí | [V] `Param` solo tiene `type` y `name` (`ast.h:110-113`); en curso (2026-11-20) | — | S | P1 |
| Lambdas / clausuras | Sí | [V] Existe `FunctionRefExpr` (`ast.h:259`), no hay nodo lambda; en curso (2026-11-20) | Captura de entorno pendiente | M | P1 |
| Límite de recursión | Stack overflow | [V] SIGSEGV sin mensaje (sonda p2, exit 139) | Sin diagnóstico | S | P1 |
| Varargs | Sí | [NV] | Confirmar | M | P3 |
| Inline | Sí | [V] No hay optimizador en codegen/driver | Ausente | M | P2 |

### POO

| Aspecto | C++ | AymaraLang hoy | Brecha | Esf. | Prio. |
|---|---|---|---|---|---|
| Clases, herencia simple | Sí | [V] `ClassStmt` con una base (`ast.h:480-527`), detección de ciclos (`semantic_class.cpp:125`) | — | — | — |
| Despacho virtual | vtable O(1) | [V] Búsqueda lineal por nombre en mapa (`codegen_class.cpp:130-136`, `runtime_maps_strings.c:12`) | Lento y sin layout fijo | L | P1 |
| Interfaces / abstractos | Sí | [V] Sin nodo AST; abstractos en curso (2026-11-20) | Interfaces ausentes | M | P1 |
| Herencia múltiple | Sí | [V] `baseName` único | Ausente; baja prioridad | L | P3 |
| Sobrecarga de operadores | Sí | [V] Sin soporte en AST | Ausente | M | P2 |
| Control de acceso | public/protected/private | [V] `isPrivate` en campos y métodos (`ast.h:480-500`, `semantic_class.cpp:153,385`) | Falta `protected` | S | P3 |
| `static` | Sí | [V] `isStatic` (`semantic_class.cpp:144,368`) | — | — | — |

### Resto de áreas

| Área | C++ | AymaraLang hoy | Brecha | Esf. | Prio. |
|---|---|---|---|---|---|
| Genericidad | Templates, concepts | [V] Ningún nodo AST ni token | Ausente por completo | XL | P2 |
| Módulos y paquetes | Namespaces, módulos; gestor externo | [V] `apnaq` con listas de símbolos y alias (`parser.cpp:587-637`); manifiesto, lockfile y semver (`utils/project_manifest.cpp`, `semver.cpp`, flags `driver.cpp:61-65`) | [NV] visibilidad/export y registro de paquetes | M | P2 |
| Manejo de errores | Excepciones con unwinding y tipos | [V] try/catch/finally por `setjmp/longjmp` (`runtime_exceptions.c:52,73`, `codegen_stmt.cpp:366`); no capturada termina con `exit(1)` (`:70`) | Sin limpieza al desenrollar; errores de CPU no se convierten en excepción | M | P0 |
| Biblioteca estándar | STL completa | [V] Arreglo dinámico y mapa lineal; builtins matemáticos, entrada, sleep, random (`builtins.h:22-35`); más builtins en curso (2026-11-20) | Sin tabla hash, conjuntos, colas, ordenación genérica, tiempo; [NV] archivos | L | P1 |
| Concurrencia | Hilos, atomics, mutex | [V] Ninguna referencia a `pthread`/`thread` en `runtime/*.c` | Ausente | XL | P3 |
| Red | Vía bibliotecas | [V] Ninguna referencia a `socket` | Ausente | L | P3 |
| Rendimiento | Optimizador maduro | [V] Máquina de pila sin registros (`codegen_expr.cpp:250,270`); sin `-O`; plegado + peephole en curso (2026-11-20) | Sin IR, sin asignación de registros, sin inlining | XL | P2 |
| Portabilidad | Muchas arquitecturas | [V] Flags `--windows`/`--linux` (`driver.cpp:260-264`); salida NASM x86-64; backend `ir` incompleto (`backend.cpp:256`); [NV] macOS (`Makefile.macos` existe) | Sin ARM64 | XL | P3 |
| Herramientas | gdb/DWARF, LSP, formateador | [V] `--debug`, `--dump-ast`, `--emit-ast-json`, `--check` (`driver.cpp:98-155`); extensión VS Code con gramática y snippets (`aymlang/`); pruebas golden y unitarias (`tests/`) | [NV] contenido de `--debug` (DWARF), LSP; sin formateador, REPL ni ejecutor de pruebas del lenguaje | L | P2 |
| Diagnósticos | Detallados | [V] Códigos `AYM3xxx`, salida JSON (`driver.cpp:57`), línea y columna en tokens | Errores de ejecución sin línea de origen | M | P1 |
| Interoperabilidad (FFI C) | Nativa | [V] El runtime ya se enlaza como C y usa la convención de registros (`codegen_class.cpp:133`); no hay sintaxis `extern` en el AST | Falta declarar funciones C desde el lenguaje | M | P1 |

---

## 3. Hoja de ruta por fases

### Fase 1 — Días (seguridad de ejecución y coherencia)

| # | Ítem | Archivos | Diseño |
|---|---|---|---|
| 1.1 | División y módulo por cero como excepción | `compiler/codegen/codegen_expr.cpp:271-272`, `runtime/runtime_exceptions.c` | Antes de `idiv`: `test rbx, rbx` / `jz` a una rutina que llama `aym_throw_typed("ARITMETICA", ...)`. |
| 1.2 | Índice fuera de rango uniforme | `runtime/runtime_arrays.c:27-48` | `aym_array_get/set` deben lanzar `INDICE` como ya hacen las funciones de las líneas 97 y 102, en vez de devolver 0. |
| 1.3 | Desbordamiento de pila diagnosticado | `runtime/runtime.c`, `compiler/codegen/codegen_emit_sections.cpp` | Manejador de `SIGSEGV`/`SIGFPE` en pila alternativa (`sigaltstack`) que imprime mensaje y hace `fflush`. |
| 1.4 | Vaciar stdout al abortar | `runtime/runtime.c` | El manejador de 1.3 hace `fflush(stdout)`; hoy la salida previa se pierde (sonda p1). |
| 1.5 | Constantes, asignación compuesta, bit a bit, do-while | — | en curso (2026-11-20) |
| 1.6 | Parámetros por defecto | `ast.h:110-113`, `parser.cpp`, `semantic.cpp`, `codegen_function.cpp` | en curso (2026-11-20); añadir `std::unique_ptr<Expr> defaultValue` a `Param` y rellenar en el sitio de llamada. |

Dependencias: 1.3 antes de 1.4. 1.1 y 1.2 son independientes.

### Fase 2 — Semanas (memoria, tipos, objetos)

| # | Ítem | Archivos | Diseño |
|---|---|---|---|
| 2.1 | Gestión de memoria | `runtime/*.c`, `codegen_expr.cpp`, `codegen_stmt.cpp` | Opción recomendada por coste: GC conservador (Boehm) sustituyendo `malloc` en el runtime; no toca el codegen. Alternativa: conteo de referencias con cabecera en cada objeto, que exige emitir retain/release en asignaciones y salidas de ámbito. |
| 2.2 | Tipos de elemento en colecciones | `builtins.h:10-15`, `semantic.h`, `semantic_expr.cpp`, `parser.cpp` | Sustituir el enum por una estructura `TypeRef {kind, elem, key, className}`; sintaxis `taqa<jakhüwi>`. Es prerrequisito de genéricos. |
| 2.3 | Objetos con layout fijo y vtable | `codegen_class.cpp:85-142`, `semantic_class.cpp`, `codegen_expr.cpp` | Cada clase: tabla estática de punteros a método; cada instancia: `[vtable][campos por desplazamiento]`. Acceso a campo pasa a ser `mov rax,[obj+off]`. Requiere 2.2 para conocer el tipo estático del receptor. |
| 2.4 | Interfaces y métodos abstractos | `ast.h` (`ClassStmt`), `parser_class.cpp`, `semantic_class.cpp` | Abstractos en curso (2026-11-20). Interfaces como clases sin campos sobre la vtable de 2.3. |
| 2.5 | Lambdas con captura | `ast.h`, `parser_expr.cpp`, `codegen_function.cpp` | en curso (2026-11-20). Clausura = `{puntero a función, puntero a entorno en heap}`; depende de 2.1 para liberar el entorno. |
| 2.6 | Structs y enums | `ast.h`, `lexer.cpp`, `parser.cpp`, `semantic.cpp`, codegen | Reutilizar el layout de 2.3 sin vtable. |
| 2.7 | FFI C | `lexer.cpp`, `parser.cpp`, `codegen_emit_sections.cpp:525` | Declaración `extern` que emite `extern simbolo` en el ASM y opción de enlace `-l` en el backend. |
| 2.8 | Mapa con tabla hash | `runtime/runtime_maps_strings.c:12` | Sustituir la búsqueda lineal por direccionamiento abierto. |
| 2.9 | Línea de origen en errores de ejecución | `codegen_stmt.cpp`, `runtime_exceptions.c` | Pasar la línea como argumento a `aym_throw_typed`. |

Dependencias: 2.2 → 2.3 → 2.4 y 2.6. 2.1 → 2.5. 2.7 y 2.8 son independientes.

### Fase 3 — Meses (estructural)

| # | Ítem | Archivos | Diseño |
|---|---|---|---|
| 3.1 | IR intermedio y asignación de registros | nuevo `compiler/ir/`, `backend.cpp` (el backend `ir` ya está esbozado, `:256`) | IR de tres direcciones en SSA; asignación por barrido lineal. Alternativa de menor coste total: emitir LLVM IR y obtener optimización, DWARF y ARM64 a la vez. |
| 3.2 | Genéricos | parser, semántico, codegen | Monomorfización sobre `TypeRef` de 2.2. Depende de 2.2 y 2.3. |
| 3.3 | Destructores y limpieza en excepciones | `ast.h`, `codegen_stmt.cpp:366`, `runtime_exceptions.c` | Registrar acciones de limpieza por marco de `try`; con `longjmp` exige una pila de limpieza explícita en el runtime. Depende de 2.1 y 2.3. |
| 3.4 | Información de depuración | codegen, backend | Directivas `%line` de NASM como mínimo; DWARF completo llega con 3.1 si se usa LLVM. |
| 3.5 | Servidor de lenguaje y formateador | `aymlang/`, nuevo `tools/` | Apoyarse en `--emit-ast-json` y `--diagnostics-json`, que ya existen. |
| 3.6 | Concurrencia | runtime, semántico | Solo después de 2.1: el modelo de memoria condiciona el de hilos. |
| 3.7 | ARM64 | backend | Depende de 3.1. |

---

## 4. Riesgos y deuda técnica

| # | Hallazgo | Evidencia | Gravedad |
|---|---|---|---|
| R1 | División entera por cero termina el proceso con SIGFPE (exit 136) y no es capturable; se pierde la salida previa en stdout cuando va a un pipe | Sonda `build/tmp/m5/p1.aym`; `codegen_expr.cpp:271-272` emite `cqo`/`idiv` sin comprobación. Repro: `/tmp/claude-1000/-home-blink-documents-projects-aymaraLang/79f8aa6b-d9c0-47e5-b04f-60628bf57df5/scratchpad/bugs/m5_div_zero.aym` | Alta |
| R2 | Recursión sin caso base termina en SIGSEGV (exit 139) sin mensaje | Sonda `build/tmp/m5/p2.aym` | Media |
| R3 | `aym_array_get` devuelve 0 en silencio para índice fuera de rango o arreglo nulo, mientras otras funciones del mismo archivo lanzan `INDICE` | `runtime/runtime_arrays.c:27-36` frente a `:97,102` | Alta (errores silenciosos) |
| R4 | Sin liberación de cadenas ni mapas; cada concatenación reserva memoria nueva | `runtime/runtime_maps_strings.c:188-197`; lista completa de `free` en `runtime/` limitada a rutas de error y `aym_array_free` | Alta |
| R5 | `aym_array_free` manual sin protección: doble liberación y uso posterior quedan a cargo del programa | `runtime/runtime_arrays.c:50-54` | Media |
| R6 | Despacho de métodos y acceso a campos O(n) con `strcmp` | `codegen_class.cpp:110-142`, `runtime_maps_strings.c:12` | Media (rendimiento) |
| R7 | Excepción no capturada termina con `exit(1)`; [NV] no se comprobó qué mensaje imprime | `runtime/runtime_exceptions.c:67-70` | Baja |
| R8 | `strcpy` en el runtime gráfico; [NV] no se comprobó el tamaño del destino | `runtime/runtime_gfx_linux.c:123` | Por revisar |
| R9 | Búfer fijo de 64 bytes en conversión a cadena; [NV] no se comprobó si usa `snprintf` | `runtime/runtime_maps_strings.c:341` | Por revisar |
| R10 | Lectura de entero con `fscanf("%ld")` que devuelve 0 ante error, indistinguible de un 0 válido | `runtime/runtime.c:857` | Baja |
| R11 | Backend `ir` no completa el enlace | `compiler/backend/backend.cpp:256` | Informativo |

No se pudo comprobar el comportamiento ante desbordamiento de enteros: la sonda lo incluía, pero su salida se perdió por R1.

---

## 5. Pendiente de verificar antes de planificar

- Compilación de aritmética `Float` (qué archivo la emite).
- Sobrecarga de funciones libres, varargs, inferencia de tipo en `yatiya`.
- Builtins de archivos y cadenas ya presentes (`compiler/builtins/builtins.cpp`).
- Qué emite `--debug`.
- Estado real de Windows y macOS.
- Contenido de `aymlang/src` (si hay servidor de lenguaje).
