#!/usr/bin/env bash
# Compara AymaraLang (aymc) contra C++ (g++ -O0 y -O2) sobre los mismos algoritmos.
# Uso: bash samples/bench/cpp_vs_aym/run.sh   (desde cualquier directorio)
# Salida: tabla markdown con el mejor tiempo de pared de 3 ejecuciones.
set -u
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/../../.." && pwd)"
OUT="$ROOT/build/tmp/bench_cpp_vs_aym"
AYMC="${AYMC:-$ROOT/bin/aymc}"
RUNS="${RUNS:-3}"
mkdir -p "$OUT"

best_ms() { # mejor tiempo de pared (ms) de $RUNS ejecuciones
  local best=-1 s e d i
  for ((i = 0; i < RUNS; i++)); do
    s=$(date +%s%N); "$1" >/dev/null 2>&1; e=$(date +%s%N)
    d=$(( (e - s) / 1000000 ))
    if [ "$best" -lt 0 ] || [ "$d" -lt "$best" ]; then best=$d; fi
  done
  echo "$best"
}

echo "| benchmark | aym ms | C++ -O0 ms | C++ -O2 ms | aym / C++ -O2 | aym / C++ -O0 |"
echo "| --- | ---: | ---: | ---: | ---: | ---: |"
status=0
for src in "$HERE"/*.aym; do
  b="$(basename "$src" .aym)"
  if ! "$AYMC" --emit-asm "$src" -o "$OUT/$b" >"$OUT/$b.aymc.log" 2>&1; then
    echo "| $b | ERROR aymc | - | - | - | - |"; status=1; continue
  fi
  g++ -O0 -o "$OUT/${b}_cpp_O0" "$HERE/$b.cpp" || { status=1; continue; }
  g++ -O2 -o "$OUT/${b}_cpp_O2" "$HERE/$b.cpp" || { status=1; continue; }
  a="$("$OUT/$b" 2>&1)"; c0="$("$OUT/${b}_cpp_O0")"; c2="$("$OUT/${b}_cpp_O2")"
  if [ "$a" != "$c2" ] || [ "$a" != "$c0" ]; then
    echo "| $b | SALIDA DISTINTA (aym=$a cpp=$c2) | - | - | - | - |"; status=1; continue
  fi
  if [ -f "$HERE/$b.expected" ] && [ "$a" != "$(cat "$HERE/$b.expected")" ]; then
    echo "| $b | NO COINCIDE CON .expected | - | - | - | - |"; status=1; continue
  fi
  ta=$(best_ms "$OUT/$b"); t0=$(best_ms "$OUT/${b}_cpp_O0"); t2=$(best_ms "$OUT/${b}_cpp_O2")
  r2=$(awk -v a="$ta" -v c="$t2" 'BEGIN{ if (c < 1) c = 1; printf "%.0fx", a / c }')
  r0=$(awk -v a="$ta" -v c="$t0" 'BEGIN{ if (c < 1) c = 1; printf "%.1fx", a / c }')
  echo "| $b | $ta | $t0 | $t2 | $r2 | $r0 |"
done
exit $status
