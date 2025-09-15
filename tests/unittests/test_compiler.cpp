#include <gtest/gtest.h>
#include "compiler/lexer/lexer.h"
#include "compiler/parser/parser.h"
#include "compiler/codegen/codegen.h"
#include "compiler/ast/ast.h"
#include "compiler/utils/module_resolver.h"
#define private public
#define protected public
#include "compiler/interpreter/interpreter.h"
#undef private
#undef protected
#include "compiler/builtins/builtins.h"
#include <fstream>
#include <cstdio>
#include <stdexcept>
#include <filesystem>
#include <cmath>

using namespace aym;
namespace fs = std::filesystem;

TEST(LexerTest, SimpleTokenize) {
    Lexer lexer("willt’aña(1);");
    auto tokens = lexer.tokenize();
    ASSERT_EQ(tokens.size(), 6);
    EXPECT_EQ(tokens[0].type, TokenType::KeywordPrint);
    EXPECT_EQ(tokens[1].type, TokenType::LParen);
    EXPECT_EQ(tokens[2].type, TokenType::Number);
    EXPECT_EQ(tokens[2].text, "1");
    EXPECT_EQ(tokens[3].type, TokenType::RParen);
    EXPECT_EQ(tokens[4].type, TokenType::Semicolon);
    EXPECT_EQ(tokens[5].type, TokenType::EndOfFile);
}

TEST(LexerTest, EscapedString) {
    Lexer lexer(R"("line1\nline2\t\"quoted\"\\")");
    auto tokens = lexer.tokenize();
    ASSERT_EQ(tokens.size(), 2u);
    EXPECT_EQ(tokens[0].type, TokenType::String);
    EXPECT_EQ(tokens[0].text, std::string("line1\nline2\t\"quoted\"\\"));
    EXPECT_EQ(tokens[1].type, TokenType::EndOfFile);
}

TEST(LexerTest, UnterminatedString) {
    Lexer lexer("\"missing end");
    try {
        lexer.tokenize();
        FAIL() << "Expected std::runtime_error";
    } catch (const std::runtime_error &e) {
        std::string msg = e.what();
        EXPECT_NE(msg.find("line 1"), std::string::npos);
        EXPECT_NE(msg.find("column 1"), std::string::npos);
    }
}

TEST(LexerTest, UnterminatedBlockComment) {
    Lexer lexer("/* comment");
    EXPECT_THROW(lexer.tokenize(), std::runtime_error);
}

TEST(ParserTest, ParsePrintStmt) {
    Lexer lexer("willt’aña(1);");
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto nodes = parser.parse();
    ASSERT_FALSE(parser.hasError());
    ASSERT_EQ(nodes.size(), 1u);
    auto *printStmt = dynamic_cast<PrintStmt*>(nodes[0].get());
    ASSERT_NE(printStmt, nullptr);
    auto *num = dynamic_cast<NumberExpr*>(printStmt->getExpr());
    ASSERT_NE(num, nullptr);
    EXPECT_EQ(num->getValue(), 1);
}

TEST(ParserTest, ExpressionPrecedence) {
    Lexer lexer("1 + 2 * 3;");
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto nodes = parser.parse();
    ASSERT_FALSE(parser.hasError());
    ASSERT_EQ(nodes.size(), 1u);
    auto *exprStmt = dynamic_cast<ExprStmt*>(nodes[0].get());
    ASSERT_NE(exprStmt, nullptr);
    auto *add = dynamic_cast<BinaryExpr*>(exprStmt->getExpr());
    ASSERT_NE(add, nullptr);
    EXPECT_EQ(add->getOp(), '+');
    auto *left = dynamic_cast<NumberExpr*>(add->getLeft());
    ASSERT_NE(left, nullptr);
    EXPECT_EQ(left->getValue(), 1);
    auto *mul = dynamic_cast<BinaryExpr*>(add->getRight());
    ASSERT_NE(mul, nullptr);
    EXPECT_EQ(mul->getOp(), '*');
}

TEST(ParserTest, MultipleErrorsRecovery) {
    Lexer lexer("; willt’aña(1); *; willt’aña(2);");
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto nodes = parser.parse();
    EXPECT_TRUE(parser.hasError());
    ASSERT_GE(nodes.size(), 4u);
    auto *print1 = dynamic_cast<PrintStmt*>(nodes[1].get());
    ASSERT_NE(print1, nullptr);
    auto *num1 = dynamic_cast<NumberExpr*>(print1->getExpr());
    ASSERT_NE(num1, nullptr);
    EXPECT_EQ(num1->getValue(), 1);
    auto *print2 = dynamic_cast<PrintStmt*>(nodes[3].get());
    ASSERT_NE(print2, nullptr);
    auto *num2 = dynamic_cast<NumberExpr*>(print2->getExpr());
    ASSERT_NE(num2, nullptr);
    EXPECT_EQ(num2->getValue(), 2);
}

TEST(CodeGenTest, GeneratesAssembly) {
    std::string src = "willt’aña(\"ok\");";
    Lexer lexer(src);
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto nodes = parser.parse();
    ASSERT_FALSE(parser.hasError());
    std::filesystem::create_directory("build");
    std::filesystem::create_directory("bin");
    CodeGenerator cg;
#ifdef _WIN32
    cg.generate(nodes, "build/test_output.asm", {}, {}, {}, true, 0);
#else
    cg.generate(nodes, "build/test_output.asm", {}, {}, {}, false, 0);
#endif

    std::ifstream in("build/test_output.asm");
    ASSERT_TRUE(in.is_open());
    std::string contents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();
    EXPECT_NE(contents.find("ok"), std::string::npos);

    std::remove("build/test_output.asm");
#ifdef _WIN32
    std::remove("build/test_output.obj");
    std::remove("bin/test_output.exe");
#else
    std::remove("build/test_output.o");
    std::remove("bin/test_output");
#endif
}

TEST(ModuleResolverTest, LoadsModuleFromRelativeDirectory) {
    fs::path base = fs::current_path() / "tests" / "tmp_modules";
    fs::create_directories(base / "modules");
    std::ofstream mod(base / "modules" / "util.aym");
    mod << "luräwi util() { kutiyana(1); }\n";
    mod.close();

    Lexer lexer("apu \"modules/util\";");
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto nodes = parser.parse();
    ASSERT_FALSE(parser.hasError());

    ModuleResolver resolver(base);
    resolver.resolve(nodes, base);

    ASSERT_FALSE(nodes.empty());
    auto *fn = dynamic_cast<FunctionStmt*>(nodes[0].get());
    ASSERT_NE(fn, nullptr);
    EXPECT_EQ(fn->getName(), "util");

    fs::remove_all(base);
}

TEST(InterpreterTest, BuiltinPrintFloat) {
    Interpreter interp;
    testing::internal::CaptureStdout();
    interp.callFunction(BUILTIN_PRINT, {Value::Float(3.14)}, 0, 0);
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(output, std::string("3.14\n"));
}

TEST(InterpreterTest, BuiltinPrintBool) {
    Interpreter interp;
    testing::internal::CaptureStdout();
    interp.callFunction(BUILTIN_PRINT, {Value::Bool(true)}, 0, 0);
    interp.callFunction(BUILTIN_PRINT, {Value::Bool(false)}, 0, 0);
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(output, std::string("cheka\njaniwa\n"));
}

TEST(InterpreterTest, MathBuiltinsProduceFloats) {
    Interpreter interp;

    auto sinVal = interp.callFunction(BUILTIN_SIN, {Value::Float(0.0)}, 0, 0);
    EXPECT_EQ(sinVal.type, Value::Type::Float);
    EXPECT_NEAR(sinVal.f, 0.0, 1e-9);

    auto cosVal = interp.callFunction(BUILTIN_COS, {Value::Float(0.0)}, 0, 0);
    EXPECT_EQ(cosVal.type, Value::Type::Float);
    EXPECT_NEAR(cosVal.f, 1.0, 1e-9);

    auto sqrtVal = interp.callFunction(BUILTIN_SQRT, {Value::Float(9.0)}, 0, 0);
    EXPECT_EQ(sqrtVal.type, Value::Type::Float);
    EXPECT_NEAR(sqrtVal.f, 3.0, 1e-9);

    auto powVal = interp.callFunction(BUILTIN_POW,
                                      {Value::Float(2.0), Value::Float(3.0)}, 0, 0);
    EXPECT_EQ(powVal.type, Value::Type::Float);
    EXPECT_NEAR(powVal.f, 8.0, 1e-9);

    auto logVal = interp.callFunction(BUILTIN_LOG,
                                      {Value::Float(std::exp(1.0))}, 0, 0);
    EXPECT_EQ(logVal.type, Value::Type::Float);
    EXPECT_NEAR(logVal.f, 1.0, 1e-9);

    auto fabsVal = interp.callFunction(BUILTIN_FABS, {Value::Float(-3.5)}, 0, 0);
    EXPECT_EQ(fabsVal.type, Value::Type::Float);
    EXPECT_NEAR(fabsVal.f, 3.5, 1e-9);
}

TEST(InterpreterTest, BuiltinArrayLength) {
    Interpreter interp;
    auto handle = interp.callFunction(BUILTIN_ARRAY_NEW, {Value::Int(5)}, 0, 0);
    auto len = interp.callFunction(BUILTIN_ARRAY_LENGTH, {handle}, 0, 0);
    EXPECT_EQ(len.i, 5);
    interp.callFunction(BUILTIN_ARRAY_FREE, {handle}, 0, 0);
}

TEST(InterpreterTest, ArrayNewAndDefaultValues) {
    Interpreter interp;
    auto handle = interp.callFunction(BUILTIN_ARRAY_NEW, {Value::Int(3)}, 0, 0);
    ASSERT_GT(handle.i, 0);
    auto len = interp.callFunction(BUILTIN_ARRAY_LENGTH, {handle}, 0, 0);
    EXPECT_EQ(len.i, 3);
    for (int i = 0; i < 3; ++i) {
        auto val = interp.callFunction(BUILTIN_ARRAY_GET, {handle, Value::Int(i)}, 1, 1);
        EXPECT_EQ(val.i, 0);
    }
    interp.callFunction(BUILTIN_ARRAY_FREE, {handle}, 0, 0);
}

TEST(InterpreterTest, ArraySetAndGet) {
    Interpreter interp;
    auto handle = interp.callFunction(BUILTIN_ARRAY_NEW, {Value::Int(2)}, 0, 0);
    interp.callFunction(BUILTIN_ARRAY_SET,
                        {handle, Value::Int(1), Value::Int(42)}, 1, 1);
    auto val = interp.callFunction(BUILTIN_ARRAY_GET,
                                   {handle, Value::Int(1)}, 1, 1);
    EXPECT_EQ(val.i, 42);
    interp.callFunction(BUILTIN_ARRAY_FREE, {handle}, 0, 0);
}

TEST(InterpreterTest, ArrayGetOutOfBoundsThrows) {
    Interpreter interp;
    auto handle = interp.callFunction(BUILTIN_ARRAY_NEW, {Value::Int(2)}, 0, 0);
    EXPECT_THROW(interp.callFunction(BUILTIN_ARRAY_GET,
                                    {handle, Value::Int(2)}, 1, 1),
                 std::runtime_error);
    interp.callFunction(BUILTIN_ARRAY_FREE, {handle}, 0, 0);
}

TEST(InterpreterTest, ArraySetOutOfBoundsThrows) {
    Interpreter interp;
    auto handle = interp.callFunction(BUILTIN_ARRAY_NEW, {Value::Int(2)}, 0, 0);
    EXPECT_THROW(interp.callFunction(BUILTIN_ARRAY_SET,
                                    {handle, Value::Int(2), Value::Int(99)}, 1, 1),
                 std::runtime_error);
    interp.callFunction(BUILTIN_ARRAY_FREE, {handle}, 0, 0);
}

TEST(InterpreterTest, ArrayGetInvalidHandleReturnsZero) {
    Interpreter interp;
    auto val = interp.callFunction(BUILTIN_ARRAY_GET,
                                   {Value::Int(0), Value::Int(0)}, 1, 1);
    EXPECT_EQ(val.i, 0);
    val = interp.callFunction(BUILTIN_ARRAY_GET,
                               {Value::Int(99), Value::Int(0)}, 1, 1);
    EXPECT_EQ(val.i, 0);
}

TEST(InterpreterTest, ArrayLengthInvalidHandleReturnsZero) {
    Interpreter interp;
    auto len = interp.callFunction(BUILTIN_ARRAY_LENGTH, {Value::Int(0)}, 0, 0);
    EXPECT_EQ(len.i, 0);
    len = interp.callFunction(BUILTIN_ARRAY_LENGTH, {Value::Int(123)}, 0, 0);
    EXPECT_EQ(len.i, 0);
}

TEST(InterpreterTest, ArrayGetNegativeIndexThrows) {
    Interpreter interp;
    auto handle = interp.callFunction(BUILTIN_ARRAY_NEW, {Value::Int(3)}, 0, 0);
    EXPECT_THROW(interp.callFunction(BUILTIN_ARRAY_GET,
                                    {handle, Value::Int(-1)}, 1, 1),
                 std::runtime_error);
    interp.callFunction(BUILTIN_ARRAY_FREE, {handle}, 0, 0);
}

TEST(InterpreterTest, LookupUndefinedVariableThrows) {
    Interpreter interp;
    EXPECT_THROW(interp.lookup("missing", 0, 0), std::runtime_error);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
