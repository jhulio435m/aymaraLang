# Números decimales (`chikata`)

AymaraLang tiene un tipo decimal de punto flotante de 64 bits (IEEE 754) junto
al entero `jakhüwi`.

```aym
yatiya chikata pi = 3.14159;
yatiya chikata r = 2;              // el entero se convierte solo: 2.0
qillqa(pi * r * r);                // 12.56636
qillqa(7 / 2);                     // 3   (entero / entero = entero)
qillqa(7 / 2.0);                   // 3.5 (si hay un chikata, el resultado es chikata)
qillqa(jakhüwi(9.99));             // 9   (trunca hacia cero)
qillqa(round(2.5) + 1);            // 4   (floor/ceil/round devuelven jakhüwi)
qillqa("total: " + aru(1.25));     // total: 1.25
```

## Reglas

- **Literales:** `3.14`, `0.5`, `1e6`, `2.5e-3`. Debe haber un dígito antes y después del punto (`.5` y `5.` no son válidos).
- **Promoción:** en `+ - * / % ^` y en comparaciones, si un operando es `chikata` el `jakhüwi` se convierte a decimal. Entre dos enteros todo sigue siendo entero.
- **Conversión implícita solo hacia `chikata`:** se puede asignar, pasar como argumento o devolver un `jakhüwi` donde se espera `chikata`, también en literales de lista (`t'aqa<chikata> v = [1, 2, 3]`). Al revés es un error de compilación: usa `jakhüwi(x)`, `floor(x)`, `ceil(x)` o `round(x)`.
- **`kuttaya` se valida:** devolver un tipo distinto al declarado es un error (`AYM3003`).
- **No admite:** operadores bit a bit (`& | ^^ << >> ~`) ni concatenación directa con `aru` (usa `aru(x)`).
- **Semántica IEEE:** `1.0 / 0` es `inf`, `sqrt(-1)` es `nan`; la división decimal por cero no lanza excepción (la entera sí).
- **Impresión:** 15 cifras significativas y siempre con parte decimal: `2.0`, `0.3`, `1e+20`, `inf`, `nan`.

## Builtins

| Función | Resultado |
|---|---|
| `chikata(x)` | convierte `jakhüwi` o `aru` (`"2.75"`) a decimal; texto inválido lanza `CONVERSION` |
| `jakhüwi(x)` | trunca un `chikata` |
| `sin cos tan asin acos atan sqrt exp log log10 pow fabs` | `chikata` (antes redondeaban a entero) |
| `floor ceil round` | `jakhüwi` |
| `thanta jiska_taqa jacha_taqa` sobre `t'aqa<chikata>` | `chikata` |
| `wakicha wakicha_qhipa` sobre `t'aqa<chikata>` | ordenan numéricamente |

Listas, mapas, campos de clase, parámetros (incluidos más de 6), lambdas en
`map`/`filter`/`reduce` y la interpolación `$"..."` funcionan con decimales.

## Implementación

- El valor viaja como los 64 bits del `double` en `rax`, igual que el resto de valores: no cambian pilas, listas ni mapas. La aritmética usa SSE2 en línea (`addsd`, `ucomisd`…); `%` y `^` llaman a `aym_fmod`/`aym_fpow`.
- El análisis semántico anota en cada `Expr` su tipo (`semType`) y dónde hace falta convertir (`convertTo`). El codegen aplica la conversión (`cvtsi2sd`) en un único punto, `emitExpr`.
- El runtime usa enteros de 64 bits para pasar los decimales (`int64_t` con los bits del double), así no depende de los registros `xmm` de cada ABI.
- Los mapas guardan el tipo de cada valor (0 entero, 1 `aru`, 2 `chikata`) para imprimirlos bien.
- Archivos: `compiler/lexer/*`, `compiler/ast/ast.h`, `compiler/parser/*`, `compiler/semantic/*`, `compiler/codegen/codegen_expr.cpp`, `codegen_type_print.cpp`, `codegen_builtins.cpp`, `runtime/math.c`, `runtime/runtime_arrays.c`, `runtime/runtime_maps_strings.c`.
- Pruebas: `tests/golden/decimales/`.

## Limitaciones

- `khiti` sobre `chikata` compara los bits exactos (no recomendado).
- Una condición `ukaxa (x)` con `x` de tipo `chikata` trata `-0.0` como verdadero.
- No se probó todavía en Windows (el código generado es el mismo, con `cvtsi2sd`/`movq`).
