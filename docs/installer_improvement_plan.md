# Plan de mejora del instalador

## Hallazgos

- El instalador NSIS dependía de `VC_redist.x64.exe` como archivo externo en
  tiempo de instalación. Si el usuario ejecutaba solo `AymaraLang-Setup.exe`,
  la instalación podía abortar aunque el build del instalador hubiese sido
  exitoso.
- La versión de NSIS estaba hardcodeada en `installer/aymaralang.nsi`, mientras
  que el MSI ya leía `VERSION.txt`. Esto abría la puerta a artefactos Windows
  con metadatos inconsistentes.
- El repo no tenía un smoke test dedicado para validar la generación de
  instaladores Windows a partir del build actual.

## Fase 1

- Embebido de `VC_redist.x64.exe` dentro del instalador NSIS y ejecución desde
  `$PLUGINSDIR`.
- Propagación de `VERSION.txt` hacia el build de NSIS.
- Smoke test automatizado para construir `dist`, generar `AymaraLang-Setup.exe`
  y `AymaraLang-Setup.msi`, y validar que ambos artefactos existan.

## Fase 2

- Unificar la detección de prerequisitos entre NSIS, WiX y
  `scripts/install/install_deps_windows.ps1`.
- Corregir la lógica de alta/baja en `PATH` del instalador NSIS para evitar
  duplicados y remociones parciales.
- Hacer explícita la estrategia de toolchain soportada en Windows
  (`gcc`/`g++`, MSYS2/MinGW, MSVC) para que instalador y documentación no
  diverjan.

## Fase 3

- Agregar smoke e2e de instalación/desinstalación en Windows con validación de
  `PATH`, asociación `.aym` y ejecución mínima de `aymc`.
- Evaluar un bootstrapper MSI/Burn para cubrir redistribuibles de Visual C++
  con la misma experiencia autocontenida del instalador NSIS.
