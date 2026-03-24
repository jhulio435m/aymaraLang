#include <gtest/gtest.h>
#include "compiler/lexer/lexer.h"
#include "compiler/parser/parser.h"
#include "compiler/ast/ast_json.h"
#include "compiler/semantic/semantic.h"
#include "compiler/backend/backend.h"
#include "compiler/codegen/codegen.h"
#include "compiler/ast/ast.h"
#include "compiler/utils/module_resolver.h"
#include "compiler/utils/diagnostic.h"
#include "compiler/utils/driver.h"
#include "compiler/utils/project_manifest.h"
#include "compiler/utils/project_tool.h"
#include "compiler/utils/process.h"
#include "compiler/utils/semver.h"
#include "compiler/utils/utils.h"
#include <fstream>
#include <cstdio>
#include <stdexcept>
#include <filesystem>
#include <cmath>
#include <cstdlib>

using namespace aym;
namespace fs = std::filesystem;

namespace {
std::string normalizeNewlines(std::string text) {
    std::string out;
    out.reserve(text.size());
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\r') {
            continue;
        }
        out.push_back(text[i]);
    }
    return out;
}

class ScopedEnvVar {
public:
    ScopedEnvVar(const std::string &name, const std::string &value) : name_(name) {
        const char *existing = std::getenv(name.c_str());
        if (existing != nullptr) {
            hadOriginal_ = true;
            originalValue_ = existing;
        }
        set(value);
    }

    ~ScopedEnvVar() {
        if (hadOriginal_) {
            set(originalValue_);
        } else {
            unset();
        }
    }

private:
    void set(const std::string &value) {
#ifdef _WIN32
        _putenv_s(name_.c_str(), value.c_str());
#else
        setenv(name_.c_str(), value.c_str(), 1);
#endif
    }

    void unset() {
#ifdef _WIN32
        _putenv_s(name_.c_str(), "");
#else
        unsetenv(name_.c_str());
#endif
    }

    std::string name_;
    bool hadOriginal_ = false;
    std::string originalValue_;
};

std::string executableSuffix() {
#ifdef _WIN32
    return ".exe";
#else
    return "";
#endif
}
} // namespace

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

TEST(LexerTest, InvalidCharacterThrows) {
    Lexer lexer("qallta @ tukuya");
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

TEST(ParserTest, ReportsDiagnosticsWithLocation) {
    Lexer lexer("qallta ; *; qillqa(1); tukuya");
    auto tokens = lexer.tokenize();
    DiagnosticEngine diagnostics;
    Parser parser(tokens, &diagnostics);
    auto nodes = parser.parse();
    (void)nodes;

    EXPECT_TRUE(parser.hasError());
    ASSERT_FALSE(diagnostics.empty());
    EXPECT_TRUE(diagnostics.hasErrors());
    EXPECT_EQ(diagnostics.all()[0].code, "AYM2001");
    EXPECT_GT(diagnostics.all()[0].line, 0u);
    EXPECT_GT(diagnostics.all()[0].column, 0u);
    EXPECT_FALSE(diagnostics.all()[0].suggestion.empty());
}

TEST(ParserTest, ParseSelectiveImportList) {
    Lexer lexer("qallta apnaq(\"modules/util\", [\"uno\", \"paya\"]); tukuya");
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto nodes = parser.parse();
    ASSERT_FALSE(parser.hasError());
    ASSERT_EQ(nodes.size(), 1u);
    auto *imp = dynamic_cast<ImportStmt*>(nodes[0].get());
    ASSERT_NE(imp, nullptr);
    EXPECT_EQ(imp->getModule(), "modules/util");
    ASSERT_EQ(imp->getSymbols().size(), 2u);
    EXPECT_EQ(imp->getSymbols()[0], "uno");
    EXPECT_EQ(imp->getSymbols()[1], "paya");
}

TEST(AstJsonTest, WritesAstJsonProgram) {
    Lexer lexer("qallta qillqa(\"ok\"); tukuya");
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto nodes = parser.parse();
    ASSERT_FALSE(parser.hasError());

    std::filesystem::create_directory("build");
    std::string error;
    bool ok = writeAstJson(nodes, "build/test_ast.json", error);
    ASSERT_TRUE(ok) << error;

    std::ifstream in("build/test_ast.json");
    ASSERT_TRUE(in.is_open());
    std::string contents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    EXPECT_NE(contents.find("\"kind\": \"Program\""), std::string::npos);
    EXPECT_NE(contents.find("\"kind\": \"PrintStmt\""), std::string::npos);

    std::remove("build/test_ast.json");
}

TEST(ParserTest, ParseImportAliasesMap) {
    Lexer lexer("qallta apnaq(\"modules/util\", {\"uno\":\"maya\", \"paya\":\"paya2\"}); tukuya");
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto nodes = parser.parse();
    ASSERT_FALSE(parser.hasError());
    ASSERT_EQ(nodes.size(), 1u);
    auto *imp = dynamic_cast<ImportStmt*>(nodes[0].get());
    ASSERT_NE(imp, nullptr);
    EXPECT_EQ(imp->getModule(), "modules/util");
    ASSERT_EQ(imp->getAliases().size(), 2u);
    EXPECT_EQ(imp->getAliases()[0].first, "uno");
    EXPECT_EQ(imp->getAliases()[0].second, "maya");
    EXPECT_EQ(imp->getAliases()[1].first, "paya");
    EXPECT_EQ(imp->getAliases()[1].second, "paya2");
}

TEST(LexerTest, EnumMatchKeywords) {
    Lexer lexer("siqicha Estado { INICIO } khiti(1){ kuna 1: { } yaqha: { } }");
    auto tokens = lexer.tokenize();
    bool hasEnum = false;
    bool hasMatch = false;
    bool hasCase = false;
    bool hasDefault = false;
    for (const auto &tok : tokens) {
        if (tok.type == TokenType::KeywordEnum) hasEnum = true;
        if (tok.type == TokenType::KeywordMatch) hasMatch = true;
        if (tok.type == TokenType::KeywordCase) hasCase = true;
        if (tok.type == TokenType::KeywordDefault) hasDefault = true;
    }
    EXPECT_TRUE(hasEnum);
    EXPECT_TRUE(hasMatch);
    EXPECT_TRUE(hasCase);
    EXPECT_TRUE(hasDefault);
}

TEST(LexerTest, CanonicalAymaraKeywords) {
    Lexer lexer("qallta jakhuwi taqa kari kuti pakhina sarantana yantana katjana tukuyawi tukuya");
    auto tokens = lexer.tokenize();

    bool hasTypeNumber = false;
    bool hasTypeList = false;
    bool hasFalse = false;
    bool hasFor = false;
    bool hasBreak = false;
    bool hasContinue = false;
    bool hasTry = false;
    bool hasCatch = false;
    bool hasFinally = false;

    for (const auto &tok : tokens) {
        if (tok.type == TokenType::KeywordTypeNumber) hasTypeNumber = true;
        if (tok.type == TokenType::KeywordTypeList) hasTypeList = true;
        if (tok.type == TokenType::KeywordFalse) hasFalse = true;
        if (tok.type == TokenType::KeywordFor) hasFor = true;
        if (tok.type == TokenType::KeywordBreak) hasBreak = true;
        if (tok.type == TokenType::KeywordContinue) hasContinue = true;
        if (tok.type == TokenType::KeywordTry) hasTry = true;
        if (tok.type == TokenType::KeywordCatch) hasCatch = true;
        if (tok.type == TokenType::KeywordFinally) hasFinally = true;
    }

    EXPECT_TRUE(hasTypeNumber);
    EXPECT_TRUE(hasTypeList);
    EXPECT_TRUE(hasFalse);
    EXPECT_TRUE(hasFor);
    EXPECT_TRUE(hasBreak);
    EXPECT_TRUE(hasContinue);
    EXPECT_TRUE(hasTry);
    EXPECT_TRUE(hasCatch);
    EXPECT_TRUE(hasFinally);
}

TEST(ParserTest, ParseEnumAsMapDeclaration) {
    Lexer lexer("qallta siqicha Estado { INICIO, JUGANDO = 5, FIN } tukuya");
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto nodes = parser.parse();
    ASSERT_FALSE(parser.hasError());
    ASSERT_EQ(nodes.size(), 1u);

    auto *decl = dynamic_cast<VarDeclStmt*>(nodes[0].get());
    ASSERT_NE(decl, nullptr);
    EXPECT_EQ(decl->getType(), "mapa");
    EXPECT_EQ(decl->getName(), "Estado");

    auto *mapExpr = dynamic_cast<MapExpr*>(decl->getInit());
    ASSERT_NE(mapExpr, nullptr);
    ASSERT_EQ(mapExpr->getItems().size(), 3u);

    auto *v0 = dynamic_cast<NumberExpr*>(mapExpr->getItems()[0].second.get());
    auto *v1 = dynamic_cast<NumberExpr*>(mapExpr->getItems()[1].second.get());
    auto *v2 = dynamic_cast<NumberExpr*>(mapExpr->getItems()[2].second.get());
    ASSERT_NE(v0, nullptr);
    ASSERT_NE(v1, nullptr);
    ASSERT_NE(v2, nullptr);
    EXPECT_EQ(v0->getValue(), 0);
    EXPECT_EQ(v1->getValue(), 5);
    EXPECT_EQ(v2->getValue(), 6);
}

TEST(ParserTest, ParseMatchAsSwitchStatement) {
    Lexer lexer("qallta khiti(2){ kuna 1: { qillqa(\"a\"); } kuna 2: { qillqa(\"b\"); } yaqha: { qillqa(\"x\"); } } tukuya");
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto nodes = parser.parse();
    ASSERT_FALSE(parser.hasError());
    ASSERT_EQ(nodes.size(), 1u);

    auto *sw = dynamic_cast<SwitchStmt*>(nodes[0].get());
    ASSERT_NE(sw, nullptr);
    ASSERT_EQ(sw->getCases().size(), 2u);
    ASSERT_NE(sw->getDefault(), nullptr);

    for (const auto &c : sw->getCases()) {
        ASSERT_FALSE(c.second->statements.empty());
        auto *brk = dynamic_cast<BreakStmt*>(c.second->statements.back().get());
        ASSERT_NE(brk, nullptr);
    }
}

TEST(ParserTest, IdentifierCanStartWithUnderscore) {
    Lexer lexer("qallta yatiya jakhuwi _contador = 1; qillqa(_contador); tukuya");
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto nodes = parser.parse();
    ASSERT_FALSE(parser.hasError());
    ASSERT_EQ(nodes.size(), 2u);

    auto *decl = dynamic_cast<VarDeclStmt*>(nodes[0].get());
    ASSERT_NE(decl, nullptr);
    EXPECT_EQ(decl->getName(), "_contador");
}

TEST(DriverTest, ParseDiagnosticsJsonOption) {
    char arg0[] = "aymc";
    char arg1[] = "--diagnostics-json=build/tmp/diag.json";
    char arg2[] = "entrada.aym";
    char *argv[] = {arg0, arg1, arg2};

    CompileOptions options;
    std::string errorMsg;
    const auto result = parseCompileOptions(3, argv, options, errorMsg);

    EXPECT_EQ(result, CliParseResult::Ok);
    EXPECT_TRUE(errorMsg.empty());
    EXPECT_TRUE(options.emitDiagnosticsJson);
    EXPECT_EQ(options.diagnosticsJsonPath, "build/tmp/diag.json");
}

TEST(DriverTest, ParseDiagnosticsJsonOptionRejectsEmptyPath) {
    char arg0[] = "aymc";
    char arg1[] = "--diagnostics-json=";
    char arg2[] = "entrada.aym";
    char *argv[] = {arg0, arg1, arg2};

    CompileOptions options;
    std::string errorMsg;
    const auto result = parseCompileOptions(3, argv, options, errorMsg);

    EXPECT_EQ(result, CliParseResult::Error);
    EXPECT_NE(errorMsg.find("--diagnostics-json"), std::string::npos);
}

TEST(DriverTest, ParseEmitAsmOption) {
    char arg0[] = "aymc";
    char arg1[] = "--emit-asm";
    char arg2[] = "entrada.aym";
    char *argv[] = {arg0, arg1, arg2};

    CompileOptions options;
    std::string errorMsg;
    const auto result = parseCompileOptions(3, argv, options, errorMsg);

    EXPECT_EQ(result, CliParseResult::Ok);
    EXPECT_TRUE(errorMsg.empty());
    EXPECT_TRUE(options.emitAsm);
}

TEST(DriverTest, ParseBackendOptionNative) {
    char arg0[] = "aymc";
    char arg1[] = "--backend";
    char arg2[] = "native";
    char arg3[] = "entrada.aym";
    char *argv[] = {arg0, arg1, arg2, arg3};

    CompileOptions options;
    std::string errorMsg;
    const auto result = parseCompileOptions(4, argv, options, errorMsg);

    EXPECT_EQ(result, CliParseResult::Ok);
    EXPECT_TRUE(errorMsg.empty());
    EXPECT_EQ(options.backend, "native");
}

TEST(DriverTest, ParseBackendOptionWithEquals) {
    char arg0[] = "aymc";
    char arg1[] = "--backend=IR";
    char arg2[] = "entrada.aym";
    char *argv[] = {arg0, arg1, arg2};

    CompileOptions options;
    std::string errorMsg;
    const auto result = parseCompileOptions(3, argv, options, errorMsg);

    EXPECT_EQ(result, CliParseResult::Ok);
    EXPECT_TRUE(errorMsg.empty());
    EXPECT_EQ(options.backend, "ir");
}

TEST(DriverTest, ParseBackendOptionRejectsUnknownValue) {
    char arg0[] = "aymc";
    char arg1[] = "--backend=desconocido";
    char arg2[] = "entrada.aym";
    char *argv[] = {arg0, arg1, arg2};

    CompileOptions options;
    std::string errorMsg;
    const auto result = parseCompileOptions(3, argv, options, errorMsg);

    EXPECT_EQ(result, CliParseResult::Error);
    EXPECT_NE(errorMsg.find("Backend no soportado"), std::string::npos);
}

TEST(DriverTest, ParseCompileOnlyOption) {
    char arg0[] = "aymc";
    char arg1[] = "--compile-only";
    char arg2[] = "entrada.aym";
    char *argv[] = {arg0, arg1, arg2};

    CompileOptions options;
    std::string errorMsg;
    const auto result = parseCompileOptions(3, argv, options, errorMsg);

    EXPECT_EQ(result, CliParseResult::Ok);
    EXPECT_TRUE(errorMsg.empty());
    EXPECT_TRUE(options.compileOnly);
}

TEST(DriverTest, ParseTimePipelineOption) {
    char arg0[] = "aymc";
    char arg1[] = "--time-pipeline";
    char arg2[] = "entrada.aym";
    char *argv[] = {arg0, arg1, arg2};

    CompileOptions options;
    std::string errorMsg;
    const auto result = parseCompileOptions(3, argv, options, errorMsg);

    EXPECT_EQ(result, CliParseResult::Ok);
    EXPECT_TRUE(errorMsg.empty());
    EXPECT_TRUE(options.timePipeline);
}

TEST(DriverTest, ParseTimePipelineJsonOption) {
    char arg0[] = "aymc";
    char arg1[] = "--time-pipeline-json=build/tmp/pipeline.json";
    char arg2[] = "entrada.aym";
    char *argv[] = {arg0, arg1, arg2};

    CompileOptions options;
    std::string errorMsg;
    const auto result = parseCompileOptions(3, argv, options, errorMsg);

    EXPECT_EQ(result, CliParseResult::Ok);
    EXPECT_TRUE(errorMsg.empty());
    EXPECT_TRUE(options.emitTimePipelineJson);
    EXPECT_EQ(options.timePipelineJsonPath, "build/tmp/pipeline.json");
}

TEST(DriverTest, ParseTimePipelineJsonOptionRejectsEmptyPath) {
    char arg0[] = "aymc";
    char arg1[] = "--time-pipeline-json=";
    char arg2[] = "entrada.aym";
    char *argv[] = {arg0, arg1, arg2};

    CompileOptions options;
    std::string errorMsg;
    const auto result = parseCompileOptions(3, argv, options, errorMsg);

    EXPECT_EQ(result, CliParseResult::Error);
    EXPECT_NE(errorMsg.find("--time-pipeline-json"), std::string::npos);
}

TEST(DriverTest, ParseToolTimeoutOption) {
    char arg0[] = "aymc";
    char arg1[] = "--tool-timeout-ms=1500";
    char arg2[] = "entrada.aym";
    char *argv[] = {arg0, arg1, arg2};

    CompileOptions options;
    std::string errorMsg;
    const auto result = parseCompileOptions(3, argv, options, errorMsg);

    EXPECT_EQ(result, CliParseResult::Ok);
    EXPECT_TRUE(errorMsg.empty());
    EXPECT_EQ(options.toolTimeoutMs, 1500);
}

TEST(UtilsTest, ResolveToolExecutableUsesBundledToolchainRootOverride) {
    const fs::path base = fs::path("build") / "tmp" / "test_toolchain_root_override";
    fs::remove_all(base);
    fs::create_directories(base / "toolchain" / "bin");
    fs::create_directories(base / "toolchain" / "mingw64" / "bin");

    const fs::path nasmPath = base / "toolchain" / "bin" / ("nasm" + executableSuffix());
    const fs::path gccPath = base / "toolchain" / "mingw64" / "bin" / ("gcc" + executableSuffix());
    {
        std::ofstream nasm(nasmPath);
        nasm << "stub";
    }
    {
        std::ofstream gcc(gccPath);
        gcc << "stub";
    }

    ScopedEnvVar toolchainRoot("AYM_TOOLCHAIN_ROOT", base.string());
    ScopedEnvVar gccOverride("AYM_GCC_PATH", "");
    ScopedEnvVar nasmOverride("AYM_NASM_PATH", "");

    EXPECT_EQ(fs::absolute(resolveToolExecutable("nasm")).lexically_normal(),
              fs::absolute(nasmPath).lexically_normal());
    EXPECT_EQ(fs::absolute(resolveToolExecutable("gcc")).lexically_normal(),
              fs::absolute(gccPath).lexically_normal());

    fs::remove_all(base);
}

TEST(DriverTest, ParseToolTimeoutOptionRejectsNegativeValue) {
    char arg0[] = "aymc";
    char arg1[] = "--tool-timeout-ms=-1";
    char arg2[] = "entrada.aym";
    char *argv[] = {arg0, arg1, arg2};

    CompileOptions options;
    std::string errorMsg;
    const auto result = parseCompileOptions(3, argv, options, errorMsg);

    EXPECT_EQ(result, CliParseResult::Error);
    EXPECT_NE(errorMsg.find("--tool-timeout-ms"), std::string::npos);
}

TEST(DriverTest, LinkOnlyWithoutOutputIsRejected) {
    char arg0[] = "aymc";
    char arg1[] = "--link-only";
    char *argv[] = {arg0, arg1};

    CompileOptions options;
    std::string errorMsg;
    const auto result = parseCompileOptions(2, argv, options, errorMsg);

    EXPECT_EQ(result, CliParseResult::Error);
    EXPECT_NE(errorMsg.find("--link-only"), std::string::npos);
}

TEST(DriverTest, LinkOnlyWithOutputAndNoInputsIsAccepted) {
    char arg0[] = "aymc";
    char arg1[] = "--link-only";
    char arg2[] = "-o";
    char arg3[] = "build/test_link_only";
    char *argv[] = {arg0, arg1, arg2, arg3};

    CompileOptions options;
    std::string errorMsg;
    const auto result = parseCompileOptions(4, argv, options, errorMsg);

    EXPECT_EQ(result, CliParseResult::Ok);
    EXPECT_TRUE(errorMsg.empty());
    EXPECT_TRUE(options.linkOnly);
    EXPECT_TRUE(options.inputs.empty());
    EXPECT_EQ(options.output, "build/test_link_only");
}

TEST(DriverTest, CompileOnlyAndLinkOnlyTogetherAreRejected) {
    char arg0[] = "aymc";
    char arg1[] = "--compile-only";
    char arg2[] = "--link-only";
    char arg3[] = "-o";
    char arg4[] = "build/test_link_only";
    char *argv[] = {arg0, arg1, arg2, arg3, arg4};

    CompileOptions options;
    std::string errorMsg;
    const auto result = parseCompileOptions(5, argv, options, errorMsg);

    EXPECT_EQ(result, CliParseResult::Error);
    EXPECT_NE(errorMsg.find("--compile-only"), std::string::npos);
}

TEST(DriverTest, CheckAndTimePipelineAreRejectedTogether) {
    char arg0[] = "aymc";
    char arg1[] = "--check";
    char arg2[] = "--time-pipeline";
    char arg3[] = "entrada.aym";
    char *argv[] = {arg0, arg1, arg2, arg3};

    CompileOptions options;
    std::string errorMsg;
    const auto result = parseCompileOptions(4, argv, options, errorMsg);

    EXPECT_EQ(result, CliParseResult::Error);
    EXPECT_NE(errorMsg.find("--time-pipeline"), std::string::npos);
}

TEST(DriverTest, HelpTextIncludesModernOptions) {
    const std::string help = compileHelpText();
    EXPECT_NE(help.find("--backend"), std::string::npos);
    EXPECT_NE(help.find("--check"), std::string::npos);
    EXPECT_NE(help.find("--emit-asm"), std::string::npos);
    EXPECT_NE(help.find("--compile-only"), std::string::npos);
    EXPECT_NE(help.find("--link-only"), std::string::npos);
    EXPECT_NE(help.find("--time-pipeline"), std::string::npos);
    EXPECT_NE(help.find("--time-pipeline-json"), std::string::npos);
    EXPECT_NE(help.find("--tool-timeout-ms"), std::string::npos);
    EXPECT_NE(help.find("--emit-ast-json"), std::string::npos);
    EXPECT_NE(help.find("--diagnostics-json"), std::string::npos);
    EXPECT_NE(help.find("--check-manifest"), std::string::npos);
    EXPECT_NE(help.find("--emit-lock"), std::string::npos);
    EXPECT_NE(help.find("--check-lock"), std::string::npos);
    EXPECT_NE(help.find("Ejemplos"), std::string::npos);
}

TEST(DriverTest, HelpTextSnapshot) {
    std::ifstream in("tests/snapshots/aymc_help.txt");
    ASSERT_TRUE(in.is_open());
    std::string expected((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    const std::string current = compileHelpText();
    EXPECT_EQ(normalizeNewlines(current), normalizeNewlines(expected));
}

TEST(DriverTest, ManifestFlagsWithoutInputsAreAccepted) {
    char arg0[] = "aymc";
    char arg1[] = "--check-manifest";
    char arg2[] = "--emit-lock";
    char *argv[] = {arg0, arg1, arg2};

    CompileOptions options;
    std::string errorMsg;
    const auto result = parseCompileOptions(3, argv, options, errorMsg);

    EXPECT_EQ(result, CliParseResult::Ok);
    EXPECT_TRUE(errorMsg.empty());
    EXPECT_TRUE(options.checkManifest);
    EXPECT_TRUE(options.emitLock);
    EXPECT_TRUE(options.inputs.empty());
}

TEST(DriverTest, ManifestPathFlagIsApplied) {
    char arg0[] = "aymc";
    char arg1[] = "--check-manifest";
    char arg2[] = "--manifest=build/tmp/aym.toml";
    char *argv[] = {arg0, arg1, arg2};

    CompileOptions options;
    std::string errorMsg;
    const auto result = parseCompileOptions(3, argv, options, errorMsg);

    EXPECT_EQ(result, CliParseResult::Ok);
    EXPECT_TRUE(options.checkManifest);
    EXPECT_EQ(options.manifestPath, "build/tmp/aym.toml");
}

TEST(DriverTest, CheckLockFlagWithoutInputsIsAccepted) {
    char arg0[] = "aymc";
    char arg1[] = "--check-lock";
    char *argv[] = {arg0, arg1};

    CompileOptions options;
    std::string errorMsg;
    const auto result = parseCompileOptions(2, argv, options, errorMsg);

    EXPECT_EQ(result, CliParseResult::Ok);
    EXPECT_TRUE(options.checkLock);
    EXPECT_TRUE(options.inputs.empty());
}

TEST(SemverTest, ParsesVersionAndRangeRequirement) {
    SemverVersion version;
    std::string versionError;
    ASSERT_TRUE(parseSemverVersion("1.2.3", version, versionError)) << versionError;
    EXPECT_EQ(version.major, 1);
    EXPECT_EQ(version.minor, 2);
    EXPECT_EQ(version.patch, 3);

    SemverRequirement requirement;
    std::string requirementError;
    ASSERT_TRUE(parseSemverRequirement(">=1.2.0, <2.0.0", requirement, requirementError)) << requirementError;
    EXPECT_FALSE(requirement.any);
    ASSERT_EQ(requirement.comparators.size(), 2u);

    EXPECT_TRUE(semverSatisfies(version, requirement));
}

TEST(SemverTest, InfersResolvedVersionFromCaretRequirement) {
    SemverRequirement requirement;
    std::string requirementError;
    ASSERT_TRUE(parseSemverRequirement("^2.4.1", requirement, requirementError)) << requirementError;

    SemverVersion resolved;
    std::string resolveError;
    ASSERT_TRUE(inferResolvedVersion(requirement, resolved, resolveError)) << resolveError;
    EXPECT_EQ(formatSemverVersion(resolved), "2.4.1");
}

TEST(BackendDispatchTest, ParseBackendKindRecognizesNativeAndIr) {
    BackendKind kind = BackendKind::Native;
    std::string error;

    EXPECT_TRUE(parseBackendKind("native", kind, error));
    EXPECT_TRUE(error.empty());
    EXPECT_EQ(kind, BackendKind::Native);

    EXPECT_TRUE(parseBackendKind("IR", kind, error));
    EXPECT_TRUE(error.empty());
    EXPECT_EQ(kind, BackendKind::Ir);
}

TEST(BackendDispatchTest, IrBackendGeneratesPrototypeArtifact) {
    fs::create_directories(fs::path("build") / "tmp");
    const fs::path irOutputBase = fs::path("build") / "tmp" / "backend_ir_prototype.asm";
    const fs::path irPath = fs::path("build") / "tmp" / "backend_ir_prototype.ir";
    std::remove(irPath.string().c_str());

    std::vector<std::unique_ptr<Node>> nodes;
    std::unordered_set<std::string> globals;
    std::unordered_map<std::string, std::vector<std::string>> paramTypes;
    std::unordered_map<std::string, std::string> functionReturnTypes;
    std::unordered_map<std::string, std::string> globalTypes;
    std::string error;

    const bool ok = runBackendCompile(BackendKind::Ir,
                                      nodes,
                                      irOutputBase.string(),
                                      globals,
                                      paramTypes,
                                      functionReturnTypes,
                                      globalTypes,
                                      true,
                                      0,
                                      "runtime",
                                      false,
                                      CodegenPipelineMode::Full,
                                      false,
                                      "",
                                      0,
                                      error);
    ASSERT_TRUE(ok) << error;
    EXPECT_TRUE(fs::exists(irPath));

    std::ifstream in(irPath);
    ASSERT_TRUE(in.is_open());
    std::string contents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();
    EXPECT_NE(contents.find("backend = ir"), std::string::npos);

    std::remove(irPath.string().c_str());
}

TEST(BackendDispatchTest, IrBackendRejectsLinkOnlyMode) {
    std::vector<std::unique_ptr<Node>> nodes;
    std::unordered_set<std::string> globals;
    std::unordered_map<std::string, std::vector<std::string>> paramTypes;
    std::unordered_map<std::string, std::string> functionReturnTypes;
    std::unordered_map<std::string, std::string> globalTypes;
    std::string error;

    const bool ok = runBackendCompile(BackendKind::Ir,
                                      nodes,
                                      "build/backend_ir_link_only.asm",
                                      globals,
                                      paramTypes,
                                      functionReturnTypes,
                                      globalTypes,
                                      true,
                                      0,
                                      "runtime",
                                      false,
                                      CodegenPipelineMode::LinkOnly,
                                      false,
                                      "",
                                      0,
                                      error);
    EXPECT_FALSE(ok);
    EXPECT_NE(error.find("--link-only"), std::string::npos);
}

TEST(BackendDispatchTest, NativeBackendReportsDetailedFailureForMissingObjectInLinkOnly) {
    fs::create_directories(fs::path("build") / "tmp");
    const fs::path asmPath = fs::path("build") / "tmp" / "backend_native_missing_obj.asm";
#ifdef _WIN32
    std::remove((fs::path("build") / "tmp" / "backend_native_missing_obj.obj").string().c_str());
#else
    std::remove((fs::path("build") / "tmp" / "backend_native_missing_obj.o").string().c_str());
#endif

    std::vector<std::unique_ptr<Node>> nodes;
    std::unordered_set<std::string> globals;
    std::unordered_map<std::string, std::vector<std::string>> paramTypes;
    std::unordered_map<std::string, std::string> functionReturnTypes;
    std::unordered_map<std::string, std::string> globalTypes;
    std::string error;

    const bool ok = runBackendCompile(BackendKind::Native,
                                      nodes,
                                      asmPath.string(),
                                      globals,
                                      paramTypes,
                                      functionReturnTypes,
                                      globalTypes,
#ifdef _WIN32
                                      true,
#else
                                      false,
#endif
                                      0,
                                      "runtime",
                                      false,
                                      CodegenPipelineMode::LinkOnly,
                                      false,
                                      "",
                                      0,
                                      error);
    EXPECT_FALSE(ok);
    EXPECT_NE(error.find("pre-link"), std::string::npos);
    EXPECT_NE(error.find("objeto"), std::string::npos);
    EXPECT_NE(error.find("exit_code"), std::string::npos);
}

TEST(ProcessTest, ReportsFailureForMissingCommand) {
    ProcessResult result;
    const bool ok = runProcessCommand({"__aym_no_existe_cmd__"}, result);
    EXPECT_FALSE(ok);
    EXPECT_NE(result.exitCode, 0);
    EXPECT_FALSE(result.command.empty());
}

TEST(ProcessTest, RunsSimpleSuccessCommand) {
    ProcessResult result;
#ifdef _WIN32
    const bool ok = runProcessCommand({"cmd", "/c", "exit", "0"}, result);
#else
    const bool ok = runProcessCommand({"sh", "-c", "exit 0"}, result);
#endif
    EXPECT_TRUE(ok);
    EXPECT_EQ(result.exitCode, 0);
}

TEST(ProcessTest, EnforcesTimeoutWhenConfigured) {
    ProcessResult result;
    ProcessOptions options;
    options.timeoutMs = 100;
#ifdef _WIN32
    const bool ok = runProcessCommand({"cmd", "/c", "ping", "-n", "6", "127.0.0.1"}, result, options);
#else
    const bool ok = runProcessCommand({"sh", "-c", "sleep 2"}, result, options);
#endif
    EXPECT_FALSE(ok);
    EXPECT_TRUE(result.timedOut);
    EXPECT_EQ(result.exitCode, 124);
    EXPECT_NE(result.error.find("timeout"), std::string::npos);
}

TEST(ProcessTest, CapturesStdoutAndStderrWhenRequested) {
    ProcessResult result;
    ProcessOptions options;
    options.captureOutput = true;
    options.maxCaptureBytes = 256;
#ifdef _WIN32
    const bool ok = runProcessCommand({"cmd", "/c", "(echo out) & (echo err 1>&2) & exit 7"}, result, options);
#else
    const bool ok = runProcessCommand({"sh", "-c", "echo out; echo err 1>&2; exit 7"}, result, options);
#endif
    EXPECT_FALSE(ok);
    EXPECT_EQ(result.exitCode, 7);
    EXPECT_NE(result.stdoutText.find("out"), std::string::npos);
    EXPECT_NE(result.stderrText.find("err"), std::string::npos);
}

TEST(ProcessTest, TruncatesCapturedOutputByLimit) {
    ProcessResult result;
    ProcessOptions options;
    options.captureOutput = true;
    options.maxCaptureBytes = 8;
#ifdef _WIN32
    const bool ok = runProcessCommand({"cmd", "/c", "echo 12345678901234567890"}, result, options);
#else
    const bool ok = runProcessCommand({"sh", "-c", "echo 12345678901234567890"}, result, options);
#endif
    EXPECT_TRUE(ok);
    EXPECT_TRUE(result.stdoutTruncated);
    EXPECT_LE(result.stdoutText.size(), static_cast<size_t>(8));
}

TEST(ProjectManifestTest, ParsesValidManifest) {
    fs::create_directories(fs::path("build") / "tmp");
    const fs::path manifestPath = fs::path("build") / "tmp" / "test_aym.toml";
    {
        std::ofstream out(manifestPath);
        out << "[package]\n";
        out << "name = \"demo\"\n";
        out << "version = \"0.1.0\"\n";
        out << "edition = \"2026\"\n";
        out << "\n";
        out << "[dependencies]\n";
        out << "math = \"1.0.0\"\n";
        out << "io = \"2.3.4\"\n";
    }

    ProjectManifest manifest;
    std::string error;
    ASSERT_TRUE(parseProjectManifestFile(manifestPath.string(), manifest, error)) << error;
    EXPECT_EQ(manifest.name, "demo");
    EXPECT_EQ(manifest.version, "0.1.0");
    EXPECT_EQ(manifest.edition, "2026");
    ASSERT_EQ(manifest.dependencies.size(), 2u);
    EXPECT_EQ(manifest.dependencies["math"], "1.0.0");
    EXPECT_EQ(manifest.dependencies["io"], "2.3.4");

    std::remove(manifestPath.string().c_str());
}

TEST(ProjectManifestTest, RejectsMissingVersion) {
    fs::create_directories(fs::path("build") / "tmp");
    const fs::path manifestPath = fs::path("build") / "tmp" / "test_aym_invalid.toml";
    {
        std::ofstream out(manifestPath);
        out << "[package]\n";
        out << "name = \"demo\"\n";
    }

    ProjectManifest manifest;
    std::string error;
    EXPECT_FALSE(parseProjectManifestFile(manifestPath.string(), manifest, error));
    EXPECT_NE(error.find("version"), std::string::npos);

    std::remove(manifestPath.string().c_str());
}

TEST(ProjectManifestTest, RejectsInvalidPackageVersionWithDetailedCode) {
    fs::create_directories(fs::path("build") / "tmp");
    const fs::path manifestPath = fs::path("build") / "tmp" / "test_aym_bad_version.toml";
    {
        std::ofstream out(manifestPath);
        out << "[package]\n";
        out << "name = \"demo\"\n";
        out << "version = \"1.0\"\n";
    }

    ProjectManifest manifest;
    ManifestError error;
    EXPECT_FALSE(parseProjectManifestFileDetailed(manifestPath.string(), manifest, error));
    EXPECT_EQ(error.code, "AYM5008");
    EXPECT_GT(error.line, 0u);

    std::remove(manifestPath.string().c_str());
}

TEST(ProjectManifestTest, RejectsInvalidDependencyRequirementWithDetailedCode) {
    fs::create_directories(fs::path("build") / "tmp");
    const fs::path manifestPath = fs::path("build") / "tmp" / "test_aym_bad_requirement.toml";
    {
        std::ofstream out(manifestPath);
        out << "[package]\n";
        out << "name = \"demo\"\n";
        out << "version = \"0.1.0\"\n";
        out << "\n";
        out << "[dependencies]\n";
        out << "math = \"v1\"\n";
    }

    ProjectManifest manifest;
    ManifestError error;
    EXPECT_FALSE(parseProjectManifestFileDetailed(manifestPath.string(), manifest, error));
    EXPECT_EQ(error.code, "AYM5009");
    EXPECT_GT(error.line, 0u);

    std::remove(manifestPath.string().c_str());
}

TEST(ProjectManifestTest, DetailedErrorIncludesCodeAndLine) {
    fs::create_directories(fs::path("build") / "tmp");
    const fs::path manifestPath = fs::path("build") / "tmp" / "test_aym_bad_syntax.toml";
    {
        std::ofstream out(manifestPath);
        out << "[package]\n";
        out << "name = demo\n"; // missing quotes
        out << "version = \"0.1.0\"\n";
    }

    ProjectManifest manifest;
    ManifestError error;
    EXPECT_FALSE(parseProjectManifestFileDetailed(manifestPath.string(), manifest, error));
    EXPECT_EQ(error.code, "AYM5002");
    EXPECT_GT(error.line, 0u);
    EXPECT_FALSE(error.message.empty());

    std::remove(manifestPath.string().c_str());
}

TEST(ProjectManifestTest, WritesDeterministicLockFile) {
    fs::create_directories(fs::path("build") / "tmp");
    ProjectManifest manifest;
    manifest.name = "demo";
    manifest.version = "0.1.0";
    manifest.edition = "2026";
    manifest.dependencies["io"] = "2.3.4";
    manifest.dependencies["math"] = "1.0.0";

    const fs::path lockPath = fs::path("build") / "tmp" / "test_aym.lock";
    std::string error;
    ASSERT_TRUE(writeProjectLockFile(manifest, lockPath.string(), error)) << error;

    std::ifstream in(lockPath);
    ASSERT_TRUE(in.is_open());
    std::string contents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    EXPECT_NE(contents.find("lock_version = 1"), std::string::npos);
    EXPECT_NE(contents.find("project_name = \"demo\""), std::string::npos);
    EXPECT_NE(contents.find("manifest_checksum = \"fnv1a64:"), std::string::npos);
    EXPECT_NE(contents.find("name = \"io\""), std::string::npos);
    EXPECT_NE(contents.find("name = \"math\""), std::string::npos);
    EXPECT_NE(contents.find("checksum = \"fnv1a64:"), std::string::npos);
    EXPECT_NE(contents.find("resolved = \"2.3.4\""), std::string::npos);
    EXPECT_NE(contents.find("resolved = \"1.0.0\""), std::string::npos);
    EXPECT_LT(contents.find("name = \"io\""), contents.find("name = \"math\""));

    std::remove(lockPath.string().c_str());
}

TEST(ProjectManifestTest, WritesLockWithResolvedVersionFromRangeRequirement) {
    fs::create_directories(fs::path("build") / "tmp");
    ProjectManifest manifest;
    manifest.name = "demo";
    manifest.version = "0.1.0";
    manifest.edition = "2026";
    manifest.dependencies["math"] = "^1.4.2";

    const fs::path lockPath = fs::path("build") / "tmp" / "test_aym_resolve_range.lock";
    std::string error;
    ASSERT_TRUE(writeProjectLockFile(manifest, lockPath.string(), error)) << error;

    std::ifstream in(lockPath);
    ASSERT_TRUE(in.is_open());
    std::string contents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    EXPECT_NE(contents.find("requirement = \"^1.4.2\""), std::string::npos);
    EXPECT_NE(contents.find("resolved = \"1.4.2\""), std::string::npos);

    std::remove(lockPath.string().c_str());
}

TEST(ProjectManifestTest, LockWriterDetailedErrorForEmptyPath) {
    ProjectManifest manifest;
    manifest.name = "demo";
    manifest.version = "0.1.0";
    manifest.edition = "2026";

    ManifestError error;
    EXPECT_FALSE(writeProjectLockFileDetailed(manifest, "", error));
    EXPECT_EQ(error.code, "AYM5004");
}

TEST(ProjectManifestTest, ParsesGeneratedLockFile) {
    fs::create_directories(fs::path("build") / "tmp");

    ProjectManifest manifest;
    manifest.name = "demo";
    manifest.version = "0.1.0";
    manifest.edition = "2026";
    manifest.dependencies["io"] = "2.3.4";

    const fs::path lockPath = fs::path("build") / "tmp" / "test_parse_aym.lock";
    std::string writeError;
    ASSERT_TRUE(writeProjectLockFile(manifest, lockPath.string(), writeError)) << writeError;

    ProjectLock lock;
    std::string parseError;
    ASSERT_TRUE(parseProjectLockFile(lockPath.string(), lock, parseError)) << parseError;
    EXPECT_EQ(lock.lockVersion, 1);
    EXPECT_EQ(lock.projectName, "demo");
    EXPECT_EQ(lock.projectVersion, "0.1.0");
    EXPECT_FALSE(lock.manifestChecksum.empty());
    ASSERT_EQ(lock.dependencies.size(), 1u);
    EXPECT_EQ(lock.dependencies[0].name, "io");

    std::remove(lockPath.string().c_str());
}

TEST(ProjectManifestTest, ParseLockRejectsInvalidResolvedVersion) {
    fs::create_directories(fs::path("build") / "tmp");
    const fs::path lockPath = fs::path("build") / "tmp" / "test_bad_resolved.lock";
    {
        std::ofstream out(lockPath);
        out << "lock_version = 1\n";
        out << "project_name = \"demo\"\n";
        out << "project_version = \"0.1.0\"\n";
        out << "edition = \"2026\"\n";
        out << "manifest_checksum = \"fnv1a64:0000000000000000\"\n";
        out << "\n";
        out << "[[dependency]]\n";
        out << "name = \"math\"\n";
        out << "requirement = \"^1.0.0\"\n";
        out << "resolved = \"^1.0.0\"\n";
        out << "checksum = \"fnv1a64:0000000000000000\"\n";
    }

    ProjectLock lock;
    ManifestError error;
    EXPECT_FALSE(parseProjectLockFileDetailed(lockPath.string(), lock, error));
    EXPECT_EQ(error.code, "AYM5010");

    std::remove(lockPath.string().c_str());
}

TEST(ProjectManifestTest, ParseLockRejectsInvalidDependencyChecksumFormat) {
    fs::create_directories(fs::path("build") / "tmp");
    const fs::path lockPath = fs::path("build") / "tmp" / "test_bad_dep_checksum_format.lock";
    {
        std::ofstream out(lockPath);
        out << "lock_version = 1\n";
        out << "project_name = \"demo\"\n";
        out << "project_version = \"0.1.0\"\n";
        out << "edition = \"2026\"\n";
        out << "manifest_checksum = \"fnv1a64:0000000000000000\"\n";
        out << "\n";
        out << "[[dependency]]\n";
        out << "name = \"math\"\n";
        out << "requirement = \"^1.2.3\"\n";
        out << "resolved = \"1.2.3\"\n";
        out << "checksum = \"invalido\"\n";
    }

    ProjectLock lock;
    ManifestError error;
    EXPECT_FALSE(parseProjectLockFileDetailed(lockPath.string(), lock, error));
    EXPECT_EQ(error.code, "AYM5006");

    std::remove(lockPath.string().c_str());
}

TEST(ProjectManifestTest, ParseLockRejectsInconsistentDependencyChecksum) {
    fs::create_directories(fs::path("build") / "tmp");
    const fs::path lockPath = fs::path("build") / "tmp" / "test_bad_dep_checksum_value.lock";
    {
        std::ofstream out(lockPath);
        out << "lock_version = 1\n";
        out << "project_name = \"demo\"\n";
        out << "project_version = \"0.1.0\"\n";
        out << "edition = \"2026\"\n";
        out << "manifest_checksum = \"fnv1a64:0000000000000000\"\n";
        out << "\n";
        out << "[[dependency]]\n";
        out << "name = \"math\"\n";
        out << "requirement = \"^1.2.3\"\n";
        out << "resolved = \"1.2.3\"\n";
        out << "checksum = \"fnv1a64:0000000000000000\"\n";
    }

    ProjectLock lock;
    ManifestError error;
    EXPECT_FALSE(parseProjectLockFileDetailed(lockPath.string(), lock, error));
    EXPECT_EQ(error.code, "AYM5007");

    std::remove(lockPath.string().c_str());
}

TEST(ProjectManifestTest, ParseLockRejectsMissingManifestChecksum) {
    fs::create_directories(fs::path("build") / "tmp");
    const fs::path lockPath = fs::path("build") / "tmp" / "test_missing_manifest_checksum.lock";
    {
        std::ofstream out(lockPath);
        out << "lock_version = 1\n";
        out << "project_name = \"demo\"\n";
        out << "project_version = \"0.1.0\"\n";
        out << "edition = \"2026\"\n";
        out << "\n";
        out << "[[dependency]]\n";
        out << "name = \"math\"\n";
        out << "requirement = \"^1.0.0\"\n";
        out << "resolved = \"1.0.0\"\n";
        out << "checksum = \"fnv1a64:0000000000000000\"\n";
    }

    ProjectLock lock;
    ManifestError error;
    EXPECT_FALSE(parseProjectLockFileDetailed(lockPath.string(), lock, error));
    EXPECT_EQ(error.code, "AYM5006");

    std::remove(lockPath.string().c_str());
}

TEST(ProjectManifestTest, VerifyLockAgainstManifestDetectsMismatch) {
    fs::create_directories(fs::path("build") / "tmp");

    ProjectManifest manifest;
    manifest.name = "demo";
    manifest.version = "0.1.0";
    manifest.edition = "2026";
    manifest.dependencies["io"] = "2.3.4";

    const fs::path lockPath = fs::path("build") / "tmp" / "test_verify_mismatch.lock";
    std::string writeError;
    ASSERT_TRUE(writeProjectLockFile(manifest, lockPath.string(), writeError)) << writeError;

    ProjectLock lock;
    std::string parseError;
    ASSERT_TRUE(parseProjectLockFile(lockPath.string(), lock, parseError)) << parseError;
    ASSERT_FALSE(lock.dependencies.empty());
    lock.dependencies[0].requirement = "9.9.9";

    ManifestError error;
    EXPECT_FALSE(verifyLockAgainstManifestDetailed(manifest, lock, error));
    EXPECT_EQ(error.code, "AYM5007");

    std::remove(lockPath.string().c_str());
}

TEST(ProjectManifestTest, VerifyLockAgainstManifestDetectsIncompatibleResolvedVersion) {
    fs::create_directories(fs::path("build") / "tmp");

    ProjectManifest manifest;
    manifest.name = "demo";
    manifest.version = "0.1.0";
    manifest.edition = "2026";
    manifest.dependencies["math"] = "^1.0.0";

    const fs::path lockPath = fs::path("build") / "tmp" / "test_verify_incompatible.lock";
    std::string writeError;
    ASSERT_TRUE(writeProjectLockFile(manifest, lockPath.string(), writeError)) << writeError;

    ProjectLock lock;
    std::string parseError;
    ASSERT_TRUE(parseProjectLockFile(lockPath.string(), lock, parseError)) << parseError;
    ASSERT_FALSE(lock.dependencies.empty());
    lock.dependencies[0].resolved = "2.0.0";

    ManifestError error;
    EXPECT_FALSE(verifyLockAgainstManifestDetailed(manifest, lock, error));
    EXPECT_EQ(error.code, "AYM5010");

    std::remove(lockPath.string().c_str());
}

TEST(ProjectManifestTest, VerifyLockAgainstManifestDetectsManifestChecksumMismatch) {
    fs::create_directories(fs::path("build") / "tmp");

    ProjectManifest manifest;
    manifest.name = "demo";
    manifest.version = "0.1.0";
    manifest.edition = "2026";
    manifest.dependencies["io"] = "2.3.4";

    const fs::path lockPath = fs::path("build") / "tmp" / "test_verify_manifest_checksum.lock";
    std::string writeError;
    ASSERT_TRUE(writeProjectLockFile(manifest, lockPath.string(), writeError)) << writeError;

    ProjectLock lock;
    std::string parseError;
    ASSERT_TRUE(parseProjectLockFile(lockPath.string(), lock, parseError)) << parseError;
    lock.manifestChecksum = "fnv1a64:0000000000000000";

    ManifestError error;
    EXPECT_FALSE(verifyLockAgainstManifestDetailed(manifest, lock, error));
    EXPECT_EQ(error.code, "AYM5007");

    std::remove(lockPath.string().c_str());
}

TEST(ProjectToolTest, CreateProjectScaffoldCreatesExpectedFiles) {
    const fs::path projectDir = fs::path("build") / "tmp" / "project_tool_new_demo";
    fs::remove_all(projectDir);

    std::string error;
    ASSERT_TRUE(createProjectScaffold(projectDir, "demo_tool", error)) << error;
    EXPECT_TRUE(fs::exists(projectDir / "aym.toml"));
    EXPECT_TRUE(fs::exists(projectDir / "src" / "main.aym"));
    EXPECT_TRUE(fs::exists(projectDir / ".gitignore"));

    ProjectWorkspace workspace;
    ASSERT_TRUE(loadProjectWorkspace(projectDir / "aym.toml", workspace, error)) << error;
    EXPECT_EQ(workspace.manifest.name, "demo_tool");
    EXPECT_EQ(workspace.manifest.version, "0.1.0");

    fs::remove_all(projectDir);
}

TEST(ProjectToolTest, UpsertProjectDependencyAddsAndUpdates) {
    const fs::path projectDir = fs::path("build") / "tmp" / "project_tool_dep_demo";
    fs::remove_all(projectDir);

    std::string error;
    ASSERT_TRUE(createProjectScaffold(projectDir, "demo_dep", error)) << error;

    const fs::path manifestPath = projectDir / "aym.toml";
    ASSERT_TRUE(upsertProjectDependency(manifestPath, "math", "^1.2.3", error)) << error;

    ProjectManifest manifest;
    ASSERT_TRUE(parseProjectManifestFile(manifestPath.string(), manifest, error)) << error;
    ASSERT_EQ(manifest.dependencies.size(), 1u);
    EXPECT_EQ(manifest.dependencies["math"], "^1.2.3");

    ASSERT_TRUE(upsertProjectDependency(manifestPath, "math", "~1.2.3", error)) << error;
    ASSERT_TRUE(parseProjectManifestFile(manifestPath.string(), manifest, error)) << error;
    EXPECT_EQ(manifest.dependencies["math"], "~1.2.3");

    fs::remove_all(projectDir);
}

TEST(ProjectToolTest, LocateManifestUpwardsFindsParentManifest) {
    const fs::path projectDir = fs::path("build") / "tmp" / "project_tool_find_demo";
    fs::remove_all(projectDir);

    std::string error;
    ASSERT_TRUE(createProjectScaffold(projectDir, "demo_find", error)) << error;

    const fs::path nested = projectDir / "src" / "nested" / "deep";
    fs::create_directories(nested);

    fs::path manifestPath;
    ASSERT_TRUE(locateManifestUpwards(nested, manifestPath));
    EXPECT_EQ(fs::absolute(projectDir / "aym.toml"), fs::absolute(manifestPath));

    fs::remove_all(projectDir);
}

TEST(ProjectToolTest, GatherProjectTestFilesReturnsSortedAymFiles) {
    const fs::path projectDir = fs::path("build") / "tmp" / "project_tool_tests_demo";
    fs::remove_all(projectDir);

    std::string error;
    ASSERT_TRUE(createProjectScaffold(projectDir, "demo_tests", error)) << error;

    fs::create_directories(projectDir / "tests" / "sub");
    {
        std::ofstream out(projectDir / "tests" / "b.aym");
        out << "qallta\ntukuya\n";
    }
    {
        std::ofstream out(projectDir / "tests" / "a.aym");
        out << "qallta\ntukuya\n";
    }
    {
        std::ofstream out(projectDir / "tests" / "sub" / "z.aym");
        out << "qallta\ntukuya\n";
    }
    {
        std::ofstream out(projectDir / "tests" / "README.txt");
        out << "not a test\n";
    }

    std::vector<fs::path> tests;
    ASSERT_TRUE(gatherProjectTestFiles(projectDir, tests, error)) << error;
    ASSERT_EQ(tests.size(), 3u);
    EXPECT_LT(tests[0].string(), tests[1].string());
    EXPECT_LT(tests[1].string(), tests[2].string());

    fs::remove_all(projectDir);
}

TEST(ProjectToolTest, PrepareProjectDependencyCacheSeedsAndSyncsLocalRepo) {
    const fs::path projectDir = fs::path("build") / "tmp" / "project_tool_cache_demo";
    const fs::path repoOverride = projectDir / ".local_repo";
    const fs::path cacheOverride = projectDir / ".local_cache";
    fs::remove_all(projectDir);

    std::string error;
    ASSERT_TRUE(createProjectScaffold(projectDir, "demo_cache", error)) << error;
    ASSERT_TRUE(upsertProjectDependency(projectDir / "aym.toml", "math", "^1.2.3", error)) << error;

    ProjectWorkspace workspace;
    ASSERT_TRUE(loadProjectWorkspace(projectDir / "aym.toml", workspace, error)) << error;
    ASSERT_TRUE(writeProjectLockFile(workspace.manifest, workspace.lockPath.string(), error)) << error;

#ifdef _WIN32
    _putenv_s("AYM_PKG_REPO", repoOverride.string().c_str());
    _putenv_s("AYM_PKG_CACHE", cacheOverride.string().c_str());
#else
    setenv("AYM_PKG_REPO", repoOverride.string().c_str(), 1);
    setenv("AYM_PKG_CACHE", cacheOverride.string().c_str(), 1);
#endif

    DependencyCacheLayout layout;
    ASSERT_TRUE(prepareProjectDependencyCache(workspace, layout, error)) << error;
    EXPECT_EQ(fs::absolute(repoOverride), layout.repoRoot);
    EXPECT_EQ(fs::absolute(cacheOverride), layout.cacheRoot);

    const fs::path repoPackage = layout.repoRoot / "math" / "1.2.3";
    const fs::path cachePackage = layout.cacheRoot / "math" / "1.2.3";
    EXPECT_TRUE(fs::exists(repoPackage / "modules"));
    EXPECT_TRUE(fs::exists(cachePackage / "modules"));
    ASSERT_FALSE(layout.searchPaths.empty());

    bool foundModulesPath = false;
    for (const auto &path : layout.searchPaths) {
        if (path == fs::absolute(cachePackage / "modules")) {
            foundModulesPath = true;
            break;
        }
    }
    EXPECT_TRUE(foundModulesPath);

#ifdef _WIN32
    _putenv_s("AYM_PKG_REPO", "");
    _putenv_s("AYM_PKG_CACHE", "");
#else
    unsetenv("AYM_PKG_REPO");
    unsetenv("AYM_PKG_CACHE");
#endif

    fs::remove_all(projectDir);
}

TEST(ProjectToolTest, ResolveProjectDependencyLayoutUsesWorkspaceDefaults) {
    const fs::path projectDir = fs::path("build") / "tmp" / "project_tool_layout_demo";
    fs::remove_all(projectDir);

    std::string error;
    ASSERT_TRUE(createProjectScaffold(projectDir, "demo_layout", error)) << error;

    ProjectWorkspace workspace;
    ASSERT_TRUE(loadProjectWorkspace(projectDir / "aym.toml", workspace, error)) << error;

#ifdef _WIN32
    _putenv_s("AYM_PKG_REPO", "");
    _putenv_s("AYM_PKG_CACHE", "");
#else
    unsetenv("AYM_PKG_REPO");
    unsetenv("AYM_PKG_CACHE");
#endif

    DependencyCacheLayout layout;
    ASSERT_TRUE(resolveProjectDependencyLayout(workspace, layout, error)) << error;
    EXPECT_EQ(layout.repoRoot, fs::absolute(projectDir / ".aym" / "repo"));
    EXPECT_EQ(layout.cacheRoot, fs::absolute(projectDir / ".aym" / "cache"));

    fs::remove_all(projectDir);
}

TEST(ProjectToolTest, CleanProjectDependencyCacheRemovesRequestedStores) {
    const fs::path projectDir = fs::path("build") / "tmp" / "project_tool_clean_cache_demo";
    fs::remove_all(projectDir);

    std::string error;
    ASSERT_TRUE(createProjectScaffold(projectDir, "demo_clean", error)) << error;

    ProjectWorkspace workspace;
    ASSERT_TRUE(loadProjectWorkspace(projectDir / "aym.toml", workspace, error)) << error;

#ifdef _WIN32
    _putenv_s("AYM_PKG_REPO", "");
    _putenv_s("AYM_PKG_CACHE", "");
#else
    unsetenv("AYM_PKG_REPO");
    unsetenv("AYM_PKG_CACHE");
#endif

    const fs::path repoRoot = projectDir / ".aym" / "repo";
    const fs::path cacheRoot = projectDir / ".aym" / "cache";
    fs::create_directories(repoRoot / "math" / "1.2.3" / "modules");
    fs::create_directories(cacheRoot / "math" / "1.2.3" / "modules");
    {
        std::ofstream out(repoRoot / "math" / "1.2.3" / "modules" / "mod.aym");
        out << "qallta\ntukuya\n";
    }
    {
        std::ofstream out(cacheRoot / "math" / "1.2.3" / "modules" / "mod.aym");
        out << "qallta\ntukuya\n";
    }

    ASSERT_TRUE(cleanProjectDependencyCache(workspace, false, true, error)) << error;
    EXPECT_TRUE(fs::exists(repoRoot));
    EXPECT_FALSE(fs::exists(cacheRoot));

    ASSERT_TRUE(cleanProjectDependencyCache(workspace, true, false, error)) << error;
    EXPECT_FALSE(fs::exists(repoRoot));

    fs::remove_all(projectDir);
}

TEST(ProjectToolTest, InspectProjectDependencyStoreReportsMissingEntries) {
    const fs::path projectDir = fs::path("build") / "tmp" / "project_tool_inspect_missing_demo";
    fs::remove_all(projectDir);

    std::string error;
    ASSERT_TRUE(createProjectScaffold(projectDir, "demo_inspect_missing", error)) << error;
    ASSERT_TRUE(upsertProjectDependency(projectDir / "aym.toml", "math", "^1.2.3", error)) << error;

    ProjectWorkspace workspace;
    ASSERT_TRUE(loadProjectWorkspace(projectDir / "aym.toml", workspace, error)) << error;
    ASSERT_TRUE(writeProjectLockFile(workspace.manifest, workspace.lockPath.string(), error)) << error;

#ifdef _WIN32
    _putenv_s("AYM_PKG_REPO", "");
    _putenv_s("AYM_PKG_CACHE", "");
#else
    unsetenv("AYM_PKG_REPO");
    unsetenv("AYM_PKG_CACHE");
#endif

    DependencyStoreReport report;
    ASSERT_TRUE(inspectProjectDependencyStore(workspace, report, error)) << error;
    ASSERT_EQ(report.entries.size(), 1u);
    EXPECT_EQ(report.entries[0].name, "math");
    EXPECT_EQ(report.entries[0].resolved, "1.2.3");
    EXPECT_FALSE(report.entries[0].repoModules);
    EXPECT_FALSE(report.entries[0].cacheModules);

    fs::remove_all(projectDir);
}

TEST(ProjectToolTest, InspectProjectDependencyStoreReportsPresentEntriesAfterPrepare) {
    const fs::path projectDir = fs::path("build") / "tmp" / "project_tool_inspect_present_demo";
    fs::remove_all(projectDir);

    std::string error;
    ASSERT_TRUE(createProjectScaffold(projectDir, "demo_inspect_present", error)) << error;
    ASSERT_TRUE(upsertProjectDependency(projectDir / "aym.toml", "math", "^1.2.3", error)) << error;

    ProjectWorkspace workspace;
    ASSERT_TRUE(loadProjectWorkspace(projectDir / "aym.toml", workspace, error)) << error;
    ASSERT_TRUE(writeProjectLockFile(workspace.manifest, workspace.lockPath.string(), error)) << error;
    DependencyCacheLayout layout;
    ASSERT_TRUE(prepareProjectDependencyCache(workspace, layout, error)) << error;

#ifdef _WIN32
    _putenv_s("AYM_PKG_REPO", "");
    _putenv_s("AYM_PKG_CACHE", "");
#else
    unsetenv("AYM_PKG_REPO");
    unsetenv("AYM_PKG_CACHE");
#endif

    DependencyStoreReport report;
    ASSERT_TRUE(inspectProjectDependencyStore(workspace, report, error)) << error;
    ASSERT_EQ(report.entries.size(), 1u);
    EXPECT_EQ(report.entries[0].name, "math");
    EXPECT_EQ(report.entries[0].resolved, "1.2.3");
    EXPECT_TRUE(report.entries[0].repoModules);
    EXPECT_TRUE(report.entries[0].cacheModules);

    fs::remove_all(projectDir);
}

TEST(CodeGenTest, GeneratesAssembly) {
    std::string src = "qallta qillqa(\"ok\"); tukuya";
    Lexer lexer(src);
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto nodes = parser.parse();
    ASSERT_FALSE(parser.hasError());
    std::filesystem::create_directory("build");
    CodeGenerator cg;
#ifdef _WIN32
    bool ok = cg.generate(nodes, "build/test_output.asm", {}, {}, {}, {}, true, 0, "runtime");
#else
    bool ok = cg.generate(nodes, "build/test_output.asm", {}, {}, {}, {}, false, 0, "runtime");
#endif
    ASSERT_TRUE(ok);

    const fs::path asmPath = fs::path("build") / "test_output.asm";
    if (fs::exists(asmPath)) {
        std::ifstream in(asmPath);
        ASSERT_TRUE(in.is_open());
        std::string contents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        in.close();
        EXPECT_NE(contents.find("global main"), std::string::npos);
        EXPECT_NE(contents.find("call printf"), std::string::npos);
    } else {
#ifdef _WIN32
        EXPECT_TRUE(fs::exists(fs::path("build") / "test_output.exe"));
#else
        FAIL() << "No se genero el archivo asm esperado";
#endif
    }

    std::remove(asmPath.string().c_str());
#ifdef _WIN32
    std::remove((fs::path("build") / "test_output.obj").string().c_str());
    std::remove((fs::path("build") / "test_output.exe").string().c_str());
#else
    std::remove((fs::path("build") / "test_output.o").string().c_str());
    std::remove((fs::path("build") / "test_output").string().c_str());
#endif
}

TEST(CodeGenTest, KeepsAssemblyWhenRequested) {
    std::string src = "qallta qillqa(\"ok\"); tukuya";
    Lexer lexer(src);
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto nodes = parser.parse();
    ASSERT_FALSE(parser.hasError());
    std::filesystem::create_directory("build");

    const fs::path asmPath = fs::path("build") / "test_keep_asm.asm";
    CodeGenerator cg;
#ifdef _WIN32
    bool ok = cg.generate(nodes, asmPath.string(), {}, {}, {}, {}, true, 0, "runtime", true);
#else
    bool ok = cg.generate(nodes, asmPath.string(), {}, {}, {}, {}, false, 0, "runtime", true);
#endif
    ASSERT_TRUE(ok);
    EXPECT_TRUE(fs::exists(asmPath));

    std::remove(asmPath.string().c_str());
#ifdef _WIN32
    std::remove((fs::path("build") / "test_keep_asm.obj").string().c_str());
    std::remove((fs::path("build") / "test_keep_asm.exe").string().c_str());
#else
    std::remove((fs::path("build") / "test_keep_asm.o").string().c_str());
    std::remove((fs::path("build") / "test_keep_asm").string().c_str());
#endif
}

TEST(CodeGenTest, CompileOnlyGeneratesObjectWithoutExecutable) {
    std::string src = "qallta qillqa(\"ok\"); tukuya";
    Lexer lexer(src);
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto nodes = parser.parse();
    ASSERT_FALSE(parser.hasError());
    std::filesystem::create_directory("build");

    const fs::path asmPath = fs::path("build") / "test_compile_only.asm";
    CodeGenerator cg;
#ifdef _WIN32
    bool ok = cg.generate(nodes,
                          asmPath.string(),
                          {},
                          {},
                          {},
                          {},
                          true,
                          0,
                          "runtime",
                          true,
                          CodegenPipelineMode::CompileOnly);
#else
    bool ok = cg.generate(nodes,
                          asmPath.string(),
                          {},
                          {},
                          {},
                          {},
                          false,
                          0,
                          "runtime",
                          true,
                          CodegenPipelineMode::CompileOnly);
#endif
    ASSERT_TRUE(ok);
    EXPECT_TRUE(fs::exists(asmPath));
#ifdef _WIN32
    EXPECT_TRUE(fs::exists(fs::path("build") / "test_compile_only.obj"));
    EXPECT_FALSE(fs::exists(fs::path("build") / "test_compile_only.exe"));
#else
    EXPECT_TRUE(fs::exists(fs::path("build") / "test_compile_only.o"));
    EXPECT_FALSE(fs::exists(fs::path("build") / "test_compile_only"));
#endif

    std::remove(asmPath.string().c_str());
#ifdef _WIN32
    std::remove((fs::path("build") / "test_compile_only.obj").string().c_str());
    std::remove((fs::path("build") / "test_compile_only.exe").string().c_str());
#else
    std::remove((fs::path("build") / "test_compile_only.o").string().c_str());
    std::remove((fs::path("build") / "test_compile_only").string().c_str());
#endif
}

TEST(CodeGenTest, LinkOnlyUsesExistingObject) {
    std::string src = "qallta qillqa(\"ok\"); tukuya";
    Lexer lexer(src);
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto nodes = parser.parse();
    ASSERT_FALSE(parser.hasError());
    std::filesystem::create_directory("build");

    const fs::path asmPath = fs::path("build") / "test_link_only.asm";
    CodeGenerator cg;
#ifdef _WIN32
    bool compiled = cg.generate(nodes,
                                asmPath.string(),
                                {},
                                {},
                                {},
                                {},
                                true,
                                0,
                                "runtime",
                                true,
                                CodegenPipelineMode::CompileOnly);
#else
    bool compiled = cg.generate(nodes,
                                asmPath.string(),
                                {},
                                {},
                                {},
                                {},
                                false,
                                0,
                                "runtime",
                                true,
                                CodegenPipelineMode::CompileOnly);
#endif
    ASSERT_TRUE(compiled);

#ifdef _WIN32
    std::remove((fs::path("build") / "test_link_only.exe").string().c_str());
#else
    std::remove((fs::path("build") / "test_link_only").string().c_str());
#endif

    std::vector<std::unique_ptr<Node>> emptyNodes;
#ifdef _WIN32
    bool linked = cg.generate(emptyNodes,
                              asmPath.string(),
                              {},
                              {},
                              {},
                              {},
                              true,
                              0,
                              "runtime",
                              false,
                              CodegenPipelineMode::LinkOnly);
#else
    bool linked = cg.generate(emptyNodes,
                              asmPath.string(),
                              {},
                              {},
                              {},
                              {},
                              false,
                              0,
                              "runtime",
                              false,
                              CodegenPipelineMode::LinkOnly);
#endif
    ASSERT_TRUE(linked);
#ifdef _WIN32
    EXPECT_TRUE(fs::exists(fs::path("build") / "test_link_only.exe"));
#else
    EXPECT_TRUE(fs::exists(fs::path("build") / "test_link_only"));
#endif

    std::remove(asmPath.string().c_str());
#ifdef _WIN32
    std::remove((fs::path("build") / "test_link_only.obj").string().c_str());
    std::remove((fs::path("build") / "test_link_only.exe").string().c_str());
#else
    std::remove((fs::path("build") / "test_link_only.o").string().c_str());
    std::remove((fs::path("build") / "test_link_only").string().c_str());
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

TEST(ModuleResolverTest, LoadsOnlySelectedSymbols) {
    fs::path base = fs::current_path() / "tests" / "tmp_modules_selective";
    fs::create_directories(base / "modules");
    std::ofstream mod(base / "modules" / "util.aym");
    mod << "qallta\n";
    mod << "lurawi uno() : jakh\u00fcwi { kuttaya(1); }\n";
    mod << "lurawi paya() : jakh\u00fcwi { kuttaya(2); }\n";
    mod << "tukuya\n";
    mod.close();

    Lexer lexer("qallta apnaq(\"modules/util\", [\"paya\"]); tukuya");
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto nodes = parser.parse();
    ASSERT_FALSE(parser.hasError());

    ModuleResolver resolver(base);
    resolver.resolve(nodes, base);

    ASSERT_EQ(nodes.size(), 1u);
    auto *fn = dynamic_cast<FunctionStmt*>(nodes[0].get());
    ASSERT_NE(fn, nullptr);
    EXPECT_EQ(fn->getName(), "paya");

    fs::remove_all(base);
}

TEST(ModuleResolverTest, LoadsAliasedSymbols) {
    fs::path base = fs::current_path() / "tests" / "tmp_modules_alias";
    fs::create_directories(base / "modules");
    std::ofstream mod(base / "modules" / "util.aym");
    mod << "qallta\n";
    mod << "lurawi uno() : jakh\u00fcwi { kuttaya(1); }\n";
    mod << "lurawi paya() : jakh\u00fcwi { kuttaya(2); }\n";
    mod << "tukuya\n";
    mod.close();

    Lexer lexer("qallta apnaq(\"modules/util\", {\"uno\":\"maya\"}); tukuya");
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto nodes = parser.parse();
    ASSERT_FALSE(parser.hasError());

    ModuleResolver resolver(base);
    resolver.resolve(nodes, base);

    ASSERT_EQ(nodes.size(), 1u);
    auto *fn = dynamic_cast<FunctionStmt*>(nodes[0].get());
    ASSERT_NE(fn, nullptr);
    EXPECT_EQ(fn->getName(), "maya");

    fs::remove_all(base);
}

TEST(ModuleResolverTest, LoadsPackageModuleFromLockAndCache) {
    fs::path base = fs::current_path() / "tests" / "tmp_modules_pkg_cache";
    fs::remove_all(base);
    fs::create_directories(base / ".aym" / "cache" / "math" / "1.2.3" / "modules");

    ProjectManifest manifest;
    manifest.name = "demo";
    manifest.version = "0.1.0";
    manifest.edition = "2026";
    manifest.dependencies["math"] = "^1.2.3";
    std::string lockError;
    ASSERT_TRUE(writeProjectLockFile(manifest, (base / "aym.lock").string(), lockError)) << lockError;

    {
        std::ofstream mod(base / ".aym" / "cache" / "math" / "1.2.3" / "modules" / "util.aym");
        mod << "qallta\nlurawi desde_cache() : jakhuwi { kuttaya(7); }\ntukuya\n";
    }

    Lexer lexer("qallta apnaq(\"math/util\"); tukuya");
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto nodes = parser.parse();
    ASSERT_FALSE(parser.hasError());

    ModuleResolver resolver(base);
    resolver.resolve(nodes, base);

    ASSERT_EQ(nodes.size(), 1u);
    auto *fn = dynamic_cast<FunctionStmt*>(nodes[0].get());
    ASSERT_NE(fn, nullptr);
    EXPECT_EQ(fn->getName(), "desde_cache");

    fs::remove_all(base);
}

TEST(ModuleResolverTest, LoadsPackageModuleFromLockAndRepoFallback) {
    fs::path base = fs::current_path() / "tests" / "tmp_modules_pkg_repo";
    fs::remove_all(base);
    fs::create_directories(base / ".aym" / "repo" / "math" / "1.2.3" / "modules");

    ProjectManifest manifest;
    manifest.name = "demo";
    manifest.version = "0.1.0";
    manifest.edition = "2026";
    manifest.dependencies["math"] = "^1.2.3";
    std::string lockError;
    ASSERT_TRUE(writeProjectLockFile(manifest, (base / "aym.lock").string(), lockError)) << lockError;

    {
        std::ofstream mod(base / ".aym" / "repo" / "math" / "1.2.3" / "modules" / "util.aym");
        mod << "qallta\nlurawi desde_repo() : jakhuwi { kuttaya(9); }\ntukuya\n";
    }

    Lexer lexer("qallta apnaq(\"math/util\"); tukuya");
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto nodes = parser.parse();
    ASSERT_FALSE(parser.hasError());

    ModuleResolver resolver(base);
    resolver.resolve(nodes, base);

    ASSERT_EQ(nodes.size(), 1u);
    auto *fn = dynamic_cast<FunctionStmt*>(nodes[0].get());
    ASSERT_NE(fn, nullptr);
    EXPECT_EQ(fn->getName(), "desde_repo");

    fs::remove_all(base);
}

TEST(ModuleResolverTest, ReportsManifestLockMismatchForPackageImport) {
    fs::path base = fs::current_path() / "tests" / "tmp_modules_pkg_manifest_lock_mismatch";
    fs::remove_all(base);
    fs::create_directories(base / ".aym" / "cache" / "math" / "1.2.3" / "modules");

    {
        std::ofstream mod(base / ".aym" / "cache" / "math" / "1.2.3" / "modules" / "util.aym");
        mod << "qallta\nlurawi desde_cache() : jakhuwi { kuttaya(7); }\ntukuya\n";
    }

    ProjectManifest lockManifest;
    lockManifest.name = "demo";
    lockManifest.version = "0.1.0";
    lockManifest.edition = "2026";
    lockManifest.dependencies["math"] = "^1.2.3";
    std::string lockError;
    ASSERT_TRUE(writeProjectLockFile(lockManifest, (base / "aym.lock").string(), lockError)) << lockError;

    {
        std::ofstream manifest(base / "aym.toml");
        manifest << "[package]\n";
        manifest << "name = \"demo\"\n";
        manifest << "version = \"0.1.0\"\n";
        manifest << "edition = \"2026\"\n\n";
        manifest << "[dependencies]\n";
        manifest << "math = \"^2.0.0\"\n";
    }

    Lexer lexer("qallta apnaq(\"math/util\"); tukuya");
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto nodes = parser.parse();
    ASSERT_FALSE(parser.hasError());

    ModuleResolver resolver(base);
    try {
        resolver.resolve(nodes, base);
        FAIL() << "Expected ModuleResolverError";
    } catch (const ModuleResolverError &ex) {
        EXPECT_EQ(ex.code(), "AYM4006");
        EXPECT_NE(std::string(ex.what()).find("Manifest/lock inconsistente"), std::string::npos);
    }

    fs::remove_all(base);
}

TEST(ModuleResolverTest, LocalImportStillWorksIfManifestDoesNotDeclarePackage) {
    fs::path base = fs::current_path() / "tests" / "tmp_modules_local_with_bad_lock";
    fs::remove_all(base);
    fs::create_directories(base / "modules");

    {
        std::ofstream mod(base / "modules" / "util.aym");
        mod << "qallta\nlurawi local_ok() : jakhuwi { kuttaya(1); }\ntukuya\n";
    }

    {
        std::ofstream manifest(base / "aym.toml");
        manifest << "[package]\n";
        manifest << "name = \"demo\"\n";
        manifest << "version = \"0.1.0\"\n";
        manifest << "edition = \"2026\"\n\n";
        manifest << "[dependencies]\n";
        manifest << "io = \"^1.0.0\"\n";
    }

    {
        std::ofstream lock(base / "aym.lock");
        lock << "esto no es un lockfile valido\n";
    }

    Lexer lexer("qallta apnaq(\"modules/util\"); tukuya");
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto nodes = parser.parse();
    ASSERT_FALSE(parser.hasError());

    ModuleResolver resolver(base);
    resolver.resolve(nodes, base);

    ASSERT_EQ(nodes.size(), 1u);
    auto *fn = dynamic_cast<FunctionStmt*>(nodes[0].get());
    ASSERT_NE(fn, nullptr);
    EXPECT_EQ(fn->getName(), "local_ok");

    fs::remove_all(base);
}

TEST(ModuleResolverTest, MissingModuleReportsTypedError) {
    fs::path base = fs::current_path() / "tests" / "tmp_modules_missing";
    fs::create_directories(base);

    Lexer lexer("qallta apnaq(\"modules/no_existe\"); tukuya");
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto nodes = parser.parse();
    ASSERT_FALSE(parser.hasError());

    ModuleResolver resolver(base);
    try {
        resolver.resolve(nodes, base);
        FAIL() << "Expected ModuleResolverError";
    } catch (const ModuleResolverError &ex) {
        EXPECT_EQ(ex.code(), "AYM4001");
        EXPECT_GT(ex.line(), 0u);
        EXPECT_GT(ex.column(), 0u);
    }

    fs::remove_all(base);
}

TEST(ModuleResolverTest, MissingImportedSymbolReportsTypedError) {
    fs::path base = fs::current_path() / "tests" / "tmp_modules_missing_symbol";
    fs::create_directories(base / "modules");
    std::ofstream mod(base / "modules" / "util.aym");
    mod << "qallta\n";
    mod << "lurawi uno() : jakh\u00fcwi { kuttaya(1); }\n";
    mod << "tukuya\n";
    mod.close();

    Lexer lexer("qallta apnaq(\"modules/util\", [\"dos\"]); tukuya");
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto nodes = parser.parse();
    ASSERT_FALSE(parser.hasError());

    ModuleResolver resolver(base);
    try {
        resolver.resolve(nodes, base);
        FAIL() << "Expected ModuleResolverError";
    } catch (const ModuleResolverError &ex) {
        EXPECT_EQ(ex.code(), "AYM4005");
        EXPECT_GT(ex.line(), 0u);
        EXPECT_GT(ex.column(), 0u);
    }

    fs::remove_all(base);
}

TEST(SemanticTest, ReportsDiagnosticsWithLocation) {
    Lexer lexer("qallta yatiya jakhuwi n = 1; n = \"x\"; tukuya");
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto nodes = parser.parse();
    ASSERT_FALSE(parser.hasError());

    DiagnosticEngine diagnostics;
    SemanticAnalyzer sem;
    sem.setDiagnosticEngine(&diagnostics);
    sem.analyze(nodes);

    EXPECT_TRUE(sem.hasErrors());
    ASSERT_FALSE(diagnostics.empty());
    EXPECT_TRUE(diagnostics.hasErrors());
    EXPECT_EQ(diagnostics.all()[0].code, "AYM3003");
    EXPECT_GT(diagnostics.all()[0].line, 0u);
    EXPECT_GT(diagnostics.all()[0].column, 0u);
    EXPECT_FALSE(diagnostics.all()[0].suggestion.empty());
}

TEST(SemanticTest, RejectsWriteWithNumericArgument) {
    Lexer lexer("qallta write(1); tukuya");
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto nodes = parser.parse();
    ASSERT_FALSE(parser.hasError());

    DiagnosticEngine diagnostics;
    SemanticAnalyzer sem;
    sem.setDiagnosticEngine(&diagnostics);
    sem.analyze(nodes);

    EXPECT_TRUE(sem.hasErrors());
    ASSERT_FALSE(diagnostics.empty());

    bool foundTypeError = false;
    for (const auto &diag : diagnostics.all()) {
        if (diag.code == "AYM3003") {
            foundTypeError = true;
            break;
        }
    }
    EXPECT_TRUE(foundTypeError);
}

TEST(DiagnosticEngineTest, WritesDiagnosticsJsonFile) {
    fs::create_directories("build");
    fs::create_directories(fs::path("build") / "tmp");

    DiagnosticEngine diagnostics;
    diagnostics.error("AYM3003", "tipo incompatible", 7, 3);

    const fs::path outPath = fs::path("build") / "tmp" / "test_diagnostics.json";
    std::string writeError;
    ASSERT_TRUE(diagnostics.writeJsonFile(outPath.string(), writeError)) << writeError;

    std::ifstream in(outPath);
    ASSERT_TRUE(in.is_open());
    std::string contents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    in.close();

    EXPECT_NE(contents.find("\"diagnostics\""), std::string::npos);
    EXPECT_NE(contents.find("\"code\": \"AYM3003\""), std::string::npos);
    EXPECT_NE(contents.find("\"line\": 7"), std::string::npos);
    EXPECT_NE(contents.find("\"column\": 3"), std::string::npos);

    std::remove(outPath.string().c_str());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
