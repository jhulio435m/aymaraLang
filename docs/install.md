# Instalación por sistema operativo

Esta guía cubre instalación de binarios publicados en Releases.

## Windows

### Instalación de AymaraLang

Los instaladores Windows publicados (`AymaraLang-Setup.msi` y
`AymaraLang-Setup.exe`) ya incluyen la toolchain necesaria para compilar:

- `nasm`
- `gcc` (MinGW embebido)

Desde artefactos de release:

```powershell
# MSI (recomendado)
msiexec /i .\AymaraLang-Setup.msi /passive

# MSI silencioso total (requiere consola elevada)
msiexec /i .\AymaraLang-Setup.msi /qn

# NSIS interactivo
.\AymaraLang-Setup.exe

# NSIS silencioso
.\AymaraLang-Setup.exe /S
```

### Verificación rápida

```powershell
aymc --help
aym --help
aymc .\samples\fundamentos\basicos.aym
```

### Construcción desde código fuente

Para compilar AymaraLang desde el repo sí se necesita toolchain de build en el
sistema. Instalación/validación automática:

```powershell
pwsh -File .\scripts\install\install_deps_windows.ps1
```

Solo validar:

```powershell
pwsh -File .\scripts\install\install_deps_windows.ps1 -CheckOnly
```

## Linux

### Dependencias requeridas

- `nasm`
- `gcc`
- `g++`
- headers/librería de X11 para soporte GUI (`pkg-config --exists x11`)

Instalación/validación automática de dependencias:

```bash
bash scripts/install/install_deps_linux.sh
```

El script instala también el soporte X11 necesario para que `uja_qallta`,
`uja_suyu`, `uja_qillqa` y los juegos GUI abran ventana en Linux.

Solo validar (sin instalar):

```bash
bash scripts/install/install_deps_linux.sh --check-only
```

### Instalación de paquete `.deb`

```bash
sudo dpkg -i aymaralang_<version>_amd64.deb
sudo apt-get install -f -y
```

Desinstalación:

```bash
sudo dpkg --purge aymaralang
```

### Uso desde bundle `.tar.gz`

```bash
tar -xzf AymaraLang-0.1.0-Linux.tar.gz
cd AymaraLang-0.1.0-Linux/bin
./aymc --help
./aym --help
```

### Verificación rápida

```bash
aymc --help
aym --help
aymc ./samples/fundamentos/basicos.aym
```

## Notas operativas

- Los instaladores registran asociación para archivos `.aym`.
- En Windows, el icono de `.aym` se instala como `logo.ico` en el directorio de
  instalación.
- Tras actualizar instaladores, el refresco de iconos puede requerir reiniciar
  sesión o reiniciar `explorer.exe`.
