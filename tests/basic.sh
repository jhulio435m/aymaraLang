#!/bin/bash
set -e
make >/dev/null
./bin/aymc samples/hola.aym >/dev/null
./build/out | grep -q Kamisaraki
