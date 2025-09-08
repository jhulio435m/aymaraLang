#include <gtest/gtest.h>
#include "compiler/lexer/lexer.h"
#include "compiler/parser/parser.h"
#include "compiler/codegen/codegen.h"
#include "compiler/ast/ast.h"
#define private public
#define protected public
#include "compiler/interpreter/interpreter.h"
#undef private
#undef protected
#include "compiler/builtins/builtins.h"
#include <fstream>
#include <cstdio>

using namespace aym;

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

TEST(CodeGenTest, GeneratesAssembly) {
    std::string src = "willt’aña(\"ok\");";
    Lexer lexer(src);
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto nodes = parser.parse();
    ASSERT_FALSE(parser.hasError());

    CodeGenerator cg;
#ifdef _WIN32
    cg.generate(nodes, "build/test_output.asm", {}, {}, {}, true);
#else
    cg.generate(nodes, "build/test_output.asm", {}, {}, {}, false);
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

TEST(InterpreterTest, BuiltinPrintFloat) {
    Interpreter interp;
    testing::internal::CaptureStdout();
    interp.callFunction(BUILTIN_PRINT, {Value::Float(3.14)});
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(output, std::string("3.14\n"));
}

TEST(InterpreterTest, BuiltinPrintBool) {
    Interpreter interp;
    testing::internal::CaptureStdout();
    interp.callFunction(BUILTIN_PRINT, {Value::Bool(true)});
    interp.callFunction(BUILTIN_PRINT, {Value::Bool(false)});
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(output, std::string("cheka\njaniwa\n"));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
