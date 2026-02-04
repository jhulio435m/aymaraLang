#!/bin/bash
set -e

echo "[test] building compiler"
make >/dev/null

run_exact(){
  src=$1
  expected_file=$2
  ./bin/aymc samples/$src.aym >/dev/null
  ./samples/$src > tmp.out
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

echo "[test] all samples compiled and ran successfully"

./tests/packaging_smoke.sh
