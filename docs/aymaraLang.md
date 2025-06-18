# AymaraLang

AymaraLang (`aym`) es un lenguaje de programación experimental con sintaxis inspirada en Python y un compilador escrito en C++17. Genera código NASM para x86_64 y enlaza con `gcc`.

## Sintaxis soportada
- Tipos: `jach’a`, `lliphiphi`, `chuymani`, `qillqa`
- Variables y asignación
- Impresión con `willt’aña(expr)`
- Control de flujo: `si`/`sino`, `mientras`, `haceña...mientras`, `para`, `tantachaña`
- Funciones con `luräwi nombre(params) { ... }`
- Expresiones aritméticas `+ - * / % ^`
- Operadores lógicos `uka`, `jan uka`, `janiwa`, comparaciones `== != < <= > >=`
- Comentarios `//` y `/* */`
- Lectura de consola con `input()`
- Longitud de cadenas con `length()`
$ ./bin/archivo
## Compilación
Para compilar un archivo `.aym` se ejecuta:

```bash
$ ./bin/aymc samples/archivo.aym
$ ./build/out
```

El compilador produce un archivo NASM, lo ensambla y enlaza automáticamente.

## Errores comunes
- Variable no declarada
- Tipos incompatibles en asignaciones o expresiones
- Uso de `break` o `continue` fuera de bucles
- `return` fuera de una función

## Palabras clave
`willt’aña`, `si`, `sino`, `mientras`, `haceña`, `para`, `en`, `jalña`, `sarantaña`, `luräwi`, `kutiyana`, `tantachaña`, `jamusa`, `akhamawa`, `uka`, `jan uka`, `janiwa`, `jach’a`, `lliphiphi`, `chuymani`, `qillqa`, `input`, `length`

### Valores booleanos
En `aym` los literales lógicos utilizan vocabulario aimara. La palabra
`cheka` representa **verdadero** (`true`) y la expresión `jan cheka`
equivale a **falso** (`false`). Son útiles para inicializar variables y
para las condiciones en estructuras de control.

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

## Ejemplo
```aymara
qillqa nombre = input();
willt’aña(nombre);
jach’a edad = input();
willt’aña(edad);
willt’aña(length(nombre));
```

### Recursividad
```aymara
luräwi fact(n) {
    si (n == 0) {
        kutiyana(1);
    }
    kutiyana(n * fact(n - 1));
}

willt’aña(fact(5));
```

## Glosario de palabras clave

| Aimara / Español | Significado |
|------------------|------------|
| `willt’aña`      | imprimir |
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

