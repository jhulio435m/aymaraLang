#!/usr/bin/env node
// Copia a src/content/samples.json los programas de ../samples que la web
// muestra, para que el sitio compile sin depender de archivos fuera de web/.
//
//   node scripts/sync-samples.mjs
import { readFile, writeFile } from "node:fs/promises";
import { fileURLToPath } from "node:url";

const root = new URL("../../", import.meta.url);
const out = fileURLToPath(new URL("../src/content/samples.json", import.meta.url));

const FILES = [
  "samples/apps/n_reinas.aym",
  "samples/apps/banca_excepciones.aym",
  "samples/modulos/modulos_imports.aym",
  "samples/poo/poo.aym",
  "samples/colecciones/mapas.aym",
  "samples/apps/inventario_archivo.aym",
  "samples/games/console/tetris.aym",
  "samples/games/console/katari.aym",
  "samples/games/console/jamachi.aym",
  "samples/games/console/sudoku_anatawi.aym",
  "samples/games/gui/tetris.aym",
  "samples/games/gui/katari_uja.aym",
  "samples/games/gui/jamachi_uja.aym",
  "samples/games/gui/sudoku_uja.aym",
  "samples/poo/paleta_uja_poo.aym",
];

const data = {};
for (const path of FILES) {
  const source = (await readFile(new URL(path, root), "utf8")).replace(/\r\n/g, "\n").replace(/\s+$/, "");
  data[path] = { source, lines: source.split("\n").length };
}
await writeFile(out, `${JSON.stringify(data, null, 1)}\n`);
console.log(`sync-samples: ${FILES.length} programas → src/content/samples.json`);
