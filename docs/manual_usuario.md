# Manual de usuario

**Producto:** AymaraLang  
**Versión:** 0.1.0  
**Documento:** MU-AYM-001

## 1. Objetivo

Este manual describe el uso práctico de AymaraLang para usuarios finales:

- instalación,
- verificación,
- compilación de programas `.aym`,
- uso del gestor `aym`,
- sintaxis esencial del lenguaje,
- resolución de problemas frecuentes.

## 2. Requisitos previos

### 2.1 Windows

Si instalas AymaraLang desde los artefactos publicados (`AymaraLang-Setup.msi`
o `AymaraLang-Setup.exe`), no necesitas instalar `nasm` ni `gcc` por separado.
El instalador ya incluye una toolchain privada lista para usar.

Solo necesitas dependencias externas si vas a compilar AymaraLang desde este
repositorio.

### 2.2 Linux

Para usar `aymc` en Linux sí se requieren dependencias del sistema:

- `nasm`
- `gcc`
- `g++`
- soporte X11 para GUI (`pkg-config --exists x11`)

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

### 3.3 Linux (`.tar.gz`)

```bash
tar -xzf AymaraLang-0.1.0-Linux.tar.gz
cd AymaraLang-0.1.0-Linux/bin
./aymc --help
```

## 4. Verificación inicial

Después de instalar:

```bash
aymc --help
aym --help
```

Prueba mínima de compilación:

```bash
aymc samples/fundamentos/basicos.aym
```

Si ambos comandos muestran ayuda y el sample compila, la instalación quedó
operativa.

## 5. Uso básico con `aymc`

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

### 5.5 Elegir plataforma objetivo

```bash
# Fuerza salida Linux
aymc --linux samples/fundamentos/basicos.aym

# Fuerza salida Windows
aymc --windows samples/fundamentos/basicos.aym
```

## 6. Sintaxis esencial

### 6.1 Programa mínimo

```aym
qallta
qillqa("Kamisaraki")
tukuya
```

### 6.2 Variables y tipos

```aym
yatiya jakhüwi edad = 21;
yatiya aru suti = "Ana";
yatiya chiqa wakisiri = chiqa;
```

### 6.3 Condicionales y bucles

```aym
ukaxa (edad >= 18) {
  qillqa("jach'a");
} maysatxa {
  qillqa("jisk'a");
}

kuti (yatiya jakhüwi i = 0; i < 3; i = i + 1) {
  qillqa(i);
}
```

### 6.4 Funciones

```aym
lurawi suma(jakhüwi a, jakhüwi b): jakhüwi {
  kuttaya a + b;
}
```

### 6.5 Enumeraciones y selección

```aym
siqicha Estado { Qalltata, Tukuyata }

khiti(1) {
  kuna 0:
    qillqa("ch'usa");
  kuna 1, 2:
    qillqa("manta");
  yaqha:
    qillqa("jani uñt'ata");
}
```

### 6.6 Clases

```aym
kasta Animal {
  lurawi aru(): aru {
    kuttaya "base";
  }
}

kasta Perro jila Animal {
  lurawi aru(): aru {
    kuttaya jilaaka.aru() + "+perro";
  }
}
```

Si una subclase redefine un método heredado, la sobrescritura es implícita. La
firma y el tipo de retorno deben coincidir con la clase base.

## 7. Uso por proyecto con `aym`

### 7.1 Crear proyecto

```bash
aym new demo
```

### 7.2 Compilar y ejecutar

```bash
cd demo
aym build
aym run
```

### 7.3 Probar

```bash
aym test
```

### 7.4 Dependencias

```bash
aym add math ^1.2.0
aym lock check
aym lock sync
```

## 8. Flujo de trabajo recomendado

1. Crear un archivo `.aym` o un proyecto con `aym new`.
2. Validar primero con `aymc --check`.
3. Compilar con `aymc -o build/app archivo.aym` o `aym build`.
4. Ejecutar pruebas con `aym test` o los scripts del repositorio.
5. Si usas GUI en Linux, validar que X11 esté disponible.

## 9. Archivos relevantes del usuario

- `aym.toml`: manifest del proyecto.
- `aym.lock`: lockfile reproducible.
- `.aym/cache` y `.aym/repo`: caché/repositorio local de dependencias.

## 10. Resolución de problemas frecuentes

| Síntoma | Causa común | Acción recomendada |
| --- | --- | --- |
| `nasm` no encontrado en Linux | Dependencia ausente en `PATH` | Ejecutar script de dependencias y abrir nueva terminal |
| `gcc` no encontrado en Linux | Toolchain no disponible | Instalar dependencias y validar `gcc --version` |
| `nasm` o `gcc` no encontrados en Windows instalado desde release | Instalación incompleta o `dist/toolchain` ausente | Reinstalar desde el instalador oficial |
| MSI error de privilegios (1925/1603) | Instalación sin elevación suficiente | Usar terminal elevada o `msiexec /i ... /passive` |
| Inconsistencia `manifest-lock` | `aym.toml` y `aym.lock` desalineados | Ejecutar `aym lock sync` o recompilar con lock actualizado |
| GUI no abre en Linux | Falta X11 o ejecución headless | Instalar `libx11-dev`, validar `DISPLAY` o usar `xvfb-run` |
| Ícono `.aym` no se refresca en Windows | Caché visual del shell | Reiniciar `explorer.exe` o cerrar sesión |

## 11. Desinstalación

### 11.1 Windows

```powershell
# MSI
msiexec /x .\AymaraLang-Setup.msi /passive

# NSIS
"C:\Program Files\AymaraLang\Uninstall.exe" /S
```

### 11.2 Linux

```bash
sudo dpkg --purge aymaralang
```

## 12. Buenas prácticas de uso

- Mantener `aym.toml` y `aym.lock` sincronizados.
- Ejecutar `aym test` antes de publicar cambios.
- Usar `--check` para validaciones rápidas en CI o pre-commit.
- Conservar scripts de build y packaging del repositorio como única fuente de verdad.
