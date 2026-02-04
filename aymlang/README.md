## AymaraLang VS Code Extension

Extensión oficial para trabajar con archivos `.aym` en Visual Studio Code.
Incluye resaltado de sintaxis, snippets básicos y autocompletado basado en el lexer/parser del compilador.

### Características

- Resaltado de palabras clave (`qallta`, `jisa`, `maysatxa`, `lurawi`, `kuttaya`, etc.).
- Tipos (`jakhüwi`, `aru`, `t'aqa`, `mapa`) y literales (`utji`, `chiqa`, `janiutji`, `k'ari`).
- Comentarios `//` y `/* */`.
- Snippets para estructuras comunes (`jisa`, `taki`, `ukhakamaxa`, `lurawi`).
- Sugerencias de IntelliCode para palabras clave, tipos, literales y plantillas de código.

### Uso

1. Abre un archivo con extensión `.aym`.
2. VS Code activará automáticamente la extensión.
3. Usa `Ctrl+Space` para ver las sugerencias de autocompletado.

### Desarrollo local

```bash
cd aymlang
npm install
npx @vscode/vsce package
```

Instala el `.vsix` desde VS Code (**Extensions → Install from VSIX**).
