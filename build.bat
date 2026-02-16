@echo off
setlocal

set "BUILD_DIR=build"
set "CONFIG=Release"

where cmake >nul 2>nul
if errorlevel 1 (
  echo [ERROR] CMake no encontrado en PATH.
  exit /b 1
)

echo [build] Configurando proyecto...
cmake -S . -B "%BUILD_DIR%"
if errorlevel 1 exit /b %errorlevel%

echo [build] Compilando %CONFIG%...
cmake --build "%BUILD_DIR%" --config %CONFIG% -j
if errorlevel 1 exit /b %errorlevel%

if not exist bin mkdir bin

set "AYMC_SRC=%BUILD_DIR%\bin\%CONFIG%\aymc.exe"
set "AYM_SRC=%BUILD_DIR%\bin\%CONFIG%\aym.exe"
if not exist "%AYMC_SRC%" set "AYMC_SRC=%BUILD_DIR%\bin\aymc.exe"
if not exist "%AYM_SRC%" set "AYM_SRC=%BUILD_DIR%\bin\aym.exe"

if exist "%AYMC_SRC%" (
  copy /Y "%AYMC_SRC%" "bin\aymc.exe" >nul
  copy /Y "%AYMC_SRC%" "aymc.exe" >nul
) else (
  echo [ERROR] No se encontro aymc.exe en la carpeta de build.
  exit /b 1
)

if exist "%AYM_SRC%" (
  copy /Y "%AYM_SRC%" "bin\aym.exe" >nul
  copy /Y "%AYM_SRC%" "aym.exe" >nul
)

echo [OK] Build completado. Binarios actualizados en bin\ y raiz del repo.
exit /b 0
