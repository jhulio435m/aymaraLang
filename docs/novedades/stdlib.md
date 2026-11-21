# Novedades de la biblioteca estándar

Nuevas funciones integradas para cadenas (`aru`), listas (`t'aqa`), matemática entera (`jakhüwi`) y archivos/sistema. Ninguna función de transformación de texto o listas modifica el argumento original; todas devuelven un valor o colección nueva.

## Cadenas (`aru`)

| Nombre | Parámetros | Devuelve | Ejemplo |
| --- | --- | --- | --- |
| `jachanchaya` | `(aru texto)` | `aru` en mayúsculas | `jachanchaya("aymara")` -> `AYMARA` |
| `jiskanchaya` | `(aru texto)` | `aru` en minúsculas | `jiskanchaya("AYMARA")` -> `aymara` |
| `kutikipa` | `(aru texto)` | `aru` invertida | `kutikipa("abc")` -> `cba` |
| `kuti_aru` | `(aru texto, jakhüwi veces)` | `aru` repetida | `kuti_aru("ab", 3)` -> `ababab` |
| `qalltati` | `(aru texto, aru prefijo)` | `chiqa` (1/0): empieza con el prefijo | `qalltati("kamisaki", "kami")` -> `1` |
| `tukuyati` | `(aru texto, aru sufijo)` | `chiqa` (1/0): termina con el sufijo | `tukuyati("kamisaki", "saki")` -> `1` |
| `jikxata` | `(aru texto, aru buscado)` | `jakhüwi`: posición (desde 0) o `-1` | `jikxata("kamisaki", "saki")` -> `4` |
| `aru_chimpu` | `(aru texto, jakhüwi indice)` | `aru` de un carácter en `indice` | `aru_chimpu("kamisaki", 0)` -> `k` |
| `jalja_aru` | `(aru texto, jakhüwi inicio, jakhüwi cantidad)` | `aru` subcadena `[inicio, inicio + cantidad)` | `jalja_aru("kamisaki", 4, 4)` -> `saki` |
| `chimpu_jakhuwi` | `(aru texto)` | `jakhüwi`: código ASCII/byte (0..255) del primer carácter o `-1` si vacío | `chimpu_jakhuwi("A")` -> `65` |
| `jakhuwi_chimpu` | `(jakhüwi codigo)` | `aru` de 1 byte para `codigo` (0..255) | `jakhuwi_chimpu(65)` -> `A` |

### Notas sobre cadenas

- `jachanchaya` y `jiskanchaya` solo convierten letras ASCII; las letras acentuadas (`ñ`, `ü`, ...) quedan sin cambios.
- `kutikipa` respeta los caracteres UTF-8 multibyte (no los rompe).
- `kuti_aru` con `veces <= 0` devuelve la cadena vacía; si el resultado superaría 1 GiB lanza un error de tipo `ARG`.
- `jikxata` devuelve la posición en bytes; con texto ASCII coincide con la posición del carácter. Si el texto buscado es vacío devuelve `0`.
- `aru_chimpu` lanza una excepción de tipo `INDICE` si `indice < 0` o `indice >= longitud`.
- `jalja_aru` recorta de forma segura `inicio` al rango `[0, longitud]` y `cantidad` a `[0, longitud - inicio]`.
- `jakhuwi_chimpu` lanza una excepción de tipo `ARG` si `codigo < 0` o `codigo > 255`.

## Listas (`t'aqa`)

| Nombre | Parámetros | Devuelve | Ejemplo |
| --- | --- | --- | --- |
| `kutikipat` | `(t'aqa lista)` | Nueva `t'aqa` en orden inverso | `kutikipat([1, 2, 3])` -> `[3, 2, 1]` |
| `jaljat` | `(t'aqa lista, jakhüwi inicio, jakhüwi fin)` | Nueva `t'aqa` con el segmento `[inicio, fin)` | `jaljat([10, 20, 30, 40], 1, 3)` -> `[20, 30]` |
| `thanta` | `(t'aqa lista)` | `jakhüwi`: suma de los elementos (`0` si vacía) | `thanta([1, 2, 3])` -> `6` |
| `jiska_taqa` | `(t'aqa lista)` | `jakhüwi`: elemento mínimo | `jiska_taqa([3, 1, 4])` -> `1` |
| `jacha_taqa` | `(t'aqa lista)` | `jakhüwi`: elemento máximo | `jacha_taqa([3, 1, 4])` -> `4` |
| `wakicha_qhipa` | `(t'aqa lista)` | Nueva `t'aqa` ordenada de forma descendente (`jakhüwi` o `aru`) | `wakicha_qhipa([2, 5, 1])` -> `[5, 2, 1]` |

### Notas sobre listas

- `jaljat` recorta `inicio` y `fin` de forma segura al rango `[0, longitud]`; si `fin <= inicio` devuelve `[]`.
- `jiska_taqa` y `jacha_taqa` lanzan una excepción de tipo `VACIO` si la lista es nula o está vacía.
- `wakicha_qhipa` admite tanto listas de enteros (`t'aqa:jakhüwi`) como listas de cadenas (`t'aqa:aru`).

## Matemática entera (`jakhüwi`)

| Nombre | Parámetros | Devuelve | Ejemplo |
| --- | --- | --- | --- |
| `jakhu_chani` | `(jakhüwi x)` | `jakhüwi`: valor absoluto | `jakhu_chani(-42)` -> `42` |
| `jakhu_jiska` | `(jakhüwi a, jakhüwi b)` | `jakhüwi`: mínimo entre `a` y `b` | `jakhu_jiska(10, 3)` -> `3` |
| `jakhu_jacha` | `(jakhüwi a, jakhüwi b)` | `jakhüwi`: máximo entre `a` y `b` | `jakhu_jacha(10, 3)` -> `10` |
| `jakhu_mcd` | `(jakhüwi a, jakhüwi b)` | `jakhüwi`: máximo común divisor | `jakhu_mcd(48, 18)` -> `6` |
| `jakhu_limita` | `(jakhüwi x, jakhüwi min, jakhüwi max)` | `jakhüwi`: `x` acotado a `[min, max]` | `jakhu_limita(25, 1, 10)` -> `10` |

### Notas sobre matemática

- `jakhu_mcd` maneja valores negativos mediante su magnitud sin desbordamiento (`jakhu_mcd(-24, 36)` -> `12`).
- `jakhu_limita` garantiza que el resultado quede dentro del intervalo cerrado `[min, max]`.

## Archivos y sistema

| Nombre | Parámetros | Devuelve | Ejemplo |
| --- | --- | --- | --- |
| `yapxata_aru` | `(aru ruta, aru texto)` | `jakhüwi` (1 si tuvo éxito, 0 si falló): agrega `texto` al final del archivo | `yapxata_aru("log.txt", "linea\n")` -> `1` |
| `ullana_siqi` | `(aru ruta)` | `t'aqa` de `aru`: líneas del archivo sin `\r`/`\n` | `ullana_siqi("log.txt")` -> `["linea"]` |
| `pichha_arkata` | `(aru ruta)` | `jakhüwi` (1 si eliminó, 0 si falló) | `pichha_arkata("log.txt")` -> `1` |
| `pacha_aru` | `(aru nombre)` | `aru`: valor de la variable de entorno o `""` | `pacha_aru("PATH")` |
| `mistuna` | `(jakhüwi codigo)` | Termina el proceso vaciando `stdout` con código `codigo` | `mistuna(0)` |

### Notas sobre archivos y sistema

- `ullana_siqi` devuelve `[]` si el archivo no existe o está vacío, y elimina tanto `\n` como `\r\n` al final de cada línea.
- `pacha_aru` devuelve `""` si la variable de entorno no está definida.
- `mistuna` ejecuta `fflush(stdout)` antes de salir para evitar pérdida de salida cuando la salida estándar está redirigida.

## Mejoras de rendimiento y runtime

- **Mapas hash O(1):** `mapa` utiliza una tabla hash con direccionamiento abierto (FNV-1a de 64 bits y factor de carga `< 0.5`) manteniendo el orden exacto de inserción para `sutinaka`, `chaninaka` y la impresión.
- **Excepciones no manejadas:** si una excepción no es atrapada por `yant'aña`/`katjaña`, el runtime imprime `Excepcion no manejada [TIPO]: mensaje` en `stderr` antes de salir con código `1`.
