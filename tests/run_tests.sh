#!/bin/bash
set -e

echo "[test] building compiler"
make >/dev/null

test_out_dir="build/tmp/run_tests"
rm -rf "${test_out_dir}"
mkdir -p "${test_out_dir}"
trap 'rm -rf "${test_out_dir}"' EXIT

run_exact(){
  src=$1
  expected_file=$2
  out="${test_out_dir}/$src"
  mkdir -p "$(dirname "${out}")"
  ./bin/aymc "samples/$src.aym" -o "${out}" >/dev/null
  "${out}" > tmp.out
  diff -u "$expected_file" tmp.out
  rm tmp.out
}

compile_sample(){
  src=$1
  out="${test_out_dir}/$src"
  mkdir -p "$(dirname "${out}")"
  ./bin/aymc "samples/$src.aym" -o "${out}" >/dev/null
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

compile_sample fundamentos/basicos
"${test_out_dir}/fundamentos/basicos" > tmp.out
grep -q -- "kamisaraki" tmp.out
grep -q -- "xdxdd" tmp.out
grep -q -- "n = 5" tmp.out
rm tmp.out

compile_sample fundamentos/funciones_listas
printf "Ana\n2\n3\nLuis\n" | "${test_out_dir}/fundamentos/funciones_listas" > tmp.out
grep -q -- "Suti = Ana" tmp.out
grep -q -- "Suti = Luis" tmp.out
grep -q -- "120" tmp.out
rm tmp.out

cat > tmp.expected <<'EOF'
0
3
hola
["1", "2", "3"]
["a", "b", "c"]
[""]
["a"]
a-b-c
ok xdd ok
Chiqa
0
3
[1, 2, 3]
c
["a", "b"]
20
[10, 30]
Chiqa
K'ari
VACIO
INDICE
CONVERSION
EOF
compile_sample colecciones/stdlib_texto_listas
"${test_out_dir}/colecciones/stdlib_texto_listas" > tmp.out
diff -u tmp.expected tmp.out
rm tmp.expected tmp.out

cat > tmp.expected <<'EOF'
Ana
{nombre: "Ana", ciudad: "Huancayo"}
Chiqa
K'ari
0
2
["a", "b"]
[1, 2]
1
0
{a: 2, b: 1}
x=10
y=20
EOF
compile_sample colecciones/mapas
"${test_out_dir}/colecciones/mapas" > tmp.out
diff -u tmp.expected tmp.out
rm tmp.expected tmp.out

echo "[test] all samples compiled and ran successfully"

./tests/packaging_smoke.sh
