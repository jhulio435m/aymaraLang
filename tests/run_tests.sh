#!/bin/bash
set -e

echo "[test] building compiler"
make >/dev/null

run(){
  src=$1
  expect=$2
  ./bin/aymc samples/$src.aym >/dev/null
  bin_name=$(basename "$src")
  ./bin/$bin_name | grep -q -- "$expect"
}

# basic programs
run basics/hola Kamisaraki
run basics/vars 13
run functions/func Kamisaraki
run control_flow/for 1
run control_flow/else else
run control_flow/condloop loop
run operators/logic ok
run functions/recursion 120
run control_flow/switch dos
run operators/advanced_ops 8
run data/string Kamisaraki
run control_flow/range_for 3
run operators/negativos -7
run data/array_length 3
./bin/aymc --seed 1 samples/programs/random.aym >/dev/null
out1=$(./bin/random)
out2=$(./bin/random)
[ "$out1" = "$out2" ]

./bin/aymc samples/programs/calculadora.aym >/dev/null
printf "3\n4\n" | ./bin/calculadora | grep -q 7

# input handling
./bin/aymc samples/basics/input.aym >/dev/null
printf "Juan\n20\n" | ./bin/input | grep -q Juan

# error detection
./bin/aymc samples/errors/bad_break.aym 2>tmp.log || true
grep -q "Error" tmp.log
rm tmp.log

echo "[test] all samples compiled and ran successfully"

./tests/packaging_smoke.sh
