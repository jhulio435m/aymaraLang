@echo off
REM Build script for AymaraLang compiler using MinGW-w64 and NASM
if not exist build mkdir build
if not exist bin mkdir bin

set LLVM_CONFIG=llvm-config
for /f "delims=" %%i in ('%LLVM_CONFIG% --cxxflags') do set LLVM_CXXFLAGS=%%i
for /f "delims=" %%i in ('%LLVM_CONFIG% --ldflags --libs core support') do set LLVM_LDFLAGS=%%i

g++ -std=c++17 -Wall -O2 %LLVM_CXXFLAGS% ^
  compiler\ast\ast.cpp ^
  compiler\builtins\builtins.cpp ^
  compiler\codegen\codegen.cpp ^
  compiler\codegen\llvm\llvm_codegen.cpp ^
  compiler\lexer\lexer.cpp ^
  compiler\parser\parser.cpp ^
  compiler\semantic\semantic.cpp ^
  compiler\utils\utils.cpp ^
  compiler\utils\error.cpp ^
  compiler\interpreter\interpreter.cpp ^
  compiler\main.cpp ^
  -o bin\aymc.exe ^
  -lstdc++fs %LLVM_LDFLAGS%

