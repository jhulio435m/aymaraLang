# Makefile for AymaraLang Compiler (aymc)

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
NASM = nasm
LD = ld

SRC_DIR = ./compiler
BUILD_DIR = ./build
BIN_DIR = ./bin
	
MAIN_SRC = $(SRC_DIR)/main.cpp

LEXER_SRC = $(SRC_DIR)/lexer/*.cpp
PARSER_SRC = $(SRC_DIR)/parser/*.cpp
AST_SRC = $(SRC_DIR)/ast/*.cpp
CODEGEN_SRC = $(SRC_DIR)/codegen/*.cpp
UTILS_SRC = $(SRC_DIR)/utils/utils.cpp
ERROR_SRC = $(SRC_DIR)/utils/error.cpp
SEMANTIC_SRC = $(SRC_DIR)/semantic/*.cpp
BUILTINS_SRC = $(SRC_DIR)/builtins/*.cpp
INTERPRETER_SRC = $(SRC_DIR)/interpreter/*.cpp

OBJS = $(BUILD_DIR)/lexer.o \
       $(BUILD_DIR)/parser.o \
       $(BUILD_DIR)/ast.o \
       $(BUILD_DIR)/codegen.o \
       $(BUILD_DIR)/utils.o \
       $(BUILD_DIR)/error.o \
       $(BUILD_DIR)/semantic.o \
       $(BUILD_DIR)/builtins.o \
       $(BUILD_DIR)/interpreter.o

OBJS_NO_MAIN = $(BUILD_DIR)/lexer.o \
       $(BUILD_DIR)/parser.o \
       $(BUILD_DIR)/ast.o \
       $(BUILD_DIR)/codegen.o \
       $(BUILD_DIR)/utils.o \
       $(BUILD_DIR)/error.o \
       $(BUILD_DIR)/semantic.o \
       $(BUILD_DIR)/builtins.o \
       $(BUILD_DIR)/interpreter.o

TEST_SRC = tests/unittests/test_compiler.cpp
TEST_OBJ = $(BUILD_DIR)/test_compiler.o

OBJS += $(BUILD_DIR)/main.o

all: $(BIN_DIR)/aymc

$(BIN_DIR)/aymc: $(OBJS)
	mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BUILD_DIR)/lexer.o: $(LEXER_SRC)
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/parser.o: $(PARSER_SRC)
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/ast.o: $(AST_SRC)
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/codegen.o: $(CODEGEN_SRC)
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/utils.o: $(UTILS_SRC)
	       mkdir -p $(BUILD_DIR)
	       $(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/error.o: $(ERROR_SRC)
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/semantic.o: $(SEMANTIC_SRC)
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/builtins.o: $(BUILTINS_SRC)
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/interpreter.o: $(INTERPRETER_SRC)
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/main.o: $(MAIN_SRC)
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TEST_OBJ): $(TEST_SRC)
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -I. -c $< -o $@

$(BIN_DIR)/unittests: $(OBJS_NO_MAIN) $(TEST_OBJ)
	mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^ -lgtest -lgtest_main -pthread

test: $(BIN_DIR)/unittests
	$<

clean:
	rm -rf $(BUILD_DIR)/*.o $(BIN_DIR)/aymc
