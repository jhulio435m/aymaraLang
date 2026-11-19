import release from "./release.json";
import samples from "./samples.json";

export const SITE_URL = process.env.NEXT_PUBLIC_SITE_URL || "https://aymaralang.soryntel.com";
export const REPO_URL = "https://github.com/jhulio435m/aymaraLang";
export const DOCS_URL = "https://jhulio435m.github.io/aymaraLang/";
export const SOURCE_URL = (path: string) => `${REPO_URL}/blob/main/${path}`;

export const TITLE = "AymaraLang · Programa en aymara, compila a nativo";
export const DESCRIPTION =
  "Lenguaje de programación con palabras clave en aymara y compilador propio, aymc, que genera ejecutables nativos. Instaladores para Windows y Linux.";

export const KEYWORDS = [
  "AymaraLang",
  "lenguaje de programación en aymara",
  "programar en aymara",
  "aymc",
  "compilador aymara",
  "lenguaje de programación peruano",
  "aprender a programar",
  "lenguas originarias",
];

export const HELLO = `qallta
qillqa("Kamisaraki!")
tukuya`;

type SampleKey = keyof typeof samples;
const sample = (path: SampleKey) => ({ path, ...samples[path] });

/** Pestañas de la sección de ejemplos: archivos completos del repositorio. */
export const EXAMPLES = [
  { id: "reinas", label: "Recursión", note: "Las ocho reinas con vuelta atrás.", ...sample("samples/apps/n_reinas.aym") },
  { id: "banca", label: "Clases y errores", note: "Cuentas, transferencias y excepciones.", ...sample("samples/apps/banca_excepciones.aym") },
  { id: "poo", label: "Herencia", note: "Clases, constructores y métodos heredados.", ...sample("samples/poo/poo.aym") },
  { id: "mapas", label: "Mapas", note: "Claves, valores y recorridos.", ...sample("samples/colecciones/mapas.aym") },
  { id: "modulos", label: "Módulos", note: "Importar funciones de otros archivos.", ...sample("samples/modulos/modulos_imports.aym") },
];

/** Programas del repositorio que cuelgan como tejidos en «Hecho con AymaraLang». */
export const WORKS = [
  { name: "Tetris", kind: "Juego con ventana", ...sample("samples/games/gui/tetris.aym") },
  { name: "Sudoku", kind: "Juego con ventana", ...sample("samples/games/gui/sudoku_uja.aym") },
  { name: "Katari", kind: "Juego con ventana", ...sample("samples/games/gui/katari_uja.aym") },
  { name: "Jamachi", kind: "Juego con ventana", ...sample("samples/games/gui/jamachi_uja.aym") },
  { name: "Paleta", kind: "Ventana con clases", ...sample("samples/poo/paleta_uja_poo.aym") },
  { name: "Tetris", kind: "Juego de consola", ...sample("samples/games/console/tetris.aym") },
  { name: "Sudoku anatawi", kind: "Juego de consola", ...sample("samples/games/console/sudoku_anatawi.aym") },
  { name: "Katari", kind: "Juego de consola", ...sample("samples/games/console/katari.aym") },
  { name: "Jamachi", kind: "Juego de consola", ...sample("samples/games/console/jamachi.aym") },
  { name: "Inventario", kind: "Mapas y archivos", ...sample("samples/apps/inventario_archivo.aym") },
];

/** Vocabulario, con el uso que le da docs/grammar.md. `hilo` es la clase de palabra. */
export const VOCABULARY: { group: string; words: { word: string; use: string; hilo: "k" | "t" | "n" }[] }[] = [
  {
    group: "Programa",
    words: [
      { word: "qallta", use: "inicio del programa", hilo: "k" },
      { word: "tukuya", use: "fin del programa", hilo: "k" },
      { word: "qillqa", use: "escribir en pantalla", hilo: "k" },
      { word: "katu", use: "leer de la consola", hilo: "k" },
    ],
  },
  {
    group: "Datos",
    words: [
      { word: "yatiya", use: "declarar una variable", hilo: "k" },
      { word: "jakhüwi", use: "número", hilo: "t" },
      { word: "aru", use: "texto", hilo: "t" },
      { word: "chiqa", use: "booleano; verdadero", hilo: "t" },
      { word: "k'ari", use: "falso", hilo: "n" },
      { word: "t'aqa", use: "lista", hilo: "t" },
      { word: "mapa", use: "mapa de claves y valores", hilo: "t" },
    ],
  },
  {
    group: "Decidir",
    words: [
      { word: "ukaxa", use: "si", hilo: "k" },
      { word: "maysatxa", use: "si no", hilo: "k" },
      { word: "khiti", use: "selección por patrón", hilo: "k" },
      { word: "kuna", use: "caso de la selección", hilo: "k" },
      { word: "yaqha", use: "caso por defecto", hilo: "k" },
      { word: "siqicha", use: "enumeración", hilo: "k" },
    ],
  },
  {
    group: "Repetir",
    words: [
      { word: "ukhakamaxa", use: "mientras", hilo: "k" },
      { word: "kuti", use: "para", hilo: "k" },
      { word: "p'akhiña", use: "salir del bucle", hilo: "k" },
      { word: "sarantaña", use: "continuar el bucle", hilo: "k" },
    ],
  },
  {
    group: "Funciones y módulos",
    words: [
      { word: "lurawi", use: "definir una función", hilo: "k" },
      { word: "kuttaya", use: "devolver un valor", hilo: "k" },
      { word: "apnaq", use: "importar un módulo", hilo: "k" },
    ],
  },
  {
    group: "Errores",
    words: [
      { word: "yant'aña", use: "intentar", hilo: "k" },
      { word: "katjaña", use: "capturar", hilo: "k" },
      { word: "tukuyawi", use: "al terminar, siempre", hilo: "k" },
      { word: "pantja", use: "lanzar un error", hilo: "k" },
    ],
  },
  {
    group: "Clases",
    words: [
      { word: "kasta", use: "definir una clase", hilo: "k" },
      { word: "machaqa", use: "crear una instancia", hilo: "k" },
      { word: "aka", use: "el objeto actual", hilo: "k" },
      { word: "jila", use: "heredar de", hilo: "k" },
      { word: "jilaaka", use: "la clase padre", hilo: "k" },
      { word: "sapa", use: "miembro privado", hilo: "k" },
      { word: "sapakasta", use: "miembro estático", hilo: "k" },
    ],
  },
];

/** Etapas que documenta docs/compiler.md, con el archivo que deja cada una. */
export const PIPELINE = [
  { step: "Escribes", file: "hola.aym", note: "Texto plano, en cualquier editor." },
  { step: "aymc valida", file: "--check", note: "Sintaxis y semántica, con diagnósticos." },
  { step: "Genera ASM", file: "hola.asm", note: "Lo conservas con --emit-asm." },
  { step: "Ensambla y enlaza", file: "hola.o", note: "Con NASM y gcc." },
  { step: "Ejecutas", file: "hola", note: "Un binario nativo, sin intérprete." },
];

/** Comando y, si hace falta, qué hace. `null` separa los dos flujos. */
export const COMMANDS: ([string, string?] | null)[] = [
  ["aymc hola.aym", "compila"],
  ["aymc --check hola.aym", "solo valida"],
  ["aymc --emit-asm -o build/hola hola.aym", "conserva el ensamblador"],
  null,
  ["aym new demo", "o por proyecto"],
  ["cd demo"],
  ["aym build"],
  ["aym run"],
];

type Asset = { name: string; size: number; url: string };
const assets = release.assets as Partial<Record<"win-exe" | "win-msi" | "linux-deb", Asset>>;

export const mb = (bytes: number) =>
  bytes >= 10 * 1024 * 1024 ? `${Math.round(bytes / 1024 / 1024)} MB` : `${(bytes / 1024 / 1024).toFixed(1).replace(".", ",")} MB`;

export const RELEASE = {
  tag: release.tag,
  publishedAt: release.publishedAt,
  version: release.version,
  page: `${REPO_URL}/releases/tag/${release.tag}`,
  winExe: assets["win-exe"],
  winMsi: assets["win-msi"],
  linuxDeb: assets["linux-deb"],
};
