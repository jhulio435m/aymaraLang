# Makefile for AymaraLang Compiler (aymc)

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
NASM = nasm
LD = ld

SRC_DIR = ./compiler
BUILD_DIR = ./build
BIN_DIR = ./bin

LEXER_SRC = $(SRC_DIR)/lexer/*.cpp
PARSER_SRC = $(SRC_DIR)/parser/*.cpp
AST_SRC = $(SRC_DIR)/ast/*.cpp
CODEGEN_SRC = $(SRC_DIR)/codegen/*.cpp
UTILS_SRC = $(SRC_DIR)/utils/*.cpp

OBJS = $(BUILD_DIR)/lexer.o $(BUILD_DIR)/parser.o $(BUILD_DIR)/ast.o $(BUILD_DIR)/codegen.o $(BUILD_DIR)/utils.o

all: $(BIN_DIR)/aymc

$(BIN_DIR)/aymc: $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BUILD_DIR)/lexer.o: $(LEXER_SRC)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/parser.o: $(PARSER_SRC)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/ast.o: $(AST_SRC)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/codegen.o: $(CODEGEN_SRC)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/utils.o: $(UTILS_SRC)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)/*.o $(BIN_DIR)/aymc
