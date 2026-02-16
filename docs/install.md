# Instalación por sistema operativo

Esta guía cubre instalación de binarios publicados en Releases.

## Windows

### Dependencias requeridas

`aymc` requiere toolchain en `PATH`:

- `nasm`
- `gcc` (MSYS2/MinGW)

Instalación/validación automática de dependencias:

```powershell
pwsh -File .\scripts\install\install_deps_windows.ps1
```

Solo validar (sin instalar):

```powershell
pwsh -File .\scripts\install\install_deps_windows.ps1 -CheckOnly
```

### Instalación de AymaraLang

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
```

## Linux

### Dependencias requeridas

- `nasm`
- `gcc`
- `g++`

Instalación/validación automática de dependencias:

```bash
bash scripts/install/install_deps_linux.sh
```

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

### Verificación rápida

```bash
aymc --help
aym --help
```

## Notas operativas

- Los instaladores registran asociación para archivos `.aym`.
- En Windows, el icono de `.aym` se instala como `logo.ico` en el directorio de
  instalación.
- Tras actualizar instaladores, el refresco de iconos puede requerir reiniciar
  sesión o reiniciar `explorer.exe`.
