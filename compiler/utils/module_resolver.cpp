#include "module_resolver.h"
#include "utils.h"
#include "../lexer/lexer.h"
#include "../parser/parser.h"
#include "../ast/ast.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>

namespace aym {

namespace {
#ifdef _WIN32
const char PATH_SEP = ';';
#else
const char PATH_SEP = ':';
#endif

std::string pathKey(const fs::path &path) {
    try {
        return fs::weakly_canonical(path).string();
    } catch (...) {
        return path.lexically_normal().string();
    }
}

}

ModuleResolver::ModuleResolver() {
    addIfUnique(fs::current_path());
    addIfUnique(fs::current_path() / "modules");
    std::string raw = getEnvVar("AYM_PATH");
    if (!raw.empty()) {
        size_t start = 0;
        while (start <= raw.size()) {
            size_t pos = raw.find(PATH_SEP, start);
            std::string segment = raw.substr(start, pos == std::string::npos ? pos : pos - start);
            if (!segment.empty()) {
                addIfUnique(fs::path(segment));
            }
            if (pos == std::string::npos) break;
            start = pos + 1;
        }
    }
}

ModuleResolver::ModuleResolver(const fs::path &entryDir) : ModuleResolver() {
    setEntryDir(entryDir);
}

void ModuleResolver::setEntryDir(const fs::path &dir) {
    if (dir.empty()) return;
    addIfUnique(dir);
    addIfUnique(dir / "modules");
}

void ModuleResolver::addSearchPath(const fs::path &path) {
    addIfUnique(path);
}

void ModuleResolver::clear() {
    loadedModules.clear();
    loadingModules.clear();
}

void ModuleResolver::addIfUnique(const fs::path &path) {
    if (path.empty()) return;
    fs::path normalized = path.lexically_normal();
    std::string key = normalized.string();
    if (key.empty()) return;
    if (searchKeys.insert(key).second) {
        searchPaths.push_back(normalized);
    }
}

bool ModuleResolver::isAbsoluteModule(const std::string &moduleName) {
    if (moduleName.empty()) return false;
    char first = moduleName[0];
    if (first == '/' || first == '\\') return true;
#ifdef _WIN32
    if (moduleName.size() >= 2 && std::isalpha(static_cast<unsigned char>(moduleName[0])) && moduleName[1] == ':')
        return true;
#endif
    return false;
}

std::string ModuleResolver::normalize(const std::string &moduleName) const {
    std::string trimmed = moduleName;
    auto notSpace = [](unsigned char ch) { return !std::isspace(ch); };
    trimmed.erase(trimmed.begin(), std::find_if(trimmed.begin(), trimmed.end(), notSpace));
    trimmed.erase(std::find_if(trimmed.rbegin(), trimmed.rend(), notSpace).base(), trimmed.end());
    std::string normalized;
    normalized.reserve(trimmed.size());
    for (char ch : trimmed) {
        if (ch == '\\') normalized.push_back('/');
        else normalized.push_back(ch);
    }
    if (normalized.size() >= 4 &&
        normalized.substr(normalized.size() - 4) == ".aym") {
        normalized.erase(normalized.size() - 4);
    }
    std::string collapsed;
    collapsed.reserve(normalized.size());
    bool prevSep = false;
    for (char ch : normalized) {
        if (ch == '/') {
            if (!prevSep) collapsed.push_back('/');
            prevSep = true;
        } else {
            collapsed.push_back(ch);
            prevSep = false;
        }
    }
    while (!collapsed.empty() && collapsed.back() == '/') collapsed.pop_back();
    return collapsed;
}

fs::path ModuleResolver::findModulePath(const std::string &moduleName,
                                        const std::string &normalized,
                                        const fs::path &currentDir) const {
    fs::path rawPath(moduleName);
    fs::path normalizedPath = normalized.empty() ? rawPath : fs::path(normalized);

    std::vector<fs::path> relatives;
    auto addVariant = [&](const fs::path &variant) {
        if (variant.empty()) return;
        fs::path norm = variant.lexically_normal();
        if (std::find(relatives.begin(), relatives.end(), norm) == relatives.end()) {
            relatives.push_back(norm);
        }
    };

    addVariant(rawPath);
    if (!normalizedPath.empty() && normalizedPath != rawPath) addVariant(normalizedPath);

    size_t initialSize = relatives.size();
    for (size_t i = 0; i < initialSize; ++i) {
        fs::path withExt = relatives[i];
        if (withExt.extension() != ".aym") {
            withExt += ".aym";
            addVariant(withExt);
        }
    }

    auto checkCandidates = [&](const fs::path &base) -> fs::path {
        if (base.empty()) return {};
        for (const auto &rel : relatives) {
            fs::path candidate = base / rel;
            if (fs::exists(candidate)) return candidate;
        }
        return {};
    };

    if (isAbsoluteModule(moduleName) || rawPath.is_absolute() || normalizedPath.is_absolute()) {
        for (const auto &rel : relatives) {
            fs::path candidate = rel;
            if (fs::exists(candidate)) return candidate;
        }
        return {};
    }

    if (auto found = checkCandidates(currentDir); !found.empty()) return found;
    if (auto found = checkCandidates(currentDir / "modules"); !found.empty()) return found;

    for (const auto &base : searchPaths) {
        if (auto found = checkCandidates(base); !found.empty()) return found;
        if (auto found = checkCandidates(base / "modules"); !found.empty()) return found;
    }

    return {};
}

std::vector<std::unique_ptr<Node>> ModuleResolver::parseModule(const fs::path &path,
                                                               const std::string &moduleName) {
    std::string source = readFile(path.string());
    Lexer lexer(source);
    std::vector<Token> tokens;
    try {
        tokens = lexer.tokenize();
    } catch (const std::runtime_error &e) {
        std::ostringstream oss;
        oss << "[modulos] Error lexicando el modulo '" << moduleName << "' en "
            << path.string() << ": " << e.what();
        throw std::runtime_error(oss.str());
    }
    Parser parser(tokens);
    auto nodes = parser.parse();
    if (parser.hasError()) {
        std::ostringstream oss;
        oss << "[modulos] El modulo '" << moduleName << "' contiene errores de sintaxis en "
            << path.string();
        throw std::runtime_error(oss.str());
    }
    return nodes;
}

std::vector<std::unique_ptr<Node>> ModuleResolver::load(const std::string &moduleName,
                                                        const fs::path &currentDir,
                                                        size_t line,
                                                        size_t column) {
    std::string normalized = normalize(moduleName);
    fs::path modulePath = findModulePath(moduleName, normalized, currentDir);
    if (modulePath.empty()) {
        std::ostringstream oss;
        oss << "[modulos] No se pudo encontrar el modulo '" << moduleName
            << "' (linea " << line << ", columna " << column << ")";
        throw std::runtime_error(oss.str());
    }

    std::string key = pathKey(modulePath);
    if (loadedModules.count(key)) {
        return {};
    }
    if (loadingModules.count(key)) {
        std::ostringstream oss;
        oss << "[modulos] Se detecto una importacion ciclica con '" << moduleName << "'";
        throw std::runtime_error(oss.str());
    }

    loadingModules.insert(key);
    auto nodes = parseModule(modulePath, moduleName);
    resolve(nodes, modulePath.parent_path());
    loadingModules.erase(key);
    loadedModules.insert(key);
    return nodes;
}

void ModuleResolver::resolve(std::vector<std::unique_ptr<Node>> &nodes,
                             const fs::path &currentDir) {
    std::vector<std::unique_ptr<Node>> resolved;
    resolved.reserve(nodes.size());
    for (auto &node : nodes) {
        if (auto *importStmt = dynamic_cast<ImportStmt*>(node.get())) {
            auto moduleNodes = load(importStmt->getModule(), currentDir,
                                    importStmt->getLine(), importStmt->getColumn());
            for (auto &mn : moduleNodes) {
                resolved.push_back(std::move(mn));
            }
        } else {
            resolved.push_back(std::move(node));
        }
    }
    nodes = std::move(resolved);
}

} // namespace aym
