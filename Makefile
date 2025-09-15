# Makefile for AymaraLang Compiler (aymc)

CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2
LDFLAGS ?=

LLVM_CONFIG ?= llvm-config

ifeq ($(OS),Windows_NT)
LLVM_FOUND := $(shell where $(LLVM_CONFIG) 2>nul)
else
LLVM_FOUND := $(shell command -v $(LLVM_CONFIG) 2>/dev/null)
endif

ifeq ($(strip $(LLVM_FOUND)),)
LLVM_ENABLED := 0
LLVM_CXXFLAGS :=
LLVM_LDFLAGS :=
else
LLVM_ENABLED := 1
LLVM_CXXFLAGS := $(shell $(LLVM_CONFIG) --cxxflags)
LLVM_LDFLAGS := $(shell $(LLVM_CONFIG) --ldflags --libs core support)
endif

CXXFLAGS += $(LLVM_CXXFLAGS)
LDFLAGS += $(LLVM_LDFLAGS)

ifeq ($(LLVM_ENABLED),1)
CXXFLAGS += -DAYM_WITH_LLVM
else
$(info LLVM backend deshabilitado (llvm-config no encontrado); construyendo sin soporte LLVM)
endif

SRC_DIR := compiler
BUILD_DIR := build
BIN_DIR := bin

# Source files (explicit globs per subdir for portability)
MAIN_SRC := $(SRC_DIR)/main.cpp
LEXER_SRC := $(wildcard $(SRC_DIR)/lexer/*.cpp)
PARSER_SRC := $(wildcard $(SRC_DIR)/parser/*.cpp)
AST_SRC := $(wildcard $(SRC_DIR)/ast/*.cpp)
CODEGEN_SRC := $(wildcard $(SRC_DIR)/codegen/*.cpp)
LLVM_CODEGEN_SRC := $(wildcard $(SRC_DIR)/codegen/llvm/*.cpp)
UTILS_SRC := $(SRC_DIR)/utils/utils.cpp
MODULE_RESOLVER_SRC := $(SRC_DIR)/utils/module_resolver.cpp
ERROR_SRC := $(SRC_DIR)/utils/error.cpp
SEMANTIC_SRC := $(wildcard $(SRC_DIR)/semantic/*.cpp)
BUILTINS_SRC := $(wildcard $(SRC_DIR)/builtins/*.cpp)
INTERPRETER_SRC := $(wildcard $(SRC_DIR)/interpreter/*.cpp)

SRCS := $(MAIN_SRC) $(LEXER_SRC) $(PARSER_SRC) $(AST_SRC) $(CODEGEN_SRC) $(LLVM_CODEGEN_SRC) \
        $(UTILS_SRC) $(MODULE_RESOLVER_SRC) $(ERROR_SRC) $(SEMANTIC_SRC) \
        $(BUILTINS_SRC) $(INTERPRETER_SRC)

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

test: all
	bash tests/run_tests.sh

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)/aymc
