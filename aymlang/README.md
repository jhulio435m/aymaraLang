## AymaraLang VS Code Extension

Extensión oficial para trabajar con archivos `.aym` en Visual Studio Code.
Incluye resaltado de sintaxis, configuración del lenguaje y snippets útiles
para escribir AymaraLang más rápido.

### Características

- Resaltado de palabras clave (`qallta`, `jisa`, `maysatxa`, `lurawi`, `kuttaya`, etc.).
- Tipos (`jakhüwi`, `aru`, `chiqa`, `t'aqa`, `mapa`).
- Funciones integradas (`qillqa`, `katu`, `input`, `array_*`, `push`, `largo`, `sin`, `cos`, ...).
- Soporte de comentarios `//` y `/* */`.
- Snippets para `jisa/maysatxa`, `taki`, `ukhakamaxa`, `lurawi`.

### Uso

1. Abre un archivo con extensión `.aym`.
2. VS Code activará automáticamente la extensión.
3. Escribe código y utiliza los snippets con los prefijos:
   - `jisa`, `ukhakamaxa`, `taki`, `lurawi`, `kuttaya`, `katu`.

### Desarrollo local

Si quieres probar la extensión localmente:

```bash
cd aymlang
npm install
npx @vscode/vsce package
```

Luego instala el `.vsix` generado desde VS Code usando **Extensions → Install from VSIX**.
