#!/bin/bash
# Pruebas "golden": compila cada <nombre>.aym que tenga un <nombre>.expected al
# lado, lo ejecuta y compara la salida estándar byte a byte.
#
# Además, todo .aym dentro de una carpeta `errores/` bajo tests/golden debe ser
# RECHAZADO por el compilador con un diagnóstico (código de salida 1..127), sin
# colgarse ni terminar por señal.
#
# Uso: bash tests/run_golden.sh [directorio ...]
#   AYMC=ruta/al/aymc   compilador a usar (por defecto ./bin/aymc)

set -u

cd "$(dirname "$0")/.."

AYMC="${AYMC:-./bin/aymc}"
OUT_DIR="build/tmp/golden"
COMPILE_TIMEOUT="${COMPILE_TIMEOUT:-60}"
RUN_TIMEOUT="${RUN_TIMEOUT:-30}"

if [ ! -x "$AYMC" ]; then
  echo "[golden] no existe $AYMC; ejecuta 'make' primero" >&2
  exit 2
fi

if [ "$#" -gt 0 ]; then
  roots=("$@")
else
  roots=(tests/golden samples/algoritmos samples/proyectos samples/bench/cpp_vs_aym samples/bench/opt biblioteca)
fi

rm -rf "$OUT_DIR"
mkdir -p "$OUT_DIR"

pass=0
fail=0
failed=()

while IFS= read -r expected; do
  src="${expected%.expected}.aym"
  if [ ! -f "$src" ]; then
    echo "FALLA  $expected (falta $src)"
    fail=$((fail + 1)); failed+=("$src"); continue
  fi
  exe="$OUT_DIR/${src%.aym}"
  mkdir -p "$(dirname "$exe")"
  if ! timeout "$COMPILE_TIMEOUT" "$AYMC" "$src" -o "$exe" >"$exe.compile.log" 2>&1; then
    echo "FALLA  $src (no compila; ver $exe.compile.log)"
    fail=$((fail + 1)); failed+=("$src"); continue
  fi
  timeout "$RUN_TIMEOUT" "$exe" >"$exe.actual" 2>"$exe.stderr" </dev/null
  if diff -u "$expected" "$exe.actual" >"$exe.diff" 2>&1; then
    pass=$((pass + 1))
  else
    echo "FALLA  $src (salida distinta; ver $exe.diff)"
    fail=$((fail + 1)); failed+=("$src")
  fi
done < <(for r in "${roots[@]}"; do [ -d "$r" ] && find "$r" -name '*.expected' -not -path '*/errores/*'; done | sort)

while IFS= read -r src; do
  exe="$OUT_DIR/${src%.aym}"
  mkdir -p "$(dirname "$exe")"
  timeout "$COMPILE_TIMEOUT" "$AYMC" "$src" -o "$exe" >"$exe.compile.log" 2>&1
  code=$?
  if [ "$code" -ge 1 ] && [ "$code" -le 123 ]; then
    pass=$((pass + 1))
  else
    echo "FALLA  $src (debía rechazarse con diagnóstico; código de salida $code)"
    fail=$((fail + 1)); failed+=("$src")
  fi
done < <(for r in "${roots[@]}"; do [ -d "$r" ] && find "$r" -path '*/errores/*' -name '*.aym'; done | sort)

echo "[golden] correctas: $pass  fallidas: $fail"
[ "$fail" -eq 0 ]
