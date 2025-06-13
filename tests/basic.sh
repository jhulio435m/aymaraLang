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

