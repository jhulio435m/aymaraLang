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

## Despliegue en Cloudflare Pages

Proyecto de Pages conectado a `jhulio435m/aymaraLang`, rama `main`:

| Ajuste | Valor |
| --- | --- |
| Directorio raíz | `web` |
| Comando de build | `node scripts/sync-release.mjs --soft && npm run build` |
| Directorio de salida | `out` |
| Variables | `NODE_VERSION=26`, `GITHUB_REPOSITORY=jhulio435m/aymaraLang` |

Dominio personalizado `aymaralang.soryntel.com` (en la zona `soryntel.com`: `CNAME aymaralang → <proyecto>.pages.dev`, con proxy). Las cabeceras de caché y seguridad están en `public/_headers`.

Una versión nueva de AymaraLang no es un commit en `web/`, así que no dispara un build: vuelve a desplegar desde el panel o llama a un Deploy Hook desde el flujo de releases.
