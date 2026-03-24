#include "project_tool.h"
#include "semver.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <unordered_set>

namespace aym {

namespace {

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

std::string readEnvVar(const std::string &name) {
    const char *value = std::getenv(name.c_str());
    return value ? std::string(value) : "";
}

std::string stripInlineComment(const std::string &line) {
    bool inString = false;
    bool escaped = false;
    for (size_t i = 0; i < line.size(); ++i) {
        const char ch = line[i];
        if (escaped) {
            escaped = false;
            continue;
        }
        if (ch == '\\') {
            escaped = true;
            continue;
        }
        if (ch == '"') {
            inString = !inString;
            continue;
        }
        if (ch == '#' && !inString) {
            return line.substr(0, i);
        }
    }
    return line;
}

bool isValidSimpleKey(const std::string &value) {
    if (value.empty()) {
        return false;
    }
    for (char ch : value) {
        const bool ok = std::isalnum(static_cast<unsigned char>(ch)) || ch == '_' || ch == '-' || ch == '.';
        if (!ok) {
            return false;
        }
    }
    return true;
}

std::vector<std::string> readLines(const fs::path &path, bool &ok) {
    ok = false;
    std::ifstream in(path);
    if (!in.is_open()) {
        return {};
    }
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines.push_back(line);
    }
    ok = true;
    return lines;
}

bool writeLines(const fs::path &path, const std::vector<std::string> &lines, std::string &error) {
    std::ofstream out(path, std::ios::trunc);
    if (!out.is_open()) {
        error = "no se pudo escribir manifest: " + path.string();
        return false;
    }
    for (const auto &line : lines) {
        out << line << "\n";
    }
    return true;
}

bool sectionNameFromLine(const std::string &line, std::string &sectionName) {
    const std::string stripped = trim(stripInlineComment(line));
    if (stripped.size() < 3 || stripped.front() != '[' || stripped.back() != ']') {
        return false;
    }
    sectionName = trim(stripped.substr(1, stripped.size() - 2));
    return true;
}

fs::path resolveRepoRoot(const fs::path &workspaceRoot) {
    const std::string configured = trim(readEnvVar("AYM_PKG_REPO"));
    if (configured.empty()) {
        return fs::absolute(workspaceRoot / ".aym" / "repo");
    }
    return fs::absolute(fs::path(configured));
}

fs::path resolveCacheRoot(const fs::path &workspaceRoot) {
    const std::string configured = trim(readEnvVar("AYM_PKG_CACHE"));
    if (configured.empty()) {
        return fs::absolute(workspaceRoot / ".aym" / "cache");
    }
    return fs::absolute(fs::path(configured));
}

bool ensureDependencySeedInRepo(const fs::path &packageRoot,
                                const std::string &name,
                                const std::string &resolved,
                                std::string &error) {
    std::error_code ec;
    fs::create_directories(packageRoot / "modules", ec);
    if (ec) {
        error = "no se pudo preparar repositorio local para dependencia '" + name +
                "'@" + resolved + ": " + packageRoot.string();
        return false;
    }

    const fs::path readmePath = packageRoot / "README.txt";
    if (!fs::exists(readmePath)) {
        std::ofstream out(readmePath);
        if (!out.is_open()) {
            error = "no se pudo escribir metadata de repositorio local: " + readmePath.string();
            return false;
        }
        out << "Paquete local de AymaraLang\n";
        out << "name = " << name << "\n";
        out << "resolved = " << resolved << "\n";
        out << "Coloca modulos .aym dentro de ./modules\n";
    }

    return true;
}

bool syncDependencyToCache(const fs::path &repoPackageRoot,
                           const fs::path &cachePackageRoot,
                           std::string &error) {
    std::error_code ec;
    fs::remove_all(cachePackageRoot, ec);
    if (ec) {
        error = "no se pudo limpiar cache local: " + cachePackageRoot.string();
        return false;
    }

    fs::create_directories(cachePackageRoot.parent_path(), ec);
    if (ec) {
        error = "no se pudo crear directorio de cache local: " + cachePackageRoot.parent_path().string();
        return false;
    }

    fs::copy(repoPackageRoot,
             cachePackageRoot,
             fs::copy_options::recursive | fs::copy_options::overwrite_existing,
             ec);
    if (ec) {
        error = "no se pudo sincronizar dependencia a cache local: " + repoPackageRoot.string();
        return false;
    }
    return true;
}

void appendUniquePath(const fs::path &path,
                      std::vector<fs::path> &paths,
                      std::unordered_set<std::string> &seen) {
    if (path.empty()) {
        return;
    }
    const fs::path absolute = fs::absolute(path).lexically_normal();
    const std::string key = absolute.string();
    if (key.empty()) {
        return;
    }
    if (seen.insert(key).second) {
        paths.push_back(absolute);
    }
}

} // namespace

bool locateManifestUpwards(const fs::path &start, fs::path &manifestPath) {
    fs::path cursor = start.empty() ? fs::current_path() : start;
    if (!fs::exists(cursor)) {
        return false;
    }
    if (fs::is_regular_file(cursor)) {
        if (cursor.filename() == "aym.toml") {
            manifestPath = fs::absolute(cursor);
            return true;
        }
        cursor = cursor.parent_path();
    }

    cursor = fs::absolute(cursor);
    while (true) {
        const fs::path candidate = cursor / "aym.toml";
        if (fs::exists(candidate) && fs::is_regular_file(candidate)) {
            manifestPath = candidate;
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

bool loadProjectWorkspace(const fs::path &manifestHint, ProjectWorkspace &workspace, std::string &error) {
    error.clear();
    workspace = {};

    fs::path manifestPath;
    if (manifestHint.empty()) {
        if (!locateManifestUpwards(fs::current_path(), manifestPath)) {
            error = "no se encontro aym.toml en el directorio actual ni en padres";
            return false;
        }
    } else {
        manifestPath = manifestHint;
    }

    manifestPath = fs::absolute(manifestPath);
    if (!fs::exists(manifestPath) || !fs::is_regular_file(manifestPath)) {
        error = "manifest no encontrado: " + manifestPath.string();
        return false;
    }

    ProjectManifest manifest;
    if (!parseProjectManifestFile(manifestPath.string(), manifest, error)) {
        return false;
    }

    workspace.rootDir = manifestPath.parent_path();
    workspace.manifestPath = manifestPath;
    workspace.lockPath = fs::path(defaultLockPathForManifest(manifestPath.string()));
    workspace.sourcePath = workspace.rootDir / "src" / "main.aym";
    workspace.buildDir = workspace.rootDir / "build";
    workspace.outputBasePath = workspace.buildDir / manifest.name;
    workspace.manifest = manifest;
    return true;
}

bool createProjectScaffold(const fs::path &projectDir, const std::string &packageName, std::string &error) {
    error.clear();
    if (!isValidSimpleKey(packageName)) {
        error = "nombre de proyecto invalido: usa solo letras, numeros, _ . -";
        return false;
    }

    const fs::path absoluteProject = fs::absolute(projectDir);
    if (fs::exists(absoluteProject)) {
        error = "el directorio ya existe: " + absoluteProject.string();
        return false;
    }

    std::error_code ec;
    fs::create_directories(absoluteProject / "src", ec);
    if (ec) {
        error = "no se pudo crear el directorio del proyecto: " + absoluteProject.string();
        return false;
    }

    const fs::path manifestPath = absoluteProject / "aym.toml";
    const fs::path sourcePath = absoluteProject / "src" / "main.aym";
    const fs::path gitignorePath = absoluteProject / ".gitignore";

    {
        std::ofstream out(manifestPath);
        if (!out.is_open()) {
            error = "no se pudo escribir manifest: " + manifestPath.string();
            return false;
        }
        out << "[package]\n";
        out << "name = \"" << packageName << "\"\n";
        out << "version = \"0.1.0\"\n";
        out << "edition = \"2026\"\n";
        out << "\n";
        out << "[dependencies]\n";
    }

    {
        std::ofstream out(sourcePath);
        if (!out.is_open()) {
            error = "no se pudo escribir archivo fuente inicial: " + sourcePath.string();
            return false;
        }
        out << "qallta\n";
        out << "qillqa(\"Kamisaraki AymaraLang\");\n";
        out << "tukuya\n";
    }

    {
        std::ofstream out(gitignorePath);
        if (out.is_open()) {
            out << "build/\n";
        }
    }

    ProjectManifest manifest;
    if (!parseProjectManifestFile(manifestPath.string(), manifest, error)) {
        return false;
    }
    return true;
}

bool upsertProjectDependency(const fs::path &manifestPath,
                             const std::string &dependency,
                             const std::string &requirement,
                             std::string &error) {
    error.clear();
    if (!isValidSimpleKey(dependency)) {
        error = "nombre de dependencia invalido: usa solo letras, numeros, _ . -";
        return false;
    }

    SemverRequirement parsedRequirement;
    std::string requirementError;
    if (!parseSemverRequirement(requirement, parsedRequirement, requirementError)) {
        error = "requirement invalido: " + requirementError;
        return false;
    }

    bool readOk = false;
    std::vector<std::string> lines = readLines(manifestPath, readOk);
    if (!readOk) {
        error = "no se pudo leer manifest: " + manifestPath.string();
        return false;
    }

    const std::string newLine = dependency + " = \"" + requirement + "\"";

    size_t depSectionHeader = static_cast<size_t>(-1);
    size_t depSectionEnd = lines.size();
    for (size_t i = 0; i < lines.size(); ++i) {
        std::string sectionName;
        if (!sectionNameFromLine(lines[i], sectionName)) {
            continue;
        }
        if (sectionName == "dependencies") {
            depSectionHeader = i;
            depSectionEnd = lines.size();
            for (size_t j = i + 1; j < lines.size(); ++j) {
                std::string nextSection;
                if (sectionNameFromLine(lines[j], nextSection)) {
                    depSectionEnd = j;
                    break;
                }
            }
            break;
        }
    }

    bool replaced = false;
    if (depSectionHeader != static_cast<size_t>(-1)) {
        for (size_t i = depSectionHeader + 1; i < depSectionEnd; ++i) {
            const std::string stripped = trim(stripInlineComment(lines[i]));
            if (stripped.empty()) {
                continue;
            }
            const size_t eq = stripped.find('=');
            if (eq == std::string::npos) {
                continue;
            }
            const std::string key = trim(stripped.substr(0, eq));
            if (key == dependency) {
                lines[i] = newLine;
                replaced = true;
                break;
            }
        }
    }

    if (depSectionHeader == static_cast<size_t>(-1)) {
        if (!lines.empty() && !trim(lines.back()).empty()) {
            lines.push_back("");
        }
        lines.push_back("[dependencies]");
        lines.push_back(newLine);
    } else if (!replaced) {
        lines.insert(lines.begin() + static_cast<std::ptrdiff_t>(depSectionEnd), newLine);
    }

    if (!writeLines(manifestPath, lines, error)) {
        return false;
    }

    ProjectManifest manifest;
    if (!parseProjectManifestFile(manifestPath.string(), manifest, error)) {
        return false;
    }
    return true;
}

bool resolveProjectDependencyLayout(const ProjectWorkspace &workspace,
                                    DependencyCacheLayout &layout,
                                    std::string &error) {
    error.clear();
    layout = {};
    if (workspace.rootDir.empty()) {
        error = "workspace invalido: directorio raiz vacio";
        return false;
    }

    layout.repoRoot = resolveRepoRoot(workspace.rootDir);
    layout.cacheRoot = resolveCacheRoot(workspace.rootDir);
    return true;
}

bool inspectProjectDependencyStore(const ProjectWorkspace &workspace,
                                   DependencyStoreReport &report,
                                   std::string &error) {
    error.clear();
    report = {};

    if (!resolveProjectDependencyLayout(workspace, report.layout, error)) {
        return false;
    }

    ProjectLock lock;
    if (!parseProjectLockFile(workspace.lockPath.string(), lock, error)) {
        error = "lockfile invalido para inspeccion de store: " + error;
        return false;
    }
    if (!verifyLockAgainstManifest(workspace.manifest, lock, error)) {
        error = "manifest/lock inconsistente para inspeccion de store: " + error;
        return false;
    }

    for (const auto &dep : lock.dependencies) {
        DependencyStoreEntry entry;
        entry.name = dep.name;
        entry.resolved = dep.resolved;

        const fs::path repoModules = report.layout.repoRoot / dep.name / dep.resolved / "modules";
        const fs::path cacheModules = report.layout.cacheRoot / dep.name / dep.resolved / "modules";
        entry.repoModules = fs::exists(repoModules) && fs::is_directory(repoModules);
        entry.cacheModules = fs::exists(cacheModules) && fs::is_directory(cacheModules);
        report.entries.push_back(entry);
    }

    return true;
}

bool prepareProjectDependencyCache(const ProjectWorkspace &workspace,
                                   DependencyCacheLayout &layout,
                                   std::string &error) {
    error.clear();
    if (!resolveProjectDependencyLayout(workspace, layout, error)) {
        return false;
    }

    std::error_code ec;
    fs::create_directories(layout.repoRoot, ec);
    if (ec) {
        error = "no se pudo crear directorio de repositorio local: " + layout.repoRoot.string();
        return false;
    }

    fs::create_directories(layout.cacheRoot, ec);
    if (ec) {
        error = "no se pudo crear directorio de cache local: " + layout.cacheRoot.string();
        return false;
    }

    ProjectLock lock;
    if (!parseProjectLockFile(workspace.lockPath.string(), lock, error)) {
        error = "lockfile invalido para cache local: " + error;
        return false;
    }
    if (!verifyLockAgainstManifest(workspace.manifest, lock, error)) {
        error = "manifest/lock inconsistente para cache local: " + error;
        return false;
    }

    std::unordered_set<std::string> seenPaths;
    for (const auto &dep : lock.dependencies) {
        const fs::path repoPackageRoot = layout.repoRoot / dep.name / dep.resolved;
        if (!ensureDependencySeedInRepo(repoPackageRoot, dep.name, dep.resolved, error)) {
            return false;
        }

        const fs::path cachePackageRoot = layout.cacheRoot / dep.name / dep.resolved;
        if (!syncDependencyToCache(repoPackageRoot, cachePackageRoot, error)) {
            return false;
        }

        const fs::path cacheModules = cachePackageRoot / "modules";
        if (fs::exists(cacheModules) && fs::is_directory(cacheModules)) {
            appendUniquePath(cacheModules, layout.searchPaths, seenPaths);
        }
        appendUniquePath(cachePackageRoot, layout.searchPaths, seenPaths);
    }

    return true;
}

bool cleanProjectDependencyCache(const ProjectWorkspace &workspace,
                                 bool cleanRepo,
                                 bool cleanCache,
                                 std::string &error) {
    error.clear();
    if (!cleanRepo && !cleanCache) {
        return true;
    }

    DependencyCacheLayout layout;
    if (!resolveProjectDependencyLayout(workspace, layout, error)) {
        return false;
    }

    std::error_code ec;
    if (cleanCache && fs::exists(layout.cacheRoot)) {
        fs::remove_all(layout.cacheRoot, ec);
        if (ec) {
            error = "no se pudo limpiar cache local: " + layout.cacheRoot.string();
            return false;
        }
    }

    if (cleanRepo && fs::exists(layout.repoRoot)) {
        fs::remove_all(layout.repoRoot, ec);
        if (ec) {
            error = "no se pudo limpiar repositorio local: " + layout.repoRoot.string();
            return false;
        }
    }

    return true;
}

bool gatherProjectTestFiles(const fs::path &projectRoot, std::vector<fs::path> &testFiles, std::string &error) {
    error.clear();
    testFiles.clear();

    const fs::path testsDir = projectRoot / "tests";
    if (!fs::exists(testsDir)) {
        return true;
    }
    if (!fs::is_directory(testsDir)) {
        error = "la ruta tests existe pero no es directorio: " + testsDir.string();
        return false;
    }

    for (const auto &entry : fs::recursive_directory_iterator(testsDir)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        const fs::path file = entry.path();
        if (file.extension() == ".aym") {
            testFiles.push_back(file);
        }
    }

    std::sort(testFiles.begin(), testFiles.end());
    return true;
}

} // namespace aym
