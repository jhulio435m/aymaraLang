## AymaraLang VS Code Extension

Extensión oficial para trabajar con archivos `.aym` en Visual Studio Code.
Incluye resaltado de sintaxis y snippets básicos.

### Características

- Resaltado de palabras clave (`qallta`, `jisa`, `maysatxa`, `lurawi`, `kuttaya`, etc.).
- Tipos (`jakhüwi`, `aru`, `chiqa`, `t'aqa`, `mapa`).
- Comentarios `//` y `/* */`.
- Snippets para estructuras comunes (`jisa`, `taki`, `ukhakamaxa`, `lurawi`).

### Uso

1. Abre un archivo con extensión `.aym`.
2. VS Code activará automáticamente la extensión.

### Desarrollo local

```bash
cd aymlang
npm install
npx @vscode/vsce package
```

Instala el `.vsix` desde VS Code (**Extensions → Install from VSIX**).
