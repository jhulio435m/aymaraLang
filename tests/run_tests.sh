#!/bin/bash
set -e

echo "[test] building compiler"
make >/dev/null

run_exact(){
  src=$1
  expected_file=$2
  ./bin/aymc "samples/$src.aym" >/dev/null
  "./samples/$src" > tmp.out
  diff -u "$expected_file" tmp.out
  rm tmp.out
}


cat > tmp.expected <<'EOF'
jach'a
jisk'a
negativu
positivu par
mantani
jani ok
maya
impar
ch'usa
0
1
2
0
1
0
1
2
3
2
1
0
1
3
4
par
par
EOF
run_exact aymara_flow tmp.expected
rm tmp.expected

./bin/aymc samples/ejemplos/basicos.aym >/dev/null
./samples/ejemplos/basicos > tmp.out
grep -q -- "kamisaraki" tmp.out
grep -q -- "xdxdd" tmp.out
grep -q -- "n = 5" tmp.out
rm tmp.out

./bin/aymc samples/ejemplos/funciones_listas.aym >/dev/null
printf "Ana\n2\n3\nLuis\n" | ./samples/ejemplos/funciones_listas > tmp.out
grep -q -- "Suti = Ana" tmp.out
grep -q -- "Suti = Luis" tmp.out
grep -q -- "120" tmp.out
rm tmp.out

echo "[test] all samples compiled and ran successfully"

./tests/packaging_smoke.sh
