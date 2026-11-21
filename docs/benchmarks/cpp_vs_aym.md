# AymaraLang vs C++: brecha de rendimiento

- Fecha de medición: 2026-11-20
- Máquina: `x86_64`, Intel(R) Core(TM) i5-10300H CPU @ 2.50GHz, Linux
- Compilador: `bin/aymc` (backend nativo, NASM + gcc) contra `g++ -O0` y `g++ -O2`
- Fuentes: `samples/bench/cpp_vs_aym/` (mismo algoritmo en `.aym` y `.cpp`, misma salida)
- Reproducir: `bash samples/bench/cpp_vs_aym/run.sh`

> **Advertencia: mediciones ruidosas.** Se tomaron mientras otros procesos
> cargaban la máquina (load average entre 13 y 19 en 8 hilos). Cada cifra es el
> mejor tiempo de pared de 3 ejecuciones e incluye el arranque del proceso. Entre
> dos corridas consecutivas el mismo binario varió hasta 2x. Los ratios pequeños
> (menos de ~3x) no son fiables; los órdenes de magnitud sí.

## Tabla medida

| benchmark | aym ms | C++ -O0 ms | C++ -O2 ms | aym / C++ -O2 | aym / C++ -O0 |
| --- | ---: | ---: | ---: | ---: | ---: |
| calls | 423 | 136 | 90 | 5x | 3.1x |
| fib | 352 | 123 | 26 | 14x | 2.9x |
| int_loop | 321 | 116 | 89 | 4x | 2.8x |
| list_sum | 287 | 268 | 24 | 12x | 1.1x |
| map_ops | 644 | 16 | 6 | 107x | 40.2x |
| methods | 168 | 14 | 11 | 15x | 12.0x |
| nested_loops | 157 | 48 | 34 | 5x | 3.3x |
| sieve | 118 | 196 | 56 | 2x | 0.6x |
| str_concat | 237 | 27 | 28 | 8x | 8.8x |

Qué mide cada caso:

| benchmark | contenido | tamaño |
| --- | --- | --- |
| int_loop | suma modular en un bucle | 20 000 000 iteraciones |
| nested_loops | dos bucles anidados con `i * j` y `%` | 3000 x 3000 |
| fib | Fibonacci recursivo ingenuo | `fib(36)` |
| sieve | criba de Eratóstenes sobre `t'aqa` | 2 000 000 |
| list_sum | `push` de 1 000 000 elementos y 30 pasadas de suma por índice | 30 000 000 lecturas |
| str_concat | `s = s + "ab"` | 20 000 concatenaciones |
| map_ops | inserción y búsqueda con claves `"k" + aru(i)` | 8000 claves |
| calls | función de 3 argumentos llamada en bucle | 20 000 000 llamadas |
| methods | método de objeto que actualiza un campo | 2 000 000 llamadas |

Notas de lectura:

- En `calls` y `methods`, `g++ -O2` puede expandir en línea la función; el ratio
  mide el código tal como lo escribiría un usuario, no el costo puro de la llamada.
- `map_ops` y `str_concat` son cuadráticos en AymaraLang (ver abajo), así que su
  ratio **crece con N**: con 20 000 claves `map_ops` no terminó en 8 s.
- `sieve` y `list_sum` quedan cerca o por debajo de `-O0`; con este ruido no se
  puede afirmar que AymaraLang gane a `-O0`.

## Por qué es lento: lo que dice el ASM

Hallazgo previo: el backend **no** encaja los valores como `double`. `jakhüwi`
se compila a enteros de 64 bits en `rax` (`add`, `cqo`/`idiv`). La brecha
numérica no viene de boxing sino de la forma del código generado.

### 1. `map_ops` (107x): el mapa es un arreglo con búsqueda lineal

`runtime/runtime_maps_strings.c`, `aym_map_find`:

```c
for (long i = 0; i < map->len; i++) {
    if (map->keys[i] && strcmp(map->keys[i], key) == 0) return i;
}
```

`aym_map_set` y `aym_map_get` llaman a `aym_map_find`, así que insertar o leer
N claves cuesta O(N^2) comparaciones de texto. Además cada iteración del bucle
hace `call aym_to_string` + `call aym_str_concat` (un `malloc` por clave que
nunca se libera: en ese archivo no hay `free` para el resultado de
`aym_str_concat`).

### 2. `methods` (15x): los objetos son mapas y todo se resuelve por nombre

ASM generado para `c.suma(i)` dentro del bucle (`methods.asm`):

```asm
mov rax, [rel c]
mov rbx, rax
lea rsi, [rel str1]      ; "suma"
mov rdi, rbx
call aym_map_get         ; busca el método por texto en cada llamada
mov r14, rax
...
call r14                 ; llamada indirecta
```

Y dentro de `__Contador_suma`, `aka.n = (aka.n + x) % 1000003` es:

```asm
lea rsi, [rel str0]      ; "n"
call aym_map_get         ; lectura del campo: strcmp lineal
...
cqo
idiv rbx
...
call aym_map_set         ; escritura del campo: otra búsqueda lineal
```

Son 3 llamadas al runtime con `strcmp` por cada invocación de un método de una
línea, más un prólogo que guarda 5 registros (`rbx`, `r12`–`r15`) siempre. En
C++ el campo es `[rdi+0]` y la llamada desaparece.

### 3. Código de máquina de pila en toda expresión (`int_loop`, `nested_loops`, `fib`, `calls`: 4–14x)

La comparación `i < 2000000` y el incremento `i = i + 1` se emiten así:

```asm
mov rax, [rel i]
push rax
sub rsp, 8
mov rax, 2000000
add rsp, 8
mov rbx, rax
pop rax
cmp rax, rbx
setl al
movzx rax,al
cmp rax,0
je forend6
```

Causas concretas visibles:

- Cada operación binaria hace `push`/`pop` y un par `sub rsp, 8` / `add rsp, 8`
  que no hace nada.
- Las constantes pasan por `rax` y `rbx` en vez de ser operandos inmediatos.
- Las condiciones se materializan como 0/1 (`setl` + `movzx` + `cmp rax,0`) en
  vez de saltar directamente con `jge`.
- Las variables viven en memoria (`[rel i]`, `[rbp-48]`); no hay asignación de
  registros, así que cada uso es una carga y cada asignación un almacenamiento.
- `% constante` usa `idiv` (20–40 ciclos); `g++ -O2` lo convierte en
  multiplicación.
- Los argumentos de llamada se copian a la pila y de vuelta
  (`mov [rsp+0], rax` / `mov rsi, [rsp+0]`).

### 4. `str_concat` (8x): concatenación cuadrática con fuga

`aym_str_concat` hace `strlen` de ambos lados, `malloc` del total y dos
`memcpy` en cada `+`. Las cadenas no guardan su longitud, así que construir un
texto de N trozos cuesta O(N^2) y deja N cadenas intermedias sin liberar.

## Cambios propuestos, ordenados por beneficio esperado

1. **Mapa con tabla hash** (`aym_map_find` → hash abierto con longitud y hash
   cacheados). Quita el comportamiento cuadrático de `map_ops`; es el único caso
   de 100x y hoy empeora sin límite al crecer N. Cambio local al runtime.
2. **Objetos con disposición fija**: resolver en compilación el desplazamiento
   de cada campo (`[obj + 8*k]`) y llamar a los métodos directamente
   (`call __Contador_suma`) cuando el tipo estático es conocido; tabla virtual
   por clase sólo para métodos sobrescritos. Elimina las 3 llamadas a
   `aym_map_get`/`aym_map_set` por invocación en `methods`.
3. **Selección de instrucciones sin pila**: operandos inmediatos
   (`cmp rax, 2000000`, `add rax, 1`), saltos condicionales directos
   (`cmp` + `jge`) y eliminar los pares `sub rsp,8`/`add rsp,8`. Es una pasada
   de mirilla (peephole) o un cambio en el emisor de expresiones binarias; afecta
   a todos los bucles.
4. **Variables locales y de bucle en registros** (asignación lineal simple
   sobre `rbx`, `r12`–`r15`) y guardar sólo los registros realmente usados en el
   prólogo. Beneficia a `fib`, `calls`, `int_loop`, `nested_loops`.
5. **Cadenas con longitud** (cabecera `len`/`cap`) y concatenación con
   capacidad amortizada cuando el destino es la misma variable (`s = s + x`).
   Arregla `str_concat` y de paso `largo`/`suyu` en O(1).
6. **Expansión en línea de funciones pequeñas** y `%`/`/` por constante mediante
   multiplicación. Menor prioridad: sólo rinde una vez hechos 3 y 4.

## Problemas encontrados al escribir los benchmarks

- `largo()` sobre un `aru` devuelve basura (`largo("abcd")` imprime
  `34190415261557345`; se esperaba `4`). `suyu()` sí funciona y es lo que usa
  `str_concat.aym`.
- `map_ops` tuvo que reducirse a 8000 claves por el costo cuadrático.

## Lo que no se verificó

- No se leyó el ASM de acceso a listas (`sieve`, `list_sum`), por lo que no se
  afirma nada sobre su costo.
- No hay medición con la máquina en reposo ni repetición estadística; el
  beneficio de cada cambio propuesto es una estimación, no una medida.
- Sólo se probó el backend `native` en Linux x86-64.
