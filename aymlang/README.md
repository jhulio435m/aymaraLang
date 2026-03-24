## AymaraLang Editor Extension

Extensión oficial para trabajar con archivos `.aym` en editores compatibles con el ecosistema de extensiones de VS Code.
Incluye resaltado de sintaxis, snippets básicos y autocompletado alineado con la variante canónica actual del compilador.

### Cobertura de editores y marketplaces

- VS Code Marketplace: soportado mediante el paquete `.vsix` de esta carpeta.
- Open VSX: soportado con el mismo paquete fuente y flujo de publicación.
- Cursor y otros forks compatibles con VS Code: soportados instalando el mismo `.vsix` o consumiendo un marketplace compatible.
- Visual Studio IDE (`visualstudio.microsoft.com`, marketplace de Visual Studio): no está soportado por esta extensión. Requiere otro proyecto distinto basado en VSSDK y un manifiesto propio de Visual Studio.

### Características

- Resaltado de palabras clave (`qallta`, `ukaxa`, `maysatxa`, `ukhakamaxa`, `kuti`, `siqicha`, `khiti`, `kuna`, `yaqha`, `lurawi`, `kuttaya`, etc.).
- Tipos (`jakhüwi`, `aru`, `t'aqa`, `mapa`) y literales (`chiqa`, `k'ari`).
- Comentarios `//` y `/* */`.
- Snippets para estructuras comunes (`ukaxa`, `kuti`, `ukhakamaxa`, `lurawi`, `kasta`).
- Sugerencias de IntelliCode para palabras clave, tipos, literales y plantillas de código.

### Sintaxis canónica soportada

- Control de flujo: `ukaxa`, `maysatxa`, `ukhakamaxa`, `kuti`
- Selección: `siqicha`, `khiti`, `kuna`, `yaqha`
- Tipos y literales: `jakhüwi`, `aru`, `t'aqa`, `mapa`, `chiqa`, `k'ari`
- POO: `kasta`, `machaqa`, `aka`, `jila`, `sapa`, `sapakasta`, `jilaaka`

La extensión ya no sugiere aliases retirados ni keywords antiguas de transición.

### Uso

1. Abre un archivo con extensión `.aym`.
2. VS Code activará automáticamente la extensión.
3. Usa `Ctrl+Space` para ver las sugerencias de autocompletado.

### Desarrollo local

```bash
cd aymlang
npm install
npm run package
```

Instala el `.vsix` desde VS Code o cualquier editor compatible (**Extensions → Install from VSIX**).

### Publicación

```bash
pwsh -File .\scripts\publish_extension.ps1 -Target package
pwsh -File .\scripts\publish_extension.ps1 -Target vscode
pwsh -File .\scripts\publish_extension.ps1 -Target openvsx
pwsh -File .\scripts\publish_extension.ps1 -Target all
```

Variables esperadas por el script:

- `VSCE_PAT`: token de Visual Studio Marketplace.
- `OVSX_PAT`: token de Open VSX.

También puedes pasar `-VsceToken` y `-OvsxToken` explícitamente, pero es mejor
usar variables de entorno.
