#!/bin/bash
set -e
make >/dev/null
./bin/aymc samples/hola.aym >/dev/null
./bin/hola | grep -q -- Kamisaraki
./bin/aymc samples/else.aym >/dev/null
./bin/else | grep -q -- else
./bin/aymc samples/for.aym >/dev/null
./bin/for | grep -q -- 1
./bin/aymc samples/func.aym >/dev/null
./bin/func | grep -q -- Kamisaraki
./bin/aymc samples/return.aym >/dev/null
./bin/return | grep -q -- 5
./bin/aymc samples/string.aym >/dev/null
./bin/string | grep -q -- Kamisaraki
./bin/aymc samples/advanced_ops.aym >/dev/null
./bin/advanced_ops | grep -q -- 8
./bin/aymc samples/switch.aym >/dev/null
./bin/switch | grep -q -- dos
./bin/aymc samples/logic.aym >/dev/null
./bin/logic | grep -q -- ok
./bin/aymc samples/comments.aym >/dev/null
./bin/comments | grep -q -- 1
./bin/aymc samples/recursion.aym >/dev/null
./bin/recursion | grep -q -- 120

./bin/aymc samples/negativos.aym >/dev/null
./bin/negativos | grep -q -- -7

