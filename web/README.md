# Web de AymaraLang

Página de producto de [aymaralang.soryntel.com](https://aymaralang.soryntel.com). Next.js (App Router) con Tailwind CSS 4, exportada como sitio estático. No aloja la documentación: enlaza al MkDocs de GitHub Pages.

```sh
npm install
npm run dev     # http://localhost:3000
npm run build   # genera out/
```

- Contenido (ejemplos, vocabulario, descargas): `src/content/site.ts`
- Página: `src/app/page.tsx`; estilos y tokens en `src/app/globals.css`
- Tokenizador de `.aym`: `src/lib/aym.ts`. Da color al código y a los tejidos (`Franja` en `src/components/weave.tsx`, `Salta` en `src/components/salta.tsx`)
- Diseño: `../DESIGN.md` y `../PRODUCT.md`

## Datos generados

- `npm run sync-samples` copia a `src/content/samples.json` los programas de `../samples` que muestra la web. Ejecútalo cuando cambien esos archivos y haz commit del resultado.
- `npm run sync-release` escribe `src/content/release.json` desde la última versión de GitHub Releases (nombre, tamaño y URL de cada instalador). Con `--check` falla si falta alguno; con `--soft` conserva el archivo si no hay red.

## Despliegue en Cloudflare Workers

`wrangler.jsonc` publica `out/` como archivos estáticos en el Worker `aymaralang`, con el dominio `aymaralang.soryntel.com`.

```sh
npm run deploy   # sync-release + build + wrangler deploy
```

Para desplegar en cada push, conecta el Worker al repositorio en el panel (Settings → Builds) con root directory `web`, build command `node scripts/sync-release.mjs --soft && npm run build`, deploy command `npx wrangler deploy` y la variable `GITHUB_REPOSITORY=jhulio435m/aymaraLang`. La versión de Node sale de `.node-version`.

Las cabeceras de caché y seguridad están en `public/_headers`. Una versión nueva de AymaraLang no es un commit en `web/`: vuelve a ejecutar `npm run deploy` para que la web tome los nuevos enlaces y tamaños.
