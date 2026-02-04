## AymaraLang VS Code Extension

Extensión oficial para trabajar con archivos `.aym` en Visual Studio Code.
Incluye resaltado de sintaxis, configuración del lenguaje y snippets útiles
para escribir AymaraLang más rápido.

### Características

- Resaltado de palabras clave (`qallta`, `suti`, `lurawi`, `kuttaya`, etc.).
- Tipos (`jakhüwi`, `aru`, `chiqa`, `listaña`, `mapa`).
- Funciones integradas (`qillqa`, `input`, `array_*`, `sin`, `cos`, ...).
- Soporte de comentarios `//` y `/* */`.
- Snippets para `suti/jani`, `sapüru`, `kunawsati`, `lurawi`.

### Uso

1. Abre un archivo con extensión `.aym`.
2. VS Code activará automáticamente la extensión.
3. Escribe código y utiliza los snippets con los prefijos:
   - `suti`, `kunawsati`, `sapüru`, `lurawi`, `kuttaya`.

### Desarrollo local

Si quieres probar la extensión localmente:

```bash
cd aymlang
npm install
npx @vscode/vsce package
```

Luego instala el `.vsix` generado desde VS Code usando **Extensions → Install from VSIX**.
