# Extensión VS Code para AymaraLang

Incluye resaltado de sintaxis básico, comentarios con `#` y configuración de
autocierre de pares para archivos `.aym`.

## Empaquetado

1. Instala `vsce` si aún no está instalado:

   ```bash
   npm install -g @vscode/vsce
   ```

2. Desde esta carpeta:

   ```bash
   vsce package
   ```

Se generará un archivo `.vsix` que puedes instalar en VS Code desde
**Extensions → Install from VSIX**.
