#include "ast_json.h"

#include "ast.h"
#include "../utils/fs.h"
#include <fstream>
#include <sstream>
#include <utility>

namespace aym {

namespace {

std::string indent(int level) {
    return std::string(static_cast<size_t>(level) * 2, ' ');
}

std::string escapeJson(const std::string &value) {
    std::string out;
    out.reserve(value.size());
    for (unsigned char ch : value) {
        switch (ch) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (ch < 0x20) {
                    std::ostringstream hex;
                    hex << "\\u00";
                    const char *digits = "0123456789ABCDEF";
                    hex << digits[(ch >> 4) & 0xF] << digits[ch & 0xF];
                    out += hex.str();
                } else {
                    out.push_back(static_cast<char>(ch));
                }
                break;
        }
    }
    return out;
}

std::string quote(const std::string &value) {
    return "\"" + escapeJson(value) + "\"";
}

std::string boolJson(bool value) {
    return value ? "true" : "false";
}

std::string makeArray(const std::vector<std::string> &items, int level) {
    if (items.empty()) {
        return "[]";
    }
    std::ostringstream out;
    out << "[\n";
    for (size_t i = 0; i < items.size(); ++i) {
        out << indent(level + 1) << items[i];
        if (i + 1 < items.size()) {
            out << ",";
        }
        out << "\n";
    }
    out << indent(level) << "]";
    return out.str();
}

std::string makeObject(const std::vector<std::pair<std::string, std::string>> &fields, int level) {
    std::ostringstream out;
    out << "{\n";
    for (size_t i = 0; i < fields.size(); ++i) {
        out << indent(level + 1) << quote(fields[i].first) << ": " << fields[i].second;
        if (i + 1 < fields.size()) {
            out << ",";
        }
        out << "\n";
    }
    out << indent(level) << "}";
    return out.str();
}

std::string serializeNode(const Node *node, int level);

std::string serializeNodeList(const std::vector<std::unique_ptr<Node>> &nodes, int level) {
    std::vector<std::string> values;
    values.reserve(nodes.size());
    for (const auto &node : nodes) {
        values.push_back(serializeNode(node.get(), level + 1));
    }
    return makeArray(values, level);
}

template <typename T>
std::string serializePtrList(const std::vector<std::unique_ptr<T>> &nodes, int level) {
    std::vector<std::string> values;
    values.reserve(nodes.size());
    for (const auto &node : nodes) {
        values.push_back(serializeNode(node.get(), level + 1));
    }
    return makeArray(values, level);
}

std::string serializeOptionalNode(const Node *node, int level) {
    if (!node) {
        return "null";
    }
    return serializeNode(node, level);
}

std::vector<std::pair<std::string, std::string>> baseFields(const std::string &kind, const Node *node) {
    std::vector<std::pair<std::string, std::string>> fields;
    fields.push_back({"kind", quote(kind)});
    if (node) {
        fields.push_back({"line", std::to_string(node->getLine())});
        fields.push_back({"column", std::to_string(node->getColumn())});
    }
    return fields;
}

std::string serializeParams(const std::vector<Param> &params, int level) {
    std::vector<std::string> items;
    items.reserve(params.size());
    for (const auto &param : params) {
        items.push_back(makeObject({
            {"type", quote(param.type)},
            {"name", quote(param.name)}
        }, level + 1));
    }
    return makeArray(items, level);
}

std::string serializeClassFields(const std::vector<ClassStmt::FieldDecl> &fieldsIn, int level) {
    std::vector<std::string> items;
    items.reserve(fieldsIn.size());
    for (const auto &field : fieldsIn) {
        items.push_back(makeObject({
            {"type", quote(field.type)},
            {"name", quote(field.name)},
            {"isStatic", boolJson(field.isStatic)},
            {"isPrivate", boolJson(field.isPrivate)},
            {"init", serializeOptionalNode(field.init.get(), level + 1)}
        }, level + 1));
    }
    return makeArray(items, level);
}

std::string serializeClassMethods(const std::vector<ClassStmt::MethodDecl> &methods, int level) {
    std::vector<std::string> items;
    items.reserve(methods.size());
    for (const auto &method : methods) {
        items.push_back(makeObject({
            {"name", quote(method.name)},
            {"params", serializeParams(method.params, level + 1)},
            {"returnType", quote(method.returnType)},
            {"isStatic", boolJson(method.isStatic)},
            {"isPrivate", boolJson(method.isPrivate)},
            {"body", serializeOptionalNode(method.body.get(), level + 1)}
        }, level + 1));
    }
    return makeArray(items, level);
}

std::string serializeClassCtors(const std::vector<ClassStmt::CtorDecl> &ctors, int level) {
    std::vector<std::string> items;
    items.reserve(ctors.size());
    for (const auto &ctor : ctors) {
        items.push_back(makeObject({
            {"params", serializeParams(ctor.params, level + 1)},
            {"body", serializeOptionalNode(ctor.body.get(), level + 1)}
        }, level + 1));
    }
    return makeArray(items, level);
}

std::string serializeMapEntries(const std::vector<std::pair<std::unique_ptr<Expr>, std::unique_ptr<Expr>>> &itemsIn, int level) {
    std::vector<std::string> items;
    items.reserve(itemsIn.size());
    for (const auto &entry : itemsIn) {
        items.push_back(makeObject({
            {"key", serializeOptionalNode(entry.first.get(), level + 1)},
            {"value", serializeOptionalNode(entry.second.get(), level + 1)}
        }, level + 1));
    }
    return makeArray(items, level);
}

std::string serializeSwitchCases(const std::vector<std::pair<std::unique_ptr<Expr>, std::unique_ptr<BlockStmt>>> &cases, int level) {
    std::vector<std::string> items;
    items.reserve(cases.size());
    for (const auto &entry : cases) {
        items.push_back(makeObject({
            {"value", serializeOptionalNode(entry.first.get(), level + 1)},
            {"body", serializeOptionalNode(entry.second.get(), level + 1)}
        }, level + 1));
    }
    return makeArray(items, level);
}

std::string serializeImportAliases(const std::vector<std::pair<std::string, std::string>> &aliases, int level) {
    std::vector<std::string> items;
    items.reserve(aliases.size());
    for (const auto &entry : aliases) {
        items.push_back(makeObject({
            {"from", quote(entry.first)},
            {"to", quote(entry.second)}
        }, level + 1));
    }
    return makeArray(items, level);
}

std::string serializeImportSymbols(const std::vector<std::string> &symbols, int level) {
    std::vector<std::string> items;
    items.reserve(symbols.size());
    for (const auto &symbol : symbols) {
        items.push_back(quote(symbol));
    }
    return makeArray(items, level);
}

std::string serializeCatches(const std::vector<TryStmt::CatchClause> &catches, int level) {
    std::vector<std::string> items;
    items.reserve(catches.size());
    for (const auto &entry : catches) {
        items.push_back(makeObject({
            {"typeName", quote(entry.typeName)},
            {"varName", quote(entry.varName)},
            {"block", serializeOptionalNode(entry.block.get(), level + 1)}
        }, level + 1));
    }
    return makeArray(items, level);
}

std::string serializeNode(const Node *node, int level) {
    if (!node) {
        return "null";
    }

    if (const auto *n = dynamic_cast<const NumberExpr*>(node)) {
        auto fields = baseFields("NumberExpr", n);
        fields.push_back({"value", std::to_string(n->getValue())});
        return makeObject(fields, level);
    }
    if (const auto *n = dynamic_cast<const BoolExpr*>(node)) {
        auto fields = baseFields("BoolExpr", n);
        fields.push_back({"value", boolJson(n->getValue())});
        return makeObject(fields, level);
    }
    if (const auto *n = dynamic_cast<const StringExpr*>(node)) {
        auto fields = baseFields("StringExpr", n);
        fields.push_back({"value", quote(n->getValue())});
        return makeObject(fields, level);
    }
    if (const auto *n = dynamic_cast<const VariableExpr*>(node)) {
        auto fields = baseFields("VariableExpr", n);
        fields.push_back({"name", quote(n->getName())});
        return makeObject(fields, level);
    }
    if (const auto *n = dynamic_cast<const BinaryExpr*>(node)) {
        auto fields = baseFields("BinaryExpr", n);
        fields.push_back({"op", quote(std::string(1, n->getOp()))});
        fields.push_back({"left", serializeOptionalNode(n->getLeft(), level + 1)});
        fields.push_back({"right", serializeOptionalNode(n->getRight(), level + 1)});
        return makeObject(fields, level);
    }
    if (const auto *n = dynamic_cast<const UnaryExpr*>(node)) {
        auto fields = baseFields("UnaryExpr", n);
        fields.push_back({"op", quote(std::string(1, n->getOp()))});
        fields.push_back({"expr", serializeOptionalNode(n->getExpr(), level + 1)});
        return makeObject(fields, level);
    }
    if (const auto *n = dynamic_cast<const TernaryExpr*>(node)) {
        auto fields = baseFields("TernaryExpr", n);
        fields.push_back({"condition", serializeOptionalNode(n->getCondition(), level + 1)});
        fields.push_back({"then", serializeOptionalNode(n->getThen(), level + 1)});
        fields.push_back({"else", serializeOptionalNode(n->getElse(), level + 1)});
        return makeObject(fields, level);
    }
    if (const auto *n = dynamic_cast<const IncDecExpr*>(node)) {
        auto fields = baseFields("IncDecExpr", n);
        fields.push_back({"name", quote(n->getName())});
        fields.push_back({"isIncrement", boolJson(n->increment())});
        fields.push_back({"isPrefix", boolJson(n->prefix())});
        return makeObject(fields, level);
    }
    if (const auto *n = dynamic_cast<const CallExpr*>(node)) {
        auto fields = baseFields("CallExpr", n);
        fields.push_back({"name", quote(n->getName())});
        fields.push_back({"args", serializePtrList(n->getArgs(), level + 1)});
        return makeObject(fields, level);
    }
    if (const auto *n = dynamic_cast<const MemberCallExpr*>(node)) {
        auto fields = baseFields("MemberCallExpr", n);
        fields.push_back({"base", serializeOptionalNode(n->getBase(), level + 1)});
        fields.push_back({"member", quote(n->getMember())});
        fields.push_back({"args", serializePtrList(n->getArgs(), level + 1)});
        fields.push_back({"staticCallee", quote(n->getStaticCallee())});
        fields.push_back({"resolvedType", quote(n->getResolvedType())});
        return makeObject(fields, level);
    }
    if (const auto *n = dynamic_cast<const NewExpr*>(node)) {
        auto fields = baseFields("NewExpr", n);
        fields.push_back({"name", quote(n->getName())});
        fields.push_back({"args", serializePtrList(n->getArgs(), level + 1)});
        return makeObject(fields, level);
    }
    if (const auto *n = dynamic_cast<const FunctionRefExpr*>(node)) {
        auto fields = baseFields("FunctionRefExpr", n);
        fields.push_back({"name", quote(n->getName())});
        return makeObject(fields, level);
    }
    if (const auto *n = dynamic_cast<const SuperExpr*>(node)) {
        auto fields = baseFields("SuperExpr", n);
        return makeObject(fields, level);
    }
    if (const auto *n = dynamic_cast<const ListExpr*>(node)) {
        auto fields = baseFields("ListExpr", n);
        fields.push_back({"elements", serializePtrList(n->getElements(), level + 1)});
        return makeObject(fields, level);
    }
    if (const auto *n = dynamic_cast<const MapExpr*>(node)) {
        auto fields = baseFields("MapExpr", n);
        fields.push_back({"items", serializeMapEntries(n->getItems(), level + 1)});
        return makeObject(fields, level);
    }
    if (const auto *n = dynamic_cast<const IndexExpr*>(node)) {
        auto fields = baseFields("IndexExpr", n);
        fields.push_back({"base", serializeOptionalNode(n->getBase(), level + 1)});
        fields.push_back({"index", serializeOptionalNode(n->getIndex(), level + 1)});
        return makeObject(fields, level);
    }
    if (const auto *n = dynamic_cast<const MemberExpr*>(node)) {
        auto fields = baseFields("MemberExpr", n);
        fields.push_back({"base", serializeOptionalNode(n->getBase(), level + 1)});
        fields.push_back({"member", quote(n->getMember())});
        fields.push_back({"isExceptionAccess", boolJson(n->isExceptionAccess())});
        fields.push_back({"staticField", quote(n->getStaticField())});
        fields.push_back({"resolvedType", quote(n->getResolvedType())});
        return makeObject(fields, level);
    }
    if (const auto *n = dynamic_cast<const PrintStmt*>(node)) {
        auto fields = baseFields("PrintStmt", n);
        fields.push_back({"exprs", serializePtrList(n->getExprs(), level + 1)});
        fields.push_back({"separator", serializeOptionalNode(n->getSeparator(), level + 1)});
        fields.push_back({"terminator", serializeOptionalNode(n->getTerminator(), level + 1)});
        return makeObject(fields, level);
    }
    if (const auto *n = dynamic_cast<const ExprStmt*>(node)) {
        auto fields = baseFields("ExprStmt", n);
        fields.push_back({"expr", serializeOptionalNode(n->getExpr(), level + 1)});
        return makeObject(fields, level);
    }
    if (const auto *n = dynamic_cast<const AssignStmt*>(node)) {
        auto fields = baseFields("AssignStmt", n);
        fields.push_back({"name", quote(n->getName())});
        fields.push_back({"value", serializeOptionalNode(n->getValue(), level + 1)});
        return makeObject(fields, level);
    }
    if (const auto *n = dynamic_cast<const IndexAssignStmt*>(node)) {
        auto fields = baseFields("IndexAssignStmt", n);
        fields.push_back({"base", serializeOptionalNode(n->getBase(), level + 1)});
        fields.push_back({"index", serializeOptionalNode(n->getIndex(), level + 1)});
        fields.push_back({"value", serializeOptionalNode(n->getValue(), level + 1)});
        return makeObject(fields, level);
    }
    if (const auto *n = dynamic_cast<const BlockStmt*>(node)) {
        auto fields = baseFields("BlockStmt", n);
        fields.push_back({"statements", serializePtrList(n->statements, level + 1)});
        return makeObject(fields, level);
    }
    if (const auto *n = dynamic_cast<const IfStmt*>(node)) {
        auto fields = baseFields("IfStmt", n);
        fields.push_back({"condition", serializeOptionalNode(n->getCondition(), level + 1)});
        fields.push_back({"then", serializeOptionalNode(n->getThen(), level + 1)});
        fields.push_back({"else", serializeOptionalNode(n->getElse(), level + 1)});
        return makeObject(fields, level);
    }
    if (const auto *n = dynamic_cast<const ForStmt*>(node)) {
        auto fields = baseFields("ForStmt", n);
        fields.push_back({"init", serializeOptionalNode(n->getInit(), level + 1)});
        fields.push_back({"condition", serializeOptionalNode(n->getCondition(), level + 1)});
        fields.push_back({"post", serializeOptionalNode(n->getPost(), level + 1)});
        fields.push_back({"body", serializeOptionalNode(n->getBody(), level + 1)});
        return makeObject(fields, level);
    }
    if (const auto *n = dynamic_cast<const BreakStmt*>(node)) {
        auto fields = baseFields("BreakStmt", n);
        return makeObject(fields, level);
    }
    if (const auto *n = dynamic_cast<const ContinueStmt*>(node)) {
        auto fields = baseFields("ContinueStmt", n);
        return makeObject(fields, level);
    }
    if (const auto *n = dynamic_cast<const ReturnStmt*>(node)) {
        auto fields = baseFields("ReturnStmt", n);
        fields.push_back({"value", serializeOptionalNode(n->getValue(), level + 1)});
        return makeObject(fields, level);
    }
    if (const auto *n = dynamic_cast<const VarDeclStmt*>(node)) {
        auto fields = baseFields("VarDeclStmt", n);
        fields.push_back({"type", quote(n->getType())});
        fields.push_back({"name", quote(n->getName())});
        fields.push_back({"init", serializeOptionalNode(n->getInit(), level + 1)});
        return makeObject(fields, level);
    }
    if (const auto *n = dynamic_cast<const FunctionStmt*>(node)) {
        auto fields = baseFields("FunctionStmt", n);
        fields.push_back({"name", quote(n->getName())});
        fields.push_back({"params", serializeParams(n->getParams(), level + 1)});
        fields.push_back({"returnType", quote(n->getReturnType())});
        fields.push_back({"body", serializeOptionalNode(n->getBody(), level + 1)});
        return makeObject(fields, level);
    }
    if (const auto *n = dynamic_cast<const ClassStmt*>(node)) {
        auto fields = baseFields("ClassStmt", n);
        fields.push_back({"name", quote(n->getName())});
        fields.push_back({"base", quote(n->getBase())});
        fields.push_back({"fields", serializeClassFields(n->getFields(), level + 1)});
        fields.push_back({"methods", serializeClassMethods(n->getMethods(), level + 1)});
        fields.push_back({"constructors", serializeClassCtors(n->getConstructors(), level + 1)});
        return makeObject(fields, level);
    }
    if (const auto *n = dynamic_cast<const WhileStmt*>(node)) {
        auto fields = baseFields("WhileStmt", n);
        fields.push_back({"condition", serializeOptionalNode(n->getCondition(), level + 1)});
        fields.push_back({"body", serializeOptionalNode(n->getBody(), level + 1)});
        return makeObject(fields, level);
    }
    if (const auto *n = dynamic_cast<const DoWhileStmt*>(node)) {
        auto fields = baseFields("DoWhileStmt", n);
        fields.push_back({"body", serializeOptionalNode(n->getBody(), level + 1)});
        fields.push_back({"condition", serializeOptionalNode(n->getCondition(), level + 1)});
        return makeObject(fields, level);
    }
    if (const auto *n = dynamic_cast<const SwitchStmt*>(node)) {
        auto fields = baseFields("SwitchStmt", n);
        fields.push_back({"expr", serializeOptionalNode(n->getExpr(), level + 1)});
        fields.push_back({"cases", serializeSwitchCases(n->getCases(), level + 1)});
        fields.push_back({"default", serializeOptionalNode(n->getDefault(), level + 1)});
        return makeObject(fields, level);
    }
    if (const auto *n = dynamic_cast<const ImportStmt*>(node)) {
        auto fields = baseFields("ImportStmt", n);
        fields.push_back({"module", quote(n->getModule())});
        fields.push_back({"symbols", serializeImportSymbols(n->getSymbols(), level + 1)});
        fields.push_back({"aliases", serializeImportAliases(n->getAliases(), level + 1)});
        return makeObject(fields, level);
    }
    if (const auto *n = dynamic_cast<const ThrowStmt*>(node)) {
        auto fields = baseFields("ThrowStmt", n);
        fields.push_back({"type", serializeOptionalNode(n->getType(), level + 1)});
        fields.push_back({"message", serializeOptionalNode(n->getMessage(), level + 1)});
        return makeObject(fields, level);
    }
    if (const auto *n = dynamic_cast<const TryStmt*>(node)) {
        auto fields = baseFields("TryStmt", n);
        fields.push_back({"tryBlock", serializeOptionalNode(n->getTryBlock(), level + 1)});
        fields.push_back({"catches", serializeCatches(n->getCatches(), level + 1)});
        fields.push_back({"finally", serializeOptionalNode(n->getFinallyBlock(), level + 1)});
        return makeObject(fields, level);
    }

    auto fields = baseFields("UnknownNode", node);
    return makeObject(fields, level);
}

} // namespace

bool writeAstJson(const std::vector<std::unique_ptr<Node>> &nodes,
                  const std::string &path,
                  std::string &error) {
    error.clear();
    if (path.empty()) {
        error = "Ruta invalida para --emit-ast-json";
        return false;
    }

    fs::path outputPath(path);
    if (outputPath.has_parent_path()) {
        std::error_code ec;
        fs::create_directories(outputPath.parent_path(), ec);
        if (ec) {
            error = "No se pudo crear el directorio para AST JSON: " + outputPath.parent_path().string();
            return false;
        }
    }

    std::ofstream out(path);
    if (!out.is_open()) {
        error = "No se pudo escribir AST JSON en: " + path;
        return false;
    }

    std::string json = makeObject({
        {"kind", quote("Program")},
        {"nodes", serializeNodeList(nodes, 1)}
    }, 0);
    out << json << "\n";
    return true;
}

} // namespace aym
