# Juegos de AymaraLang

Este directorio agrupa los juegos de ejemplo separados por interfaz:

- `console/`: versiones en terminal.
- `gui/`: versiones con ventana grafica (`uja_*`).

## Pares disponibles

| Juego | Consola | GUI |
| --- | --- | --- |
| Sudoku | `console/sudoku_anatawi.aym` y `console/sudoku_4x4.aym` | `gui/sudoku_uja.aym` |
| Tetris | `console/tetris.aym` | `gui/tetris.aym` |
| Katari | `console/katari.aym` | `gui/katari_uja.aym` |
| Jamachi | `console/jamachi.aym` | `gui/jamachi_uja.aym` |

## Ejecucion rapida

```powershell
build/bin/Release/aymc.exe samples/games/console/sudoku_anatawi.aym
samples/games/console/sudoku_anatawi.exe
```

```powershell
build/bin/Release/aymc.exe samples/games/gui/sudoku_uja.aym
samples/games/gui/sudoku_uja.exe
```
