#!/bin/bash
set -e

echo "[test] building compiler"
make >/dev/null

run(){
  src=$1
  expect=$2
  ./bin/aymc samples/$src.aym >/dev/null
  ./bin/$src | grep -q -- "$expect"
}

# basic programs
run hola Kamisaraki
run vars 13
run func Kamisaraki
run for 1
run else else
run condloop loop
run logic ok
run recursion 120
run switch dos
run advanced_ops 8
run string Kamisaraki
run range_for 3
run negativos -7
./bin/aymc samples/calculadora.aym >/dev/null
printf "3\n4\n" | ./bin/calculadora | grep -q 7

# input handling
./bin/aymc samples/input.aym >/dev/null
printf "Juan\n20\n" | ./bin/input | grep -q Juan

# error detection
./bin/aymc samples/bad_break.aym 2>tmp.log || true
grep -q "Error" tmp.log
rm tmp.log

echo "[test] all samples compiled and ran successfully"
