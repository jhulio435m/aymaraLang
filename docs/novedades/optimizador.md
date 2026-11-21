# Optimizador: plegado de constantes, eliminación de ramas y mirilla

`aymc` combina ahora optimizaciones en tiempo de generación de código (a nivel
de AST) con una pasada ampliada de mirilla (`compiler/codegen/codegen_peephole.cpp`)
sobre el ASM NASM generado justo antes de ensamblar.

## Qué se optimiza

### A nivel de AST (`codegen_expr.cpp`, `codegen_stmt.cpp`)

1. **Plegado de constantes (`tryEvalConstant`).** Se evalúan en compilación:
   - Literales numéricos (`jakhüwi`) y booleanos (`chiqa` / `k'ari`).
   - Operadores unarios: `-`, `+`, `!` y complemento bit a bit `~`.
   - Operadores binarios aritméticos (`+`, `-`, `*`, `/`, `%`, `^` sobre enteros;
     la división y módulo por cero o `LLONG_MIN / -1` no se pliegan para conservar
     la excepción `ARITMETICA` en tiempo de ejecución).
   - Operadores relacionales (`==`, `!=`, `<`, `<=`, `>`, `>=`) y lógicos
     (`&&`, `||`) con cortocircuito estático cuando el operando izquierdo ya
     fija el resultado (`k'ari && expr` -> `0`, `chiqa || expr` -> `1`) sin
     necesidad de que el lado derecho sea constante.
   - Operadores bit a bit (`&`, `|`, `^^`, `<<`, `>>` con desplazamiento en `0..63`).
   - Expresión ternaria (`cond ? a : b`) cuando la condición es constante y la
     rama tomada también es constante.
2. **Eliminación de ramas constantes.**
   - En `ukaxa` / `maysatxa` (`IfStmt`) y en el operador ternario (`TernaryExpr`),
     si la condición se resuelve como constante en compilación, solo se emite
     la rama tomada y la rama muerta se descarta sin emitir instrucciones.

### A nivel de mirilla ASM (`codegen_peephole.cpp`)

El optimizador de mirilla se ejecuta en bucle hasta alcanzar punto fijo
(`while (changed)`) con estas reglas:

1. **Operando derecho simple en operaciones binarias.** La secuencia

   ```nasm
   push rax
   sub rsp, 8        ; o 40 en Win64
   mov rax, X
   add rsp, 8        ; o 40 en Win64
   mov rbx, rax
   pop rax
   ```

   se reemplaza por `mov rbx, X` cuando `X` es un entero inmediato, una variable
   local (`[rbp-N]` / `[rbp+N]`) o una variable global con direccionamiento
   relativo a RIP (`[rel $simbolo]`). Cualquier otro operando se deja intacto.
2. **Salto condicional directo tras comparación.** La secuencia de cuatro
   instrucciones

   ```nasm
   set<cc> al
   movzx rax,al
   cmp rax,0
   je <L>            ; o jne <L>
   ```

   se colapsa en una única instrucción de salto condicional (`jl`, `jle`, `jg`,
   `jge`, `je` o `jne <L>`) conservando las banderas generadas por el `cmp`
   previo.
3. **Saltos al rótulo siguiente.** `jmp L` seguido inmediatamente de `L:` se elimina.

## Cómo activar / desactivar

La pasada de mirilla está activa por defecto. Para desactivarla:

```bash
AYM_NO_PEEPHOLE=1 ./bin/aymc programa.aym -o build/programa
```

Con `--emit-asm` se conserva el `.asm` ya optimizado.

## Pruebas golden

- `tests/golden/optimizador/peephole_arit.aym`
- `tests/golden/optimizador/plegado_y_ramas.aym`

## Mediciones previas (ronda anterior, solo mirilla inicial)

Linux x86-64, mejor y peor de 3 ejecuciones (tiempo real). La máquina estaba
cargada por otros procesos durante la medición, así que los números son ruidosos.

| Programa (`samples/bench/opt/`) | Antes (s) | Después (s) |
|---|---|---|
| `fib_rec.aym` (fib(35)) | 0.143 – 0.152 | 0.089 – 0.093 |
| `arit_rec.aym` | 1.101 – 1.228 | 0.897 – 1.061 |

`samples/bench/pipeline_bench.aym`: el ASM pasó de 1215 a 1000 líneas con
salida idéntica byte a byte.
