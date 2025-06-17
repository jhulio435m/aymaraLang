#!/bin/bash
set -e
make >/dev/null
./bin/aymc samples/hola.aym >/dev/null
./build/out | grep -q Kamisaraki
./bin/aymc samples/else.aym >/dev/null
./build/out | grep -q else
./bin/aymc samples/for.aym >/dev/null
./build/out | grep -q 1
./bin/aymc samples/func.aym >/dev/null
./build/out | grep -q Kamisaraki
./bin/aymc samples/return.aym >/dev/null
./build/out | grep -q 5
./bin/aymc samples/string.aym >/dev/null
./build/out | grep -q Kamisaraki
./bin/aymc samples/advanced_ops.aym >/dev/null
./build/out | grep -q 8
./bin/aymc samples/switch.aym >/dev/null
./build/out | grep -q dos
./bin/aymc samples/logic.aym >/dev/null
./build/out | grep -q ok
./bin/aymc samples/comments.aym >/dev/null
./build/out | grep -q 1
./bin/aymc samples/recursion.aym >/dev/null
./build/out | grep -q 120

