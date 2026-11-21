# Algoritmos clásicos en AymaraLang

Programas deterministas que sirven de ejemplo y de prueba de regresión. Cada `.aym` tiene un `.expected` con su salida exacta.

- `algoritmos_texto.aym`: Algoritmos de texto: verificacion de palindromos, inversion de palabras, conteo de subcadenas/palabras y reemplazo.
- `burbuja.aym`: Ordenamiento burbuja: compara vecinos e intercambia hasta ordenar la lista.
- `busqueda_binaria.aym`: Busqueda binaria en una lista ordenada; devuelve el indice o -1.
- `cambio_monedas.aym`: Programacion dinamica: minimo de monedas y mochila 0/1.
- `conversion_base.aym`: Conversion de decimal a binario y a base 8 construyendo el texto digito a digito.
- `criba.aym`: Criba de Eratostenes: primos menores o iguales a 50.
- `factorial_combinaciones.aym`: Factorial recursivo y combinaciones C(n, k) con la formula multiplicativa.
- `fibonacci.aym`: Fibonacci iterativo y recursivo con memoizacion en un mapa.
- `grafo_bfs.aym`: BFS sobre un grafo de 6 nodos dado como lista de aristas; imprime distancias desde 0.
- `grafo_dfs.aym`: DFS recursivo sobre un grafo no dirigido de 7 nodos; imprime orden de visita, componentes por nodo y total de componentes conexas.
- `hanoi.aym`: Torres de Hanoi: imprime los movimientos para 3 discos y cuenta los de 10.
- `insercion.aym`: Ordenamiento por insercion: inserta cada elemento en su lugar dentro del prefijo ordenado.
- `lcs_dp.aym`: Programacion dinamica: subsecuencia comun mas larga (LCS), tabla DP y reconstruccion.
- `matrices.aym`: Multiplicacion de matrices 2x2 guardadas como listas planas (fila * 2 + columna).
- `mcd_mcm.aym`: Maximo comun divisor (Euclides) y minimo comun multiplo.
- `mergesort.aym`: Ordenamiento por mezcla (Merge Sort) recursivo sobre una lista.
- `pila_cola.aym`: Pila (LIFO) implementada con una clase y un contador de elementos.
- `potencia_rapida.aym`: Exponenciacion rapida (binaria) y exponenciacion modular.
- `quicksort.aym`: Quicksort recursivo (particion de Lomuto) sobre una lista global.
- `seleccion.aym`: Ordenamiento por seleccion: busca el minimo del resto y lo coloca al frente.

Ejecutar: `./bin/aymc samples/algoritmos/x.aym -o build/tmp/x && build/tmp/x`

Nota: las matrices se guardan como listas planas porque la indexación de listas anidadas (`m[i][j]`) falla actualmente.
