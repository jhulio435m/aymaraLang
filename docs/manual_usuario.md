# Manual de usuario

**Producto:** AymaraLang  
**Versión:** 0.1.0  
**Documento:** MU-AYM-001

## 1. Objetivo

Este manual describe el uso operativo de AymaraLang para usuarios finales:

- instalación,
- verificación,
- compilación de programas `.aym`,
- flujo de trabajo por proyecto con `aym`,
- resolución de problemas frecuentes.

## 2. Requisitos previos

### 2.1 Windows

Dependencias requeridas para compilar con `aymc`:

- `nasm` en `PATH`
- `gcc` en `PATH` (MSYS2/MinGW)

Instalación automática recomendada:

```powershell
pwsh -File .\scripts\install\install_deps_windows.ps1
```

### 2.2 Linux

Dependencias requeridas:

- `nasm`
- `gcc`
- `g++`

Instalación automática recomendada:

```bash
bash scripts/install/install_deps_linux.sh
```

## 3. Instalación de AymaraLang

### 3.1 Windows

```powershell
# MSI (recomendado)
msiexec /i .\AymaraLang-Setup.msi /passive

# NSIS interactivo
.\AymaraLang-Setup.exe

# NSIS silencioso
.\AymaraLang-Setup.exe /S
```

### 3.2 Linux (.deb)

```bash
sudo dpkg -i aymaralang_<version>_amd64.deb
sudo apt-get install -f -y
```

## 4. Verificación inicial

Después de instalar:

```bash
aymc --help
aym --help
```

Si ambos comandos muestran ayuda, la instalación quedó operativa.

## 5. Uso básico con `aymc` (archivo suelto)

### 5.1 Compilar un archivo

```bash
aymc samples/fundamentos/basicos.aym
```

### 5.2 Definir salida

```bash
aymc -o build/app samples/fundamentos/basicos.aym
```

En Windows la salida final será `build/app.exe`.

### 5.3 Validar sin generar binario

```bash
aymc --check samples/fundamentos/basicos.aym
```

### 5.4 Exportar diagnósticos JSON

```bash
aymc --check --diagnostics-json samples/fundamentos/basicos.aym
```

## 6. Uso por proyecto con `aym`

### 6.1 Crear proyecto

```bash
aym new demo
```

### 6.2 Compilar y ejecutar

```bash
cd demo
aym build
aym run
```

### 6.3 Probar

```bash
aym test
```

### 6.4 Dependencias

```bash
aym add math ^1.2.0
aym lock check
aym lock sync
```

## 7. Archivos relevantes del usuario

- `aym.toml`: manifest del proyecto.
- `aym.lock`: lockfile reproducible.
- `.aym/cache` y `.aym/repo`: caché/repositorio local de dependencias.

## 8. Resolución de problemas frecuentes

| Síntoma | Causa común | Acción recomendada |
| --- | --- | --- |
| `nasm` no encontrado | Dependencia ausente en `PATH` | Ejecutar script de dependencias y abrir nueva terminal |
| `gcc` no encontrado | Toolchain MinGW/MSYS2 no disponible | Instalar dependencias y validar `gcc --version` |
| MSI error de privilegios (1925/1603) | Instalación sin elevación suficiente | Usar terminal elevada o `msiexec /i ... /passive` |
| Inconsistencia `manifest-lock` | `aym.toml` y `aym.lock` desalineados | Ejecutar `aym lock sync` o recompilar con lock actualizado |
| Ícono `.aym` no se refresca en Windows | Caché visual del shell | Reiniciar `explorer.exe` o cerrar sesión |

## 9. Desinstalación

### 9.1 Windows

```powershell
# MSI
msiexec /x .\AymaraLang-Setup.msi /passive

# NSIS
"C:\Program Files\AymaraLang\Uninstall.exe" /S
```

### 9.2 Linux

```bash
sudo dpkg --purge aymaralang
```

## 10. Buenas prácticas de uso

- Mantener `aym.toml` y `aym.lock` sincronizados.
- Ejecutar `aym test` antes de publicar cambios.
- Usar `--check` para validaciones rápidas en CI o pre-commit.
- Conservar scripts de build/packaging del repositorio como única fuente de verdad.
