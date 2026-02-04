#include <gtest/gtest.h>
#include "compiler/lexer/lexer.h"
#include "compiler/parser/parser.h"
#include "compiler/codegen/codegen.h"
#include "compiler/ast/ast.h"
#include "compiler/utils/module_resolver.h"
#include <fstream>
#include <cstdio>
#include <stdexcept>
#include <filesystem>
#include <cmath>

using namespace aym;
namespace fs = std::filesystem;

TEST(LexerTest, SimpleTokenize) {
    Lexer lexer("qillqa(1);");
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
    Lexer lexer("qallta qillqa(1); tukuya");
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto nodes = parser.parse();
    ASSERT_FALSE(parser.hasError());
    ASSERT_EQ(nodes.size(), 1u);
    auto *printStmt = dynamic_cast<PrintStmt*>(nodes[0].get());
    ASSERT_NE(printStmt, nullptr);
    ASSERT_EQ(printStmt->getExprs().size(), 1u);
    auto *num = dynamic_cast<NumberExpr*>(printStmt->getExprs()[0].get());
    ASSERT_NE(num, nullptr);
    EXPECT_EQ(num->getValue(), 1);
}

TEST(ParserTest, ExpressionPrecedence) {
    Lexer lexer("qallta 1 + 2 * 3; tukuya");
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
    Lexer lexer("qallta ; qillqa(1); *; qillqa(2); tukuya");
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto nodes = parser.parse();
    EXPECT_TRUE(parser.hasError());
    int found = 0;
    for (const auto &node : nodes) {
        auto *printStmt = dynamic_cast<PrintStmt*>(node.get());
        if (!printStmt || printStmt->getExprs().empty()) continue;
        auto *num = dynamic_cast<NumberExpr*>(printStmt->getExprs()[0].get());
        if (!num) continue;
        if (found == 0) {
            EXPECT_EQ(num->getValue(), 1);
        } else if (found == 1) {
            EXPECT_EQ(num->getValue(), 2);
        }
        ++found;
    }
    EXPECT_EQ(found, 2);
}

TEST(CodeGenTest, GeneratesAssembly) {
    std::string src = "qallta qillqa(\"ok\"); tukuya";
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
    mod << "qallta\nlurawi util() : jakhüwi { kuttaya(1); }\ntukuya\n";
    mod.close();

    Lexer lexer("qallta apnaq(\"modules/util\"); tukuya");
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

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
