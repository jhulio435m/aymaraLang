// Tokenizador mínimo de AymaraLang para resaltar código y tejerlo.
// Las palabras salen de docs/grammar.md; no valida, solo clasifica.

/** k palabra clave · t tipo · s texto · n número o valor · i nombre · c comentario · p signo · w espacio */
export type Kind = "k" | "t" | "s" | "n" | "i" | "c" | "p" | "w";
export interface Tok {
  kind: Kind;
  text: string;
}

const KEYWORDS = new Set([
  "qallta", "tukuya", "yatiya", "qillqa", "ukaxa", "maysatxa", "ukhakamaxa", "kuti", "p'akhiña",
  "sarantaña", "lurawi", "kuttaya", "apnaq", "siqicha", "khiti", "kuna", "yaqha", "yant'aña",
  "katjaña", "tukuyawi", "pantja", "kasta", "machaqa", "aka", "jila", "sapa", "sapakasta", "jilaaka",
  "katu",
]);
const TYPES = new Set(["jakhüwi", "aru", "chiqa", "t'aqa", "mapa"]);
const VALUES = new Set(["chiqa", "k'ari"]);
// Palabras con apóstrofo interno: se reconocen enteras antes que las comillas.
const QUOTED = ["p'akhiña", "yant'aña", "t'aqa", "k'ari", "ch'ullu", "ch'usa"];

const WORD = /^[\p{L}_][\p{L}\p{N}_]*/u;
const NUMBER = /^(0x[0-9a-fA-F]+|0b[01]+|\d+(\.\d+)?)/;

export function tokenize(source: string): Tok[] {
  const out: Tok[] = [];
  let i = 0;
  while (i < source.length) {
    const rest = source.slice(i);
    const ch = rest[0];
    let m: RegExpMatchArray | null;

    if ((m = rest.match(/^\s+/))) {
      out.push({ kind: "w", text: m[0] });
    } else if (rest.startsWith("//")) {
      const end = rest.indexOf("\n");
      out.push({ kind: "c", text: end < 0 ? rest : rest.slice(0, end) });
    } else if (rest.startsWith("/*")) {
      const end = rest.indexOf("*/", 2);
      out.push({ kind: "c", text: end < 0 ? rest : rest.slice(0, end + 2) });
    } else if (QUOTED.some((q) => rest.startsWith(q) && !WORD.test(rest.slice(q.length)))) {
      const q = QUOTED.find((w) => rest.startsWith(w))!;
      out.push({ kind: q === "k'ari" ? "n" : q === "t'aqa" ? "t" : KEYWORDS.has(q) ? "k" : "i", text: q });
    } else if (ch === '"' || ch === "'") {
      let j = 1;
      while (j < rest.length && rest[j] !== ch && rest[j] !== "\n") j += rest[j] === "\\" ? 2 : 1;
      out.push({ kind: "s", text: rest.slice(0, Math.min(j + 1, rest.length)) });
    } else if ((m = rest.match(NUMBER))) {
      out.push({ kind: "n", text: m[0] });
    } else if ((m = rest.match(WORD))) {
      const w = m[0];
      out.push({ kind: KEYWORDS.has(w) ? "k" : TYPES.has(w) ? "t" : "i", text: w });
    } else {
      out.push({ kind: "p", text: ch });
    }
    i += out[out.length - 1].text.length;
  }

  // `chiqa` es tipo delante de un nombre (`yatiya chiqa fin`) y valor en el resto.
  for (let k = 0; k < out.length; k++) {
    if (out[k].text !== "chiqa") continue;
    const next = out.slice(k + 1).find((t) => t.kind !== "w");
    if (!(next && next.kind === "i") && VALUES.has(out[k].text)) out[k].kind = "n";
  }
  return out;
}

/** Los mismos tokens, partidos por línea de código. */
export function tokenLines(source: string): Tok[][] {
  const lines: Tok[][] = [[]];
  for (const tok of tokenize(source)) {
    const parts = tok.text.split("\n");
    parts.forEach((part, idx) => {
      if (idx > 0) lines.push([]);
      if (part) lines[lines.length - 1].push({ kind: tok.kind, text: part });
    });
  }
  return lines;
}

/** Color de hilo por clase de palabra (variables CSS definidas en globals.css). */
export const THREAD: Record<Exclude<Kind, "w">, string> = {
  k: "var(--hilo-clave)",
  t: "var(--hilo-tipo)",
  s: "var(--hilo-texto)",
  n: "var(--hilo-numero)",
  i: "var(--hilo-nombre)",
  c: "var(--hilo-nota)",
  p: "var(--hilo-signo)",
};
