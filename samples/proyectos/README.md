# Proyectos de ejemplo

Programas mas grandes que combinan varias caracteristicas de AymaraLang.
Cada `<nombre>.aym` tiene un `<nombre>.expected` con su salida estandar exacta,
de modo que sirven tambien como pruebas de regresion. Todos son deterministas
y no interactivos.

| Programa | Que muestra |
| --- | --- |
| `banco.aym` | Jerarquia de cuentas (`kasta`/`jila`), sobrescritura, excepciones `pantja`/`katjaña` |
| `inventario.aym` | Punto de venta con mapas, varias clausulas `katjaña`, `mayjtaya`/`ajlli`/`thaqthapi` |
| `semaforo.aym` | Maquina de estados con `siqicha` y `khiti` |
| `maquina_pila.aym` | Maquina virtual de pila con opcodes en un enum y errores de ejecucion |
| `figuras.aym` | Polimorfismo con herencia en tres niveles |
| `matrices.aym` | Clase `Matriz` (producto, traspuesta, traza) y error de dimensiones |
| `tareas.aym` + `tareas_util.aym` | Gestor de tareas en dos archivos con `apnaq` |
| `cola_banco.aym` | Simulacion de una cola FIFO con estadisticas |
| `frecuencias.aym` | Frecuencia de palabras con mapas y funciones de orden superior |
| `calculadora_ast.aym` | Tokenizador, parser descendente recursivo, AST polimorfico y manejo de errores |
| `tres_en_raya_ia.aym` | Tres en Raya determinista: IA Minimax con poda alfa-beta frente a heuristicas |

## Ejecutar

```bash
mkdir -p build/tmp/m2
./bin/aymc samples/proyectos/banco.aym -o build/tmp/m2/banco && build/tmp/m2/banco
```

## Verificar todos

```bash
for f in banco inventario semaforo maquina_pila figuras matrices tareas cola_banco frecuencias calculadora_ast tres_en_raya_ia; do
  ./bin/aymc samples/proyectos/$f.aym -o build/tmp/m2/$f >/dev/null && \
    build/tmp/m2/$f | diff - samples/proyectos/$f.expected && echo "OK $f"
done
```

`tareas_util.aym` es un modulo, no un programa: no tiene `.expected`.
Algunos programas copian campos a variables locales antes de indexarlos o
pasarlos como argumento; es una forma de evitar limitaciones actuales del
compilador con accesos a miembro anidados.
