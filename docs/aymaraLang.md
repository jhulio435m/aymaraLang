# Referencia del lenguaje

Esta referencia describe la sintaxis completa, operadores, palabras clave y
funciones integradas de AymaraLang. Está pensada para consulta rápida durante el
desarrollo.

## Tipos soportados

- `jach’a` → enteros
- `lliphiphi` → flotantes
- `chuymani` → booleanos
- `qillqa` → cadenas

### Valores booleanos

En `aym` los literales lógicos utilizan vocabulario aymara. La palabra `cheka`
representa **verdadero** (`true`) y la expresión `jan cheka` equivale a **falso**
(`false`). Son útiles para inicializar variables y para las condiciones en
estructuras de control.

```aymara
chuymani bandera = cheka;
si (bandera) {
    willt’aña("activado");
}

chuymani otra = jan cheka;
si (otra) {
    willt’aña("esto no se imprime");
} sino {
    willt’aña("desactivado");
}
```

## Sintaxis soportada

- Variables y asignación.
- Impresión con `willt’aña(expr)` y `write(str)`.
- Control de flujo: `si`/`sino`, `mientras`, `haceña...mientras`, `para`, `tantachaña`.
- Funciones con `luräwi nombre(params) { ... }`.
- Expresiones aritméticas `+ - * / % ^` y operadores unarios `-expr`, `+expr`, `!expr`.
- Operadores lógicos `uka`, `jan uka`, `janiwa`, comparaciones `== != < <= > >=`.
- Comentarios `//` y `/* */`.
- Lectura de consola con `input()`.
- Longitud de cadenas con `length()`.
- Números aleatorios con `random(max)`.
- Pausa de ejecución con `sleep(ms)`.
- Arreglos dinámicos con `array(n)`, `array_get(arr, i)`, `array_set(arr, i, v)`,
  `array_free(arr)`, `array_length(arr)`.
- Funciones matemáticas con `sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `sqrt`,
  `pow`, `exp`, `log`, `log10`, `floor`, `ceil`, `round`, `fabs`.

## Ejemplos de sintaxis

### Entrada y salida
```aymara
qillqa nombre = input();
willt’aña(nombre);
```

### Arreglos dinámicos
```aymara
jach’a size = 3;
jach’a arr = array(size);
array_set(arr, 0, 10);
array_set(arr, 1, 20);
array_set(arr, 2, 30);
willt’aña(array_get(arr, 1));
array_free(arr);
```

### Funciones y recursividad
```aymara
luräwi fact(n) {
    si (n == 0) {
        kutiyana(1);
    }
    kutiyana(n * fact(n - 1));
}

willt’aña(fact(5));
```

### Bucle con `range`

La forma `para x en range(inicio, fin)` simplifica la iteración sobre rangos
numéricos.

```aymara
para x en range(0, 4) {
    willt’aña(x);
}
```

Este fragmento puede encontrarse en `samples/control_flow/range_for.aym`.

## Errores comunes

- Variable no declarada.
- Tipos incompatibles en asignaciones o expresiones.
- Uso de `break` o `continue` fuera de bucles.
- `return` fuera de una función.

## Palabras clave

`willt’aña`, `write`, `sleep`, `array`, `array_get`, `array_set`, `array_free`,
`array_length`, `sin`, `cos`, `tan`, `asin`, `acos`, `atan`, `sqrt`, `pow`, `exp`,
`log`, `log10`, `floor`, `ceil`, `round`, `fabs`, `si`, `sino`, `mientras`,
`haceña`, `para`, `en`, `jalña`, `sarantaña`, `luräwi`, `kutiyana`, `tantachaña`,
`jamusa`, `akhamawa`, `uka`, `jan uka`, `janiwa`, `jach’a`, `lliphiphi`,
`chuymani`, `qillqa`, `input`, `length`, `random`.

## Glosario de palabras clave

| Aimara / Español | Significado |
|------------------|------------|
| `willt’aña`      | imprimir |
| `write`          | imprimir sin salto |
| `si`             | if |
| `sino`           | else |
| `mientras`       | while |
| `haceña`         | do |
| `para`           | for |
| `en`             | in |
| `tantachaña`     | switch |
| `jamusa`         | case |
| `akhamawa`       | default |
| `jalña`          | break |
| `sarantaña`      | continue |
| `luräwi`         | func |
| `kutiyana`       | return |
| `jach’a`         | int |
| `lliphiphi`      | float |
| `chuymani`       | bool |
| `qillqa`         | string |
| `uka`            | and |
| `jan uka`        | or |
| `janiwa`         | not |

---

**Anterior:** [Guía de características](guide.md) | **Siguiente:** [Gramática formal](grammar.md)
