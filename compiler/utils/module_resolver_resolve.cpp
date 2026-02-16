#include "module_resolver.h"
#include "../ast/ast.h"
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace aym {

namespace {

std::string declarationName(const Node *node) {
    if (!node) return "";
    if (auto *fn = dynamic_cast<const FunctionStmt*>(node)) return fn->getName();
    if (auto *var = dynamic_cast<const VarDeclStmt*>(node)) return var->getName();
    if (auto *cls = dynamic_cast<const ClassStmt*>(node)) return cls->getName();
    return "";
}

void renameDeclaration(Node *node, const std::string &alias) {
    if (!node) return;
    if (auto *fn = dynamic_cast<FunctionStmt*>(node)) {
        fn->setName(alias);
        return;
    }
    if (auto *var = dynamic_cast<VarDeclStmt*>(node)) {
        var->setName(alias);
        return;
    }
    if (auto *cls = dynamic_cast<ClassStmt*>(node)) {
        cls->setName(alias);
        return;
    }
}

} // namespace

void ModuleResolver::resolve(std::vector<std::unique_ptr<Node>> &nodes,
                             const fs::path &currentDir) {
    std::vector<std::unique_ptr<Node>> resolved;
    resolved.reserve(nodes.size());
    for (auto &node : nodes) {
        if (auto *importStmt = dynamic_cast<ImportStmt*>(node.get())) {
            auto moduleNodes = load(importStmt->getModule(), currentDir,
                                    importStmt->getLine(), importStmt->getColumn());
            if (!importStmt->hasSelectiveImport() && !importStmt->hasAliasImport()) {
                for (auto &mn : moduleNodes) resolved.push_back(std::move(mn));
            } else if (importStmt->hasAliasImport()) {
                std::unordered_map<std::string,std::string> wanted;
                for (const auto &p : importStmt->getAliases()) wanted[p.first] = p.second;
                std::unordered_set<std::string> pending;
                for (const auto &p : wanted) pending.insert(p.first);
                for (auto &mn : moduleNodes) {
                    std::string name = declarationName(mn.get());
                    auto it = wanted.find(name);
                    if (it != wanted.end()) {
                        renameDeclaration(mn.get(), it->second);
                        resolved.push_back(std::move(mn));
                        pending.erase(name);
                    }
                }
                if (!pending.empty()) {
                    std::ostringstream oss;
                    oss << "[modulos] En '" << importStmt->getModule()
                        << "' no se encontro para alias: ";
                    bool first = true;
                    for (const auto &miss : pending) {
                        if (!first) oss << ", ";
                        oss << miss;
                        first = false;
                    }
                    throw ModuleResolverError("AYM4005", oss.str(),
                                              importStmt->getLine(),
                                              importStmt->getColumn());
                }
            } else {
                const auto &wanted = importStmt->getSymbols();
                std::unordered_set<std::string> pending(wanted.begin(), wanted.end());
                for (auto &mn : moduleNodes) {
                    std::string name = declarationName(mn.get());
                    if (!name.empty() && pending.erase(name) > 0) {
                        resolved.push_back(std::move(mn));
                    }
                }
                if (!pending.empty()) {
                    std::ostringstream oss;
                    oss << "[modulos] En '" << importStmt->getModule()
                        << "' no se encontro: ";
                    bool first = true;
                    for (const auto &miss : pending) {
                        if (!first) oss << ", ";
                        oss << miss;
                        first = false;
                    }
                    throw ModuleResolverError("AYM4005", oss.str(),
                                              importStmt->getLine(),
                                              importStmt->getColumn());
                }
            }
        } else {
            resolved.push_back(std::move(node));
        }
    }
    nodes = std::move(resolved);
}

} // namespace aym
