# Biblioteca de AymaraLang

Módulos reutilizables escritos en AymaraLang. Se importan con `apnaq`.

## Cómo importar

`apnaq("ruta")` busca `ruta.aym` relativo al directorio del archivo que importa
(luego en `modules/`, el directorio actual y las rutas de `AYM_PATH`). Las rutas con `..` funcionan:

```aym
apnaq("../matematica");            // desde biblioteca/pruebas/
apnaq("biblioteca/listas");        // con AYM_PATH=<raiz del repo>
qillqa(m_mcd(48, 18));
```

Importar un módulo incluye todas sus funciones/clases en el programa; por eso
cada módulo usa un prefijo (`m_`, `t_`, `l_`) para evitar choques de nombres.

## Módulos

| Módulo | Contenido |
| --- | --- |
| `matematica.aym` | `m_abs`, `m_min`, `m_max`, `m_limita`, `m_mcd`, `m_mcm`, `m_factorial`, `m_es_primo`, `m_potencia`, `m_raiz_entera`, `m_fibonacci`, `m_suma_lista`, `m_promedio` |
| `texto.aym` | `t_repetir`, `t_rellenar_izq`, `t_rellenar_der`, `t_contar`, `t_invertir`, `t_es_palindromo`, `t_empieza_con`, `t_termina_con`, `t_buscar`, `t_mayusculas`, `t_minusculas`, `t_recortar` |
| `listas.aym` | `l_invertir`, `l_indice`, `l_contiene`, `l_rebanada`, `l_min`, `l_max`, `l_suma`, `l_unicos`, `l_ordenar`, `l_quicksort`, `l_busqueda_binaria`, `l_rango`, `l_concatenar`, `l_producto_punto`, `l_filtrar_positivos`, `l_aplanar_pares` |
| `estructuras.aym` | clases `Pila`, `Cola`, `ListaEnlazada` (de números), `Conjunto`, `ContadorMapa` (claves de texto) |

## Referencia de módulos

### `matematica.aym`

Utilidades numéricas enteras.

- `m_abs(jakhüwi x): jakhüwi`: devuelve el valor absoluto de `x`.
- `m_min(jakhüwi a, jakhüwi b): jakhüwi`: devuelve el menor entre `a` y `b`.
- `m_max(jakhüwi a, jakhüwi b): jakhüwi`: devuelve el mayor entre `a` y `b`.
- `m_limita(jakhüwi x, jakhüwi lo, jakhüwi hi): jakhüwi`: acota `x` al rango cerrado `[lo, hi]`.
- `m_mcd(jakhüwi a, jakhüwi b): jakhüwi`: máximo común divisor de `a` y `b`.
- `m_mcm(jakhüwi a, jakhüwi b): jakhüwi`: mínimo común múltiplo de `a` y `b` (`0` si alguno es `0`).
- `m_factorial(jakhüwi n): jakhüwi`: devuelve `n!` (para `n >= 0`).
- `m_es_primo(jakhüwi n): chiqa`: devuelve `chiqa` si `n` es primo.
- `m_potencia(jakhüwi b, jakhüwi e): jakhüwi`: calcula `b^e` mediante exponenciación rápida (`e >= 0`).
- `m_raiz_entera(jakhüwi n): jakhüwi`: parte entera inferior de la raíz cuadrada de `n` (`n >= 0`).
- `m_fibonacci(jakhüwi n): jakhüwi`: `n`-ésimo número de Fibonacci (`F(0) = 0`, `F(1) = 1`).
- `m_suma_lista(t'aqa xs): jakhüwi`: suma de los elementos numéricos de `xs`.
- `m_promedio(t'aqa xs): jakhüwi`: promedio entero de `xs` (`0` si está vacía).

### `texto.aym`

Utilidades para cadenas (`aru`).

- `t_repetir(aru s, jakhüwi n): aru`: repite `s` exactamente `n` veces.
- `t_rellenar_izq(aru s, jakhüwi n, aru c): aru`: rellena `s` con `c` a la izquierda hasta alcanzar al menos largo `n` en bytes.
- `t_rellenar_der(aru s, jakhüwi n, aru c): aru`: rellena `s` con `c` a la derecha hasta alcanzar al menos largo `n` en bytes.
- `t_contar(aru s, aru sub): jakhüwi`: número de apariciones no solapadas de `sub` en `s` (usa `jaljta`; `sub` no debe ser vacío).
- `t_invertir(aru s): aru`: invierte `s` respetando caracteres UTF-8 multibyte (usa `kutikipa`).
- `t_es_palindromo(aru s): chiqa`: devuelve `chiqa` si `s` es palíndromo comparando `jiskanchaya(s)` con su inverso.
- `t_empieza_con(aru s, aru pref): chiqa`: devuelve `chiqa` si `s` comienza con `pref` (usa `qalltati`).
- `t_termina_con(aru s, aru suf): chiqa`: devuelve `chiqa` si `s` termina con `suf` (usa `tukuyati`).
- `t_buscar(aru s, aru sub): jakhüwi`: posición en bytes de la primera aparición de `sub` en `s`, o `-1` si no aparece (usa `jikxata`).
- `t_mayusculas(aru s): aru`: devuelve una copia con letras ASCII convertidas a mayúsculas (usa `jachanchaya`).
- `t_minusculas(aru s): aru`: devuelve una copia con letras ASCII convertidas a minúsculas (usa `jiskanchaya`).
- `t_recortar(aru s): aru`: recorta espacios en blanco al inicio y al final de `s` (usa `ch'usa`).

Notas de `texto.aym`:
- Para unir o partir listas de textos se usan directamente los builtins `mayachta(lista, sep)` y `jaljta(s, sep)`.
- `t_capitalizar` no se incluye porque el lenguaje no expone aún extracción de subcadenas ni indexación sobre `aru`.

### `listas.aym`

Utilidades para listas (`t'aqa`) de números (`jakhüwi`). Ninguna función muta las listas de entrada.

- `l_invertir(t'aqa xs): t'aqa`: devuelve una nueva lista con los elementos en orden inverso.
- `l_indice(t'aqa xs, jakhüwi v): jakhüwi`: devuelve el índice de la primera aparición de `v` en `xs`, o `-1`.
- `l_contiene(t'aqa xs, jakhüwi v): chiqa`: devuelve `chiqa` si `v` está en `xs`.
- `l_rebanada(t'aqa xs, jakhüwi a, jakhüwi b): t'aqa`: devuelve la sublista con los índices `[a, b)`.
- `l_min(t'aqa xs): jakhüwi`: devuelve el elemento mínimo de `xs` (lista no vacía).
- `l_max(t'aqa xs): jakhüwi`: devuelve el elemento máximo de `xs` (lista no vacía).
- `l_suma(t'aqa xs): jakhüwi`: suma de todos los elementos de `xs`.
- `l_unicos(t'aqa xs): t'aqa`: devuelve una nueva lista sin duplicados preservando el orden de aparición.
- `l_ordenar(t'aqa xs): t'aqa`: devuelve una nueva lista ordenada ascendentemente mediante ordenamiento por inserción.
- `l_quicksort(t'aqa xs): t'aqa`: devuelve una nueva lista ordenada ascendentemente mediante quicksort (auxiliar interno `l_quicksort_rango(t'aqa r, jakhüwi lo, jakhüwi hi)`).
- `l_busqueda_binaria(t'aqa xs, jakhüwi v): jakhüwi`: busca `v` en una lista ordenada ascendentemente; devuelve su índice o `-1`.
- `l_rango(jakhüwi a, jakhüwi b): t'aqa`: devuelve `[a, a+1, ..., b-1]`.
- `l_concatenar(t'aqa xs, t'aqa ys): t'aqa`: devuelve una nueva lista con `xs` seguido de `ys`.
- `l_producto_punto(t'aqa a, t'aqa b): jakhüwi`: calcula el producto punto de `a` y `b`; lanza `pantja("ARG", ...)` si sus longitudes difieren.
- `l_filtrar_positivos(t'aqa xs): t'aqa`: devuelve una nueva lista con los elementos estrictamente mayores que `0`.
- `l_aplanar_pares(t'aqa a, t'aqa b): t'aqa`: intercala los pares de `a` y `b` en una lista plana `[a0, b0, a1, b1, ...]` hasta el largo de la lista más corta.

### `estructuras.aym`

Estructuras de datos orientadas a objetos (`kasta`).

- `Pila`: pila LIFO de números (`jakhüwi`).
  - `apilar(jakhüwi v)`: inserta `v` en la cima.
  - `desapilar(): jakhüwi`: extrae y devuelve el elemento en la cima (lanza `"VACIO"` si está vacía).
  - `cima(): jakhüwi`: devuelve el elemento en la cima sin extraerlo.
  - `tamano(): jakhüwi`: cantidad de elementos en la pila.
  - `vacia(): chiqa`: `chiqa` si la pila no tiene elementos.
- `Cola`: cola FIFO de números (`jakhüwi`).
  - `encolar(jakhüwi v)`: agrega `v` al final de la cola.
  - `desencolar(): jakhüwi`: extrae y devuelve el elemento del frente.
  - `frente(): jakhüwi`: devuelve el elemento del frente sin extraerlo.
  - `tamano(): jakhüwi`: cantidad de elementos pendientes en la cola.
  - `vacia(): chiqa`: `chiqa` si la cola no tiene elementos.
- `ListaEnlazada`: lista simplemente enlazada de números (`jakhüwi`) respaldada por un pool de arreglos paralelos (`datos`, `sig`, `cabeza`, `tam`).
  - `insertar_inicio(jakhüwi v)`: inserta `v` al inicio de la lista en `O(1)`.
  - `insertar_final(jakhüwi v)`: inserta `v` al final de la lista.
  - `contiene(jakhüwi v): chiqa`: devuelve `chiqa` si `v` se encuentra en la lista.
  - `a_lista(): t'aqa`: materializa la lista enlazada en orden como una `t'aqa`.
  - `tamano(): jakhüwi`: cantidad de nodos en la lista.
- `Conjunto`: conjunto de textos (`aru`) respaldado por `mapa`.
  - `agregar(aru k)`: inserta la clave `k`.
  - `contiene(aru k): chiqa`: devuelve `chiqa` si `k` pertenece al conjunto.
  - `quitar(aru k)`: elimina la clave `k` (lanza `"CLAVE"` si no existe).
  - `tamano(): jakhüwi`: cantidad de elementos distintos.
- `ContadorMapa`: mapa de frecuencias para claves de texto (`aru`).
  - `contar(aru k)`: incrementa en `1` el contador de `k`.
  - `cuenta(aru k): jakhüwi`: devuelve la frecuencia actual de `k` (`0` si no existe).
  - `distintos(): jakhüwi`: cantidad de claves distintas registradas.

## Pruebas

```bash
mkdir -p build/tmp/m3_s2
for m in matematica texto listas estructuras; do
  ./bin/aymc biblioteca/pruebas/prueba_$m.aym -o build/tmp/m3_s2/$m &&
  build/tmp/m3_s2/$m | diff - biblioteca/pruebas/prueba_$m.expected
done
```

## Limitaciones conocidas

- Un parámetro `t'aqa` no acepta listas de textos; las funciones de listas son numéricas.
- Dentro de una clase, indexar un campo directamente (`aka.datos[i]`, `aka.m[k] = v`) no funciona; se usa un alias local (`yatiya t'aqa d = aka.datos;`).
- Para imprimir con `qillqa` una lista devuelta por un método (como `le.a_lista()`), conviene guardarla primero en una variable `yatiya t'aqa ...` para que el compilador reconozca el tipo lista al formatear la salida.
