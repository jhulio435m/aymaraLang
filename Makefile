# Makefile for AymaraLang Compiler (aymc)

CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2
LDFLAGS ?=

SRC_DIR := compiler
BUILD_DIR := build
BIN_DIR := bin

# Source files (explicit globs per subdir for portability)
MAIN_SRC := $(SRC_DIR)/main.cpp
LEXER_SRC := $(wildcard $(SRC_DIR)/lexer/*.cpp)
PARSER_SRC := $(wildcard $(SRC_DIR)/parser/*.cpp)
AST_SRC := $(wildcard $(SRC_DIR)/ast/*.cpp)
CODEGEN_SRC := $(wildcard $(SRC_DIR)/codegen/*.cpp)
UTILS_SRC := $(SRC_DIR)/utils/utils.cpp
MODULE_RESOLVER_SRC := $(SRC_DIR)/utils/module_resolver.cpp
ERROR_SRC := $(SRC_DIR)/utils/error.cpp
SEMANTIC_SRC := $(wildcard $(SRC_DIR)/semantic/*.cpp)
BUILTINS_SRC := $(wildcard $(SRC_DIR)/builtins/*.cpp)
BACKEND_SRC := $(wildcard $(SRC_DIR)/backend/*.cpp)
SRCS := $(MAIN_SRC) $(LEXER_SRC) $(PARSER_SRC) $(AST_SRC) $(CODEGEN_SRC) $(BACKEND_SRC) \
        $(UTILS_SRC) $(MODULE_RESOLVER_SRC) $(ERROR_SRC) $(SEMANTIC_SRC) \
        $(BUILTINS_SRC)

# Map each source to an object in build/ mirroring folder structure
OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))

.PHONY: all clean test

all: $(BIN_DIR)/aymc

$(BIN_DIR)/aymc: $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

# Pattern rule: compile any compiler/*.cpp to build/*.o
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

ifeq ($(OS),Windows_NT)
test: all
	pwsh -File tests/test_modulos_auto.ps1 || powershell -File tests/test_modulos_auto.ps1
else
test: all
	bash tests/run_tests.sh
endif

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)/aymc
