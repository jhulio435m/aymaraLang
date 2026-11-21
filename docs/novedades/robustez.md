# Robustez del compilador

Errores corregidos en estas rondas, con sus pruebas de regresión en `tests/golden/robustez/`.

## Corregidos

### División o módulo entero por cero terminaba con SIGFPE

- **Síntoma:** `qillqa(10 / z);` con `z == 0` mataba el programa con SIGFPE (código 136, volcado de memoria) y se perdía la salida previa de `qillqa` si stdout era un pipe. Lo mismo ocurría con `%`.
- **Causa:** el generador emitía `idiv rbx` sin comprobar el divisor.
- **Arreglo:** antes de `idiv` se comprueba el divisor; si es cero se llama a `aym_div_zero` (runtime), que vacía stdout y lanza la excepción `ARITMETICA` con el mensaje `division entera por cero`. Se puede atrapar con `yant'aña { ... } katjaña("ARITMETICA", e) { ... }`; si nadie la atrapa, el programa termina con `Excepcion no manejada` y código 1.
- **Archivos:** `compiler/codegen/codegen_expr.cpp`, `compiler/codegen/codegen_emit_sections.cpp`, `runtime/runtime_exceptions.c`.
- **Prueba:** `tests/golden/robustez/div_cero_atrapable.aym`.

### La caché del runtime no se regeneraba al cambiar `runtime_*.c`

- **Síntoma:** tras editar `runtime/runtime_exceptions.c`, `runtime_arrays.c` o `runtime_maps_strings.c`, el enlace fallaba con `undefined reference` (o se usaba código antiguo), porque se reutilizaba el objeto en caché del directorio temporal.
- **Causa:** `needsRebuildObject` solo comparaba la fecha de `runtime.c`, no la de los archivos que este incluye con `#include`.
- **Arreglo:** para `runtime.c` también se comparan las fechas de los tres archivos incluidos.
- **Archivos:** `compiler/codegen/codegen_emit_sections.cpp`.
- **Prueba:** no tiene prueba automática (depende de fechas de archivos).

### Listas anidadas (`[[...]]`) se corrompían o imprimían punteros

- **Síntoma:** literales como `[[1, 2], [3, 4]]` o `[["a", "b"], ["c"]]` perdían los elementos internos al sobrescribir `rbx`, e imprimirlos mostraba direcciones enteras en lugar de su formato anidado.
- **Causa:** `ListExpr` conservaba el puntero al arreglo en `rbx` durante `emitExpr` recursivo, `inferListType`/`listElementType` no propagaban tipos de listas anidadas y `emitPrintList` solo imprimía un nivel.
- **Arreglo:** `ListExpr` resguarda el puntero en la pila alineada a 16 bytes, la detección de tipos infiere profundidades recursivamente y `emitPrintList` invoca `aym_print_list_depth` cuando el tipo de elemento es otra lista.
- **Archivos:** `compiler/codegen/codegen_expr.cpp`, `compiler/codegen/codegen_collect_stmt.cpp`, `compiler/codegen/codegen_type_detect.cpp`, `compiler/codegen/codegen_type_print.cpp`, `compiler/codegen/codegen_emit_sections.cpp`, `runtime/runtime_arrays.c`.
- **Prueba:** `tests/golden/robustez/lista_anidada.aym`.

### Indexación sobre cadenas (`s[i]`) fallaba en ejecución

- **Síntoma:** indexar un `aru` (`s[i]`) intentaba leerlo como lista mediante `aym_array_get`, produciendo basura o caída de memoria, y no se imprimía como texto.
- **Causa:** `IndexExpr` solo despachaba entre mapa y arreglo, e `isStringExpr` no contemplaba `IndexExpr` sobre `aru`.
- **Arreglo:** `IndexExpr` llama a `aym_str_char_at` cuando la base es `aru` e `isStringExpr` reconoce el resultado como `aru`.
- **Archivos:** `compiler/codegen/codegen_expr.cpp`, `compiler/codegen/codegen_type_detect.cpp`, `compiler/codegen/codegen_emit_sections.cpp`, `runtime/runtime_maps_strings.c`.
- **Prueba:** `tests/golden/robustez/cadena_index.aym`.

### Indexación tras acceso a miembro/campo (`obj.campo[i]`, `obj.m()[i]`) y asignaciones compuestas por índice

- **Síntoma:** expresiones como `obj.campo[i]`, `obj.metodo()[i]` o `m[f()]` perdían el índice o la base en subexpresiones anidadas, e igual ocurría en `mat[i][j] = val` o `MapExpr` con valores complejos.
- **Causa:** `IndexExpr` cargaba el índice en `reg2` antes de emitir la base; `IndexAssignStmt`, `MapExpr` y `emitPrintValue` (mapas indexados) guardaban temporales en `rbx`/`r14`/`r15`; y `codegen_type_detect.cpp` no propagaba tipos a través de `MemberExpr`, `MemberCallExpr` e `IndexExpr`.
- **Arreglo:** evaluación con respaldo en pila alineada a 16 bytes (y espacio shadow en Win64) en `IndexExpr`, `IndexAssignStmt`, `MapExpr` y `emitPrintValue`, junto con detección completa de `t'aqa`/`mapa`/`chiqa` en miembros, llamadas a método e indexaciones.
- **Archivos:** `compiler/codegen/codegen_expr.cpp`, `compiler/codegen/codegen_stmt.cpp`, `compiler/codegen/codegen_type_detect.cpp`, `compiler/codegen/codegen_type_print.cpp`.
- **Prueba:** `tests/golden/robustez/miembro_y_campo_index.aym`.

### Llamadas a métodos como argumentos (`obj.f(obj.g(), obj.h())`) y constructores con inicializadores anidados

- **Síntoma:** anidar llamadas a métodos dentro de argumentos de otro método o en constructores corrompía el puntero del receptor (`Aka`), la dirección del método o argumentos previos.
- **Causa:** `emitMemberCallExpr` mantenía puntero y receptor en `r14`/`r12` durante `emitCallArgs`, y `emitNewExpr` guardaba el objeto en `r12` y evaluaba los argumentos del constructor directamente en los registros de parámetros.
- **Arreglo:** `emitMemberCallExpr` resguarda receptor y método en la pila antes de `emitCallArgs` y salta vía `r11`; `emitNewExpr` resguarda el objeto nuevo en la pila, usa `emitCallArgs(..., 1)` para el constructor y omite métodos abstractos al registrar/instanciar clases.
- **Archivos:** `compiler/codegen/codegen_class.cpp`.
- **Prueba:** `tests/golden/robustez/args_miembro.aym`.

### `largo()` sobre cadenas (`aru`) y mapas (`mapa`)

- **Síntoma:** `largo("texto")` llamaba a `aym_array_length` sobre una cadena C y provocaba accesos inválidos.
- **Causa:** `BUILTIN_LARGO` asumía siempre un arreglo dinámico.
- **Arreglo:** `BUILTIN_LARGO` (y `BUILTIN_SUYUT`) despacha a `strlen` si el argumento es `aru`, a `aym_map_size` si es `mapa` y a `aym_array_length` para listas.
- **Archivos:** `compiler/codegen/codegen_builtins.cpp`.
- **Prueba:** `tests/golden/robustez/largo_aru.aym`.

### Colisión de variables globales con palabras reservadas de NASM

- **Síntoma:** declarar variables globales llamadas `at`, `in`, `str`, `si`, etc. causaba error de ensamblado en NASM porque el ensamblador las interpretaba como directivas, instrucciones o registros.
- **Causa:** los símbolos globales se emitían sin escape en `.data` y en referencias `[rel nombre]`.
- **Arreglo:** las definiciones (`$nombre: dq 0`) y referencias `[rel $nombre]` (variables globales, campos estáticos y ranuras internas de `yant'aña`/`katjaña`) usan el prefijo `$` de NASM, que desambigua el identificador manteniendo el nombre exacto del símbolo en el objeto.
- **Archivos:** `compiler/codegen/codegen_emit_sections.cpp`, `compiler/codegen/codegen_expr.cpp`, `compiler/codegen/codegen_stmt.cpp`, `compiler/codegen/codegen_class.cpp`.
- **Prueba:** `tests/golden/robustez/global_nasm_keyword.aym`.

### Diagnóstico por literal entero fuera de rango

- **Síntoma:** un literal entero superior al rango de 64 bits con signo abortaba el compilador con una excepción no atrapada (`std::out_of_range` en `std::stoll`).
- **Arreglo:** el parser valida el rango y emite un diagnóstico controlado sin abortar el compilador.
- **Prueba:** `tests/golden/robustez/errores/entero_fuera_de_rango.aym`.

### Funciones con más de 6 argumentos (4 en Windows) recibían basura

- **Síntoma:** `muchos(1, 1, 1, 1, 1, 1, 1, 1)` con 8 parámetros devolvía `985058238575797` en vez de `36`. Lo mismo en métodos y constructores a partir del 6.º argumento (el receptor `aka` ocupa un registro) y en `uja_suyu`, cuyo 7.º argumento (azul) se perdía.
- **Causa:** `emitCallArgs` descartaba en silencio los argumentos que no cabían en registros y `emitFunction` nunca leía parámetros de la pila.
- **Arreglo:** los argumentos extra se pasan en la pila según el ABI (SysV / Win64 con shadow space); `emitCallArgs` devuelve los bytes que el llamador libera tras el `call` y el callee los copia desde `[rbp+16(+32)]`.
- **Archivos:** `compiler/codegen/codegen_expr.cpp`, `codegen_function.cpp`, `codegen_class.cpp`, `codegen_builtins.cpp`, `codegen_impl.h`.
- **Prueba:** `tests/golden/robustez/args_pila.aym`.

### Numeración de `siqicha` con valores no literales

- **Síntoma:** `siqicha N { A = -1, B, C = 2 * 3, D }` daba `B = 1` y `D = 3` (debían ser `0` y `7`).
- **Causa:** el parser solo reconocía un `NumberExpr` literal; `-1` (unario) o `2 * 3` no actualizaban el contador.
- **Arreglo:** el valor se evalúa como constante entera (`+ - * / %`, bit a bit, desplazamientos, unarios). Un valor no constante es ahora un error de compilación.
- **Archivos:** `compiler/parser/parser.cpp`.
- **Pruebas:** `tests/golden/robustez/siqicha_valores.aym`, `tests/golden/robustez/errores/siqicha_no_constante.aym`.

### Índice de lista fuera de rango devolvía 0 en silencio

- **Síntoma:** `a[7]` sobre una lista de 1 elemento valía `0` y `a[5] = x` se ignoraba, mientras que cadenas y mapas sí lanzaban excepción.
- **Arreglo:** `aym_array_get`/`aym_array_set` lanzan `INDICE` ("indice de lista fuera de rango"), atrapable con `katjaña("INDICE", e)`.
- **Archivos:** `runtime/runtime_arrays.c`.
- **Prueba:** `tests/golden/robustez/indice_lista.aym`.

### `INT_MIN / -1` terminaba con SIGFPE y la potencia era O(n)

- **Síntoma:** `m / -1` con `m` = mínimo entero mataba el proceso (código 136) aun dentro de `yant'aña`, perdiendo la salida. `x ^ n` iteraba `n` veces (con exponente negativo en ejecución, ~2^64 iteraciones).
- **Arreglo:** con divisor `-1` se emite `neg` (cociente) o `0` (resto) sin `idiv`. `^` llama a `aym_ipow` (cuadrados, O(log n)); exponente negativo da `1/b^n` truncado (`1 → 1`, `-1 → ±1`, resto `0`) y `0 ^ negativo` lanza `ARITMETICA`. El plegado de constantes usa la misma semántica.
- **Archivos:** `compiler/codegen/codegen_expr.cpp`, `codegen_emit_sections.cpp`, `runtime/runtime_exceptions.c`.
- **Prueba:** `tests/golden/robustez/division_y_potencia.aym`.

### Bloque suelto `{ ... }` se parseaba como mapa; retorno implícito con basura

- **Síntoma:** un bloque `{ yatiya ... }` como sentencia daba "se esperaba ':' en mapa". Una función tipada que terminaba sin `kuttaya` devolvía lo que hubiera en `rax`.
- **Arreglo:** `{` al inicio de sentencia abre un `BlockStmt` con ámbito propio; al caer al final de una función se devuelve `0`.
- **Archivos:** `compiler/parser/parser.cpp`, `compiler/codegen/codegen_function.cpp`.
- **Prueba:** `tests/golden/robustez/bloque_y_retorno.aym`.

### `jilaaka` se resolvía con la clase del objeto, no la del código

- **Síntoma:** con `C jila B jila A`, `jilaaka.s()` dentro de `B.s()` llamado desde un objeto `C` volvía a `B.s()` → recursión infinita. `jilaaka.m()` hacia un método definido solo en el abuelo lanzaba `CLAVE: super::m`.
- **Causa:** la llamada buscaba la clave `super::m` en el objeto, que solo apunta a los métodos propios de la base directa de la clase instanciada.
- **Arreglo:** el análisis semántico resuelve `jilaaka.m(...)` estáticamente (`__Dueño_m`) y el codegen lo llama directamente con `aka` como receptor.
- **Archivos:** `compiler/semantic/semantic_class.cpp`, `compiler/codegen/codegen_class.cpp`.
- **Prueba:** `tests/golden/poo/jilaaka_estatico.aym`.

### Constructores no heredables ni invocables desde la subclase

- **Síntoma:** `machaqa Perro("rex")` fallaba con "no hay constructor con 1 parametros" si `Perro` no declaraba constructor, y no había forma de llamar al constructor del padre.
- **Arreglo:** una clase sin constructores hereda los del ancestro más cercano; dentro de un constructor `jilaaka.qallta(args);` invoca el del padre (con validación de aridad y tipos; fuera de un constructor es error).
- **Archivos:** `compiler/parser/parser_expr.cpp`, `compiler/semantic/semantic.h`, `compiler/semantic/semantic_class.cpp`, `compiler/codegen/codegen_class.cpp`.
- **Pruebas:** `tests/golden/poo/herencia_constructor.aym`, `tests/golden/poo/errores/jilaaka_qallta_*.aym`.

### Fallos fatales sin mensaje y con pérdida de la salida

- **Síntoma:** una recursión demasiado profunda terminaba con `Segmentation fault` y sin la salida previa de `qillqa` (stdout en búfer).
- **Arreglo:** el runtime instala (en `aym_set_args`) un manejador de SIGSEGV/SIGBUS sobre pila alternativa (en Windows, `SetUnhandledExceptionFilter`) que vacía stdout e imprime `Error fatal: desbordamiento de pila o acceso invalido a memoria`.
- **Archivos:** `runtime/runtime.c`.
- **Prueba:** no automática (el código de salida es 139).

### El Makefile no recompilaba al cambiar un header

- **Síntoma:** tras editar `ast.h` u otros `.h`, `make` solo recompilaba algunos `.cpp` y el compilador resultante fallaba con segfault en casi todos los programas (68 de 70 pruebas golden).
- **Arreglo:** se compila con `-MMD -MP` y se incluyen los `.d` generados.
- **Archivos:** `Makefile`.

### Diagnóstico y documentación de `aru + jakhüwi`

- El tutorial (`docs/language.md`) usaba `"suma: " + suma(3, 4)`, que el tipado estricto rechaza. Se corrigió a `aru(suma(3, 4))` y el error ahora indica los tipos y sugiere `aru(...)`.

### `khiti` perdía su valor al evaluar un `kuna` con operaciones

- **Síntoma:** `khiti (n) { kuna k + 1: ... }` con `n == k + 1` iba a `yaqha`.
- **Causa:** el valor se guardaba en `rbx`, que la evaluación de `k + 1` reutiliza.
- **Arreglo:** el valor se guarda en la pila y se recarga antes de cada comparación; cada coincidencia pasa por un trampolín que libera la pila.
- **Archivos:** `compiler/codegen/codegen_stmt.cpp`.
- **Prueba:** `tests/golden/decimales/khiti.aym`.

## Limitaciones conocidas (sin corregir)

- La excepción no manejada por división por cero no indica la línea del programa fuente; solo imprime `Excepcion no manejada`.
- Los builtins con más argumentos que registros deben liberar la pila que devuelve `emitCallArgs` (hoy solo `uja_suyu`).
