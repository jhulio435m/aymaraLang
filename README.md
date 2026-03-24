# AymaraLang (`aym`)

**AymaraLang** es un lenguaje de programación moderno basado en la lengua originaria aymara. Su compilador, `aymc`, ha sido desarrollado desde cero en **C++17**, y permite generar ejecutables nativos. El proyecto busca promover inclusión tecnológica, educación y preservación lingüística con una sintaxis canónica ya consolidada.

Palabras clave principales del lenguaje:

- `jakhüwi`, `aru`, `chiqa`, `t'aqa`, `mapa` – tipos base
- `yatiya` – declaración de variables
- `qallta` / `tukuya` – inicio y fin del programa
- `qillqa` – salida por pantalla
- `katu` / `input` – lectura de consola
- `lurawi` / `kuttaya` – definición de funciones y retorno
- `ukaxa` / `maysatxa`, `ukhakamaxa`, `kuti` – control de flujo
- `siqicha`, `khiti`, `kuna`, `yaqha` – enumeraciones y selección
- `apnaq` – importación de módulos desde otros archivos

## Documentación

- [Inicio](docs/index.md)
- [Manual de usuario](docs/manual_usuario.md)
- [Visión general](docs/overview.md)
- [Instalación por sistema operativo](docs/install.md)
- [Compilación e instalación desde fuente](docs/build.md)
- [CLI del compilador](docs/compiler.md)
- [Arquitectura del compilador](docs/arquitectura.md)
- [Estado de modernización](docs/modernizacion.md)
- [Primeros pasos](docs/language.md)
- [Referencia rápida](docs/aymaraLang.md)
- [Gramática formal](docs/grammar.md)
- [Investigación y teoría](docs/research/investigacion.md)

## Instalación desde releases

### Windows (MSI o EXE NSIS)

Los instaladores Windows ya incluyen `nasm` y `gcc` embebidos. No hace falta instalar dependencias adicionales para usar `aymc` desde un release.

Formas de instalación:

```powershell
# MSI (recomendado para despliegue administrado)
msiexec /i .\AymaraLang-Setup.msi /passive

# NSIS (interactivo)
.\AymaraLang-Setup.exe

# NSIS silencioso
.\AymaraLang-Setup.exe /S
```

### Linux (.deb)

Dependencias requeridas para usar `aymc`:

- `nasm`
- `gcc`
- `g++`
- soporte X11 si vas a ejecutar GUI

Instalación automática de dependencias:

```bash
bash scripts/install/install_deps_linux.sh
```

Instalación del paquete:

```bash
sudo dpkg -i aymaralang_<version>_amd64.deb
sudo apt-get install -f -y
```

Si publicas el bundle genérico Linux:

```bash
tar -xzf AymaraLang-0.1.0-Linux.tar.gz
cd AymaraLang-0.1.0-Linux/bin
./aymc --help
```

## Inicio rápido (desde fuente)

Compila y ejecuta un ejemplo sencillo:

```bash
cmake -S . -B build-linux -DCMAKE_BUILD_TYPE=Release
cmake --build build-linux -j
./build-linux/bin/aymc samples/aymara_flow.aym
./samples/aymara_flow
```

Wrapper de proyecto (`aym`) disponible en el mismo directorio de build:

```bash
./build/bin/aym --help
```

Para pasos detallados por sistema operativo, revisa la guía de compilación en
[`docs/build.md`](docs/build.md).

Para levantar el sitio de documentación con MkDocs:

```bash
pip install mkdocs
mkdocs serve
```
