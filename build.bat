@echo off
REM Build script for AymaraLang compiler using MinGW-w64 and NASM
if not exist build mkdir build
if not exist bin mkdir bin

g++ -std=c++17 -Wall -O2 ^
  compiler\ast\ast.cpp ^
  compiler\builtins\builtins.cpp ^
  compiler\codegen\codegen.cpp ^
  compiler\lexer\lexer.cpp ^
  compiler\parser\parser.cpp ^
  compiler\semantic\semantic.cpp ^
  compiler\utils\utils.cpp ^
  compiler\utils\error.cpp ^
  compiler\main.cpp ^
  -o bin\aymc.exe

