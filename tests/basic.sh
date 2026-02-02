#!/bin/bash
set -e
make >/dev/null
./bin/aymc samples/basics/hola.aym >/dev/null
./bin/hola | grep -q -- Kamisaraki
./bin/aymc samples/control_flow/else.aym >/dev/null
./bin/else | grep -q -- else
./bin/aymc samples/control_flow/for.aym >/dev/null
./bin/for | grep -q -- 1
./bin/aymc samples/functions/func.aym >/dev/null
./bin/func | grep -q -- Kamisaraki
./bin/aymc samples/functions/return.aym >/dev/null
./bin/return | grep -q -- 5
./bin/aymc samples/data/string.aym >/dev/null
./bin/string | grep -q -- Kamisaraki
./bin/aymc samples/operators/advanced_ops.aym >/dev/null
./bin/advanced_ops | grep -q -- 8
./bin/aymc samples/control_flow/switch.aym >/dev/null
./bin/switch | grep -q -- dos
./bin/aymc samples/operators/logic.aym >/dev/null
./bin/logic | grep -q -- ok
./bin/aymc samples/basics/comments.aym >/dev/null
./bin/comments | grep -q -- 1
./bin/aymc samples/functions/recursion.aym >/dev/null
./bin/recursion | grep -q -- 120

./bin/aymc samples/operators/negativos.aym >/dev/null
./bin/negativos | grep -q -- -7

./bin/aymc --seed 1 samples/programs/random.aym >/dev/null
out1=$(./bin/random)
out2=$(./bin/random)
[ "$out1" = "$out2" ]

./bin/aymc samples/data/array_length.aym >/dev/null
./bin/array_length | grep -q -- 3

