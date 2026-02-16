#include "module_resolver.h"
#include "utils.h"
#include "project_manifest.h"
#include "diagnostic.h"
#include "../lexer/lexer.h"
#include "../parser/parser.h"
#include "../ast/ast.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace aym {

ModuleResolverError::ModuleResolverError(std::string code,
                                         std::string message,
                                         size_t line,
                                         size_t column)
    : std::runtime_error(std::move(message)),
      code_(std::move(code)),
      line_(line),
      column_(column) {}

namespace {
#ifdef _WIN32
const char PATH_SEP = ';';
#else
const char PATH_SEP = ':';
#endif

std::string trim(const std::string &value) {
    size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start]))) {
        ++start;
    }
    size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }
    return value.substr(start, end - start);
}

std::string pathKey(const fs::path &path) {
    try {
        return fs::weakly_canonical(path).string();
    } catch (...) {
        return path.lexically_normal().string();
    }
}

size_t extractLocationValue(const std::string &text,
                            const std::vector<std::string> &markers) {
    for (const auto &marker : markers) {
        const size_t start = text.find(marker);
        if (start == std::string::npos) {
            continue;
        }
        size_t pos = start + marker.size();
        while (pos < text.size() && (text[pos] == ' ' || text[pos] == ':')) {
            ++pos;
        }
        size_t valueStart = pos;
        while (pos < text.size() && std::isdigit(static_cast<unsigned char>(text[pos]))) {
            ++pos;
        }
        if (pos > valueStart) {
            return static_cast<size_t>(std::strtoull(text.substr(valueStart, pos - valueStart).c_str(), nullptr, 10));
        }
    }
    return 0;
}

std::pair<size_t, size_t> extractLineColumn(const std::string &message) {
    const size_t line = extractLocationValue(message, {"line ", "linea "});
    const size_t column = extractLocationValue(message, {"column ", "columna "});
    return {line, column};
}

bool locateLockUpwards(const fs::path &start, fs::path &lockPath) {
    if (start.empty()) {
        return false;
    }
    fs::path cursor = fs::absolute(start);
    if (!fs::exists(cursor)) {
        return false;
    }
    if (fs::is_regular_file(cursor)) {
        cursor = cursor.parent_path();
    }
    while (!cursor.empty()) {
        const fs::path candidate = cursor / "aym.lock";
        if (fs::exists(candidate) && fs::is_regular_file(candidate)) {
            lockPath = candidate;
            return true;
        }
        const fs::path parent = cursor.parent_path();
        if (parent.empty() || parent == cursor) {
            break;
        }
        cursor = parent;
    }
    return false;
}

bool splitPackageImport(const std::string &normalizedModule,
                        std::string &packageName,
                        std::string &moduleSuffix) {
    packageName.clear();
    moduleSuffix.clear();
    if (normalizedModule.empty()) {
        return false;
    }
    const size_t slash = normalizedModule.find('/');
    if (slash == std::string::npos || slash == 0 || slash + 1 >= normalizedModule.size()) {
        return false;
    }
    packageName = normalizedModule.substr(0, slash);
    moduleSuffix = normalizedModule.substr(slash + 1);
    if (packageName == "." || packageName == ".." || moduleSuffix == "." || moduleSuffix == "..") {
        packageName.clear();
        moduleSuffix.clear();
        return false;
    }
    return true;
}

void appendImportVariants(const std::string &rawModule,
                          const std::string &normalizedModule,
                          std::vector<fs::path> &outVariants) {
    auto addVariant = [&](const fs::path &variant) {
        if (variant.empty()) {
            return;
        }
        const fs::path normalizedPath = variant.lexically_normal();
        if (std::find(outVariants.begin(), outVariants.end(), normalizedPath) == outVariants.end()) {
            outVariants.push_back(normalizedPath);
        }
    };

    addVariant(fs::path(rawModule));
    if (!normalizedModule.empty()) {
        addVariant(fs::path(normalizedModule));
    }

    const size_t initialSize = outVariants.size();
    for (size_t i = 0; i < initialSize; ++i) {
        fs::path withExt = outVariants[i];
        if (withExt.extension() != ".aym") {
            withExt += ".aym";
            addVariant(withExt);
        }
    }
}

void appendIfUnique(const fs::path &path,
                    std::vector<fs::path> &candidates,
                    std::unordered_set<std::string> &seen) {
    if (path.empty()) {
        return;
    }
    const std::string key = fs::absolute(path).lexically_normal().string();
    if (key.empty()) {
        return;
    }
    if (seen.insert(key).second) {
        candidates.push_back(fs::absolute(path).lexically_normal());
    }
}

std::vector<fs::path> packageStoreRoots(const fs::path &workspaceRoot) {
    std::vector<fs::path> roots;
    std::unordered_set<std::string> seen;

    const std::string envCache = trim(getEnvVar("AYM_PKG_CACHE"));
    if (!envCache.empty()) {
        appendIfUnique(fs::path(envCache), roots, seen);
    }
    appendIfUnique(workspaceRoot / ".aym" / "cache", roots, seen);

    const std::string envRepo = trim(getEnvVar("AYM_PKG_REPO"));
    if (!envRepo.empty()) {
        appendIfUnique(fs::path(envRepo), roots, seen);
    }
    appendIfUnique(workspaceRoot / ".aym" / "repo", roots, seen);

    return roots;
}

fs::path findPackageModulePath(const std::string &moduleName,
                               const std::string &normalizedModule,
                               const fs::path &currentDir,
                               size_t line,
                               size_t column) {
    std::string packageName;
    std::string moduleSuffix;
    if (!splitPackageImport(normalizedModule, packageName, moduleSuffix)) {
        return {};
    }

    fs::path lockPath;
    if (!locateLockUpwards(currentDir, lockPath)) {
        return {};
    }

    const fs::path workspaceRoot = lockPath.parent_path();
    const fs::path manifestPath = workspaceRoot / "aym.toml";

    bool requireStrictManifestLock = false;
    ProjectManifest manifest;
    if (fs::exists(manifestPath) && fs::is_regular_file(manifestPath)) {
        std::string manifestError;
        if (!parseProjectManifestFile(manifestPath.string(), manifest, manifestError)) {
            std::ostringstream oss;
            oss << "[modulos] Manifest invalido para import por paquete '" << packageName
                << "': " << manifestError;
            throw ModuleResolverError("AYM4006", oss.str(), line, column);
        }
        requireStrictManifestLock = manifest.dependencies.find(packageName) != manifest.dependencies.end();
    }

    ProjectLock lock;
    std::string lockError;
    if (!parseProjectLockFile(lockPath.string(), lock, lockError)) {
        if (!requireStrictManifestLock) {
            return {};
        }
        std::ostringstream oss;
        oss << "[modulos] Lockfile invalido para import por paquete '" << packageName
            << "': " << lockError;
        throw ModuleResolverError("AYM4006", oss.str(), line, column);
    }

    if (requireStrictManifestLock) {
        std::string verifyError;
        if (!verifyLockAgainstManifest(manifest, lock, verifyError)) {
            std::ostringstream oss;
            oss << "[modulos] Manifest/lock inconsistente para import por paquete '"
                << packageName << "': " << verifyError;
            throw ModuleResolverError("AYM4006", oss.str(), line, column);
        }
    }

    const LockDependency *match = nullptr;
    for (const auto &dep : lock.dependencies) {
        if (dep.name == packageName) {
            match = &dep;
            break;
        }
    }
    if (!match) {
        return {};
    }

    std::vector<fs::path> suffixVariants;
    appendImportVariants(moduleName.substr(packageName.size() + 1), moduleSuffix, suffixVariants);

    const auto roots = packageStoreRoots(workspaceRoot);
    for (const auto &root : roots) {
        const fs::path packageRoot = root / match->name / match->resolved;
        for (const auto &suffix : suffixVariants) {
            const fs::path inModules = packageRoot / "modules" / suffix;
            if (fs::exists(inModules)) {
                return inModules;
            }
            const fs::path direct = packageRoot / suffix;
            if (fs::exists(direct)) {
                return direct;
            }
        }
    }

    return {};
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
                                        const fs::path &currentDir,
                                        size_t line,
                                        size_t column) const {
    fs::path rawPath(moduleName);
    fs::path normalizedPath = normalized.empty() ? rawPath : fs::path(normalized);

    std::vector<fs::path> relatives;
    appendImportVariants(moduleName, normalized, relatives);

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

    if (auto found = findPackageModulePath(moduleName, normalized, currentDir, line, column); !found.empty()) {
        return found;
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
        const auto [line, column] = extractLineColumn(e.what());
        std::ostringstream oss;
        oss << "[modulos] Error lexico en '" << moduleName << "' (" << path.string()
            << "): " << e.what();
        throw ModuleResolverError("AYM4003", oss.str(), line, column);
    }
    DiagnosticEngine diagnostics;
    Parser parser(tokens, &diagnostics);
    auto nodes = parser.parse();
    if (parser.hasError()) {
        size_t line = 0;
        size_t column = 0;
        std::string detail = "error de sintaxis";
        if (!diagnostics.all().empty()) {
            const auto &diag = diagnostics.all().front();
            line = diag.line;
            column = diag.column;
            detail = diag.message;
        }
        std::ostringstream oss;
        oss << "[modulos] Error de sintaxis en '" << moduleName << "' (" << path.string()
            << "): " << detail;
        throw ModuleResolverError("AYM4004", oss.str(), line, column);
    }
    return nodes;
}

std::vector<std::unique_ptr<Node>> ModuleResolver::load(const std::string &moduleName,
                                                        const fs::path &currentDir,
                                                        size_t line,
                                                        size_t column) {
    std::string normalized = normalize(moduleName);
    fs::path modulePath = findModulePath(moduleName, normalized, currentDir, line, column);
    if (modulePath.empty()) {
        std::ostringstream oss;
        oss << "[modulos] No se pudo encontrar el modulo '" << moduleName << "'";
        throw ModuleResolverError("AYM4001", oss.str(), line, column);
    }

    std::string key = pathKey(modulePath);
    if (loadedModules.count(key)) {
        return {};
    }
    if (loadingModules.count(key)) {
        std::ostringstream oss;
        oss << "[modulos] Se detecto una importacion ciclica con '" << moduleName << "'";
        throw ModuleResolverError("AYM4002", oss.str(), line, column);
    }

    loadingModules.insert(key);
    auto nodes = parseModule(modulePath, moduleName);
    resolve(nodes, modulePath.parent_path());
    loadingModules.erase(key);
    loadedModules.insert(key);
    return nodes;
}

} // namespace aym
