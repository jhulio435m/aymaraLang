## AymaraLang VS Code Extension

Extensión oficial para trabajar con archivos `.aym` en Visual Studio Code.
Incluye resaltado de sintaxis, configuración del lenguaje y snippets útiles
para escribir AymaraLang más rápido.

### Características

- Resaltado de palabras clave (`si`, `mientras`, `luräwi`, `kutiyana`, etc.).
- Tipos (`jach’a`, `lliphiphi`, `chuymani`, `qillqa`).
- Funciones integradas (`willt’aña`, `input`, `array_*`, `sin`, `cos`, ...).
- Soporte de comentarios `//` y `/* */`.
- Snippets para `si/sino`, `para`, `mientras`, `luräwi`, `tantachaña`.

### Uso

1. Abre un archivo con extensión `.aym`.
2. VS Code activará automáticamente la extensión.
3. Escribe código y utiliza los snippets con los prefijos:
   - `si`, `mientras`, `para`, `range`, `lurawi`, `kutiyana`, `tantachana`.

### Desarrollo local

Si quieres probar la extensión localmente:

```bash
cd aymlang
npm install
npx @vscode/vsce package
```

Luego instala el `.vsix` generado desde VS Code usando **Extensions → Install from VSIX**.
