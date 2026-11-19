#!/usr/bin/env node
// Writes src/content/release.json from a published GitHub release so every
// download the site mentions points at a file that exists, with its real size.
//
//   node scripts/sync-release.mjs                 # latest release
//   node scripts/sync-release.mjs --tag v0.2.0    # a specific tag
//   node scripts/sync-release.mjs --check         # also fail if an expected file is missing
//   node scripts/sync-release.mjs --soft          # keep the committed file when offline / no release yet
//
// Env: GITHUB_REPOSITORY (owner/repo, default jhulio435m/aymaraLang), GITHUB_TOKEN (optional).
import { readFile, writeFile } from "node:fs/promises";
import { fileURLToPath } from "node:url";

const args = process.argv.slice(2);
const flag = (n) => args.includes(n);
const tagArg = args.includes("--tag") ? args[args.indexOf("--tag") + 1] : null;
const repo = process.env.GITHUB_REPOSITORY || "jhulio435m/aymaraLang";
const out = fileURLToPath(new URL("../src/content/release.json", import.meta.url));

/** Los archivos que ofrece la web. El .deb lleva su propia versión en el nombre, así que se busca por patrón. */
export const EXPECTED = [
  { id: "win-exe", label: "AymaraLang-Setup.exe", match: /^AymaraLang-Setup\.exe$/ },
  { id: "win-msi", label: "AymaraLang-Setup.msi", match: /^AymaraLang-Setup\.msi$/ },
  { id: "linux-deb", label: "aymaralang_*_amd64.deb", match: /^aymaralang_.+_amd64\.deb$/ },
];

async function main() {
  const url = `https://api.github.com/repos/${repo}/releases/${tagArg ? `tags/${tagArg}` : "latest"}`;
  const headers = { Accept: "application/vnd.github+json", "User-Agent": "aymaralang-web-release-sync" };
  if (process.env.GITHUB_TOKEN) headers.Authorization = `Bearer ${process.env.GITHUB_TOKEN}`;

  let release;
  try {
    const res = await fetch(url, { headers });
    if (!res.ok) throw new Error(`${res.status} ${res.statusText} en ${url}`);
    release = await res.json();
  } catch (err) {
    if (flag("--soft")) {
      const kept = JSON.parse(await readFile(out, "utf8"));
      console.warn(`sync-release: ${err.message}; se mantiene ${kept.tag} de release.json`);
      return;
    }
    throw err;
  }

  const tag = release.tag_name;
  const version = tag.replace(/^v/, "");
  const files = release.assets.map((a) => ({ name: a.name, size: a.size, url: a.browser_download_url }));
  const assets = {};
  for (const e of EXPECTED) {
    const hit = files.find((f) => e.match.test(f.name));
    if (hit) assets[e.id] = hit;
  }
  const missing = EXPECTED.filter((e) => !assets[e.id]).map((e) => e.label);

  if (missing.length && flag("--check")) {
    console.error(`sync-release: faltan en ${tag}:\n  ${missing.join("\n  ")}`);
    process.exit(1);
  }
  if (flag("--soft") && missing.length === EXPECTED.length) {
    const kept = JSON.parse(await readFile(out, "utf8"));
    console.warn(`sync-release: ${tag} aún no ofrece paquetes; se mantiene ${kept.tag}`);
    return;
  }
  if (missing.length) console.warn(`sync-release: aviso, faltan en ${tag}: ${missing.join(", ")}`);

  const data = { repo, tag, version, publishedAt: release.published_at, assets };
  await writeFile(out, `${JSON.stringify(data, null, 2)}\n`);
  console.log(`sync-release: ${tag} con ${Object.keys(assets).length} archivos → src/content/release.json`);
}

main().catch((err) => {
  console.error(`sync-release: ${err.message}`);
  process.exit(1);
});
