#include "compiler/utils/project_tool.h"
#include "compiler/utils/fs.h"

#include <cctype>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#else
#include <climits>
#include <unistd.h>
#endif

namespace {

std::string helpText() {
    return "Uso: aym <comando> [opciones]\n"
           "\n"
           "Comandos:\n"
           "  new <nombre> [--path <dir>]        Crea un proyecto nuevo\n"
           "  build [--manifest <ruta>] [--check] [--doctor|--doctor-fix] [--frozen] Compila el proyecto actual\n"
           "  run [--manifest <ruta>] [--doctor|--doctor-fix] [--frozen] Compila y ejecuta el proyecto\n"
           "  test [--manifest <ruta>] [--doctor|--doctor-fix] [--frozen] Valida tests/*.aym con --check\n"
           "  lock <sync|check> [--manifest <ruta>] [--frozen] Gestiona lockfile del proyecto\n"
           "  cache <status|sync|clean|doctor> [opciones] Gestiona cache/repo local\n"
           "  add <dep> [requirement] [--manifest <ruta>] [--frozen] Agrega dependencia\n"
           "\n"
           "Opciones globales:\n"
           "  -h, --help                          Muestra esta ayuda\n"
           "\n"
           "Notas:\n"
           "  - requirement por defecto en 'add': 1.0.0\n"
           "  - Si no se pasa --manifest, busca aym.toml en el directorio actual o padres.\n"
           "  - --frozen evita regenerar aym.lock y exige consistencia exacta manifest-lock.\n"
           "  - En 'add', --frozen rechaza cambios (modo solo lectura).\n"
           "  - Cache local: ./.aym/cache y repo local: ./.aym/repo (override con AYM_PKG_CACHE/AYM_PKG_REPO).\n"
           "\n"
           "Ejemplos:\n"
           "  aym new demo\n"
           "  aym build\n"
           "  aym run\n"
           "  aym build --frozen\n"
           "  aym lock check\n"
           "  aym lock sync --frozen\n"
           "  aym cache status\n"
           "  aym cache sync --frozen\n"
           "  aym cache sync\n"
           "  aym cache clean --all\n"
           "  aym cache doctor --fix\n"
           "  aym add math ^1.2.0\n"
           "  aym test\n";
}

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

std::string getEnvVar(const std::string &name) {
    const char *value = std::getenv(name.c_str());
    return value ? std::string(value) : "";
}

fs::path executableDirectory() {
    fs::path result;
#ifdef _WIN32
    std::vector<char> buffer(MAX_PATH);
    const DWORD size = GetModuleFileNameA(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (size > 0) {
        result = fs::path(std::string(buffer.data(), size)).parent_path();
    }
#elif defined(__APPLE__)
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::vector<char> buffer(size);
    if (_NSGetExecutablePath(buffer.data(), &size) == 0) {
        result = fs::path(buffer.data()).parent_path();
    }
#else
    std::vector<char> buffer(PATH_MAX);
    const ssize_t len = readlink("/proc/self/exe", buffer.data(), buffer.size() - 1);
    if (len > 0) {
        buffer[static_cast<size_t>(len)] = '\0';
        result = fs::path(buffer.data()).parent_path();
    }
#endif
    if (result.empty()) {
        result = fs::current_path();
    }
    return result;
}

std::string executableName(const std::string &base) {
#ifdef _WIN32
    return base + ".exe";
#else
    return base;
#endif
}

fs::path resolveAymcPath() {
    const std::string envCompiler = trim(getEnvVar("AYM_COMPILER"));
    if (!envCompiler.empty()) {
        return fs::path(envCompiler);
    }

    const fs::path exeDir = executableDirectory();
    const std::string compilerName = executableName("aymc");
    const std::vector<fs::path> candidates = {
        exeDir / compilerName,
        exeDir.parent_path() / compilerName,
        fs::current_path() / "build" / "bin" / "Release" / compilerName,
        fs::current_path() / "build" / "bin" / compilerName
    };
    for (const auto &candidate : candidates) {
        if (fs::exists(candidate) && fs::is_regular_file(candidate)) {
            return candidate;
        }
    }
    return fs::path(executableName("aymc"));
}

std::string quoteShellArg(const std::string &value) {
    std::string out = "\"";
    for (char ch : value) {
        if (ch == '"') {
            out += "\"\"";
            continue;
        }
        out.push_back(ch);
    }
    out.push_back('"');
    return out;
}

std::string joinCommand(const std::vector<std::string> &args) {
    std::string command;
    bool first = true;
    for (const auto &arg : args) {
        if (!first) {
            command.push_back(' ');
        }
        first = false;
        command += quoteShellArg(arg);
    }
    return command;
}

bool runCommand(const std::vector<std::string> &args, std::string &error) {
    error.clear();
    if (args.empty()) {
        error = "comando vacio";
        return false;
    }
#ifdef _WIN32
    std::vector<const char *> argvRaw;
    argvRaw.reserve(args.size() + 1);
    for (const auto &arg : args) {
        argvRaw.push_back(arg.c_str());
    }
    argvRaw.push_back(nullptr);

    const intptr_t rc = _spawnv(_P_WAIT, args.front().c_str(), argvRaw.data());
    if (rc == -1) {
        error = "fallo comando: " + args.front();
        return false;
    }
    if (rc != 0) {
        error = "fallo comando (codigo " + std::to_string(static_cast<long long>(rc)) + "): " + joinCommand(args);
        return false;
    }
    return true;
#else
    const std::string command = joinCommand(args);
    const int rc = std::system(command.c_str());
    if (rc != 0) {
        error = "fallo comando (codigo " + std::to_string(rc) + "): " + command;
        return false;
    }
    return true;
#endif
}

bool syncManifestAndLock(const fs::path &aymcPath, const fs::path &manifestPath, std::string &error) {
    return runCommand(
        {aymcPath.string(), "--check-manifest", "--emit-lock", "--manifest", manifestPath.string()},
        error
    );
}

bool validateLockConsistency(const fs::path &aymcPath,
                             const fs::path &manifestPath,
                             const fs::path &lockPath,
                             std::string &error) {
    return runCommand(
        {
            aymcPath.string(),
            "--check-manifest",
            "--check-lock",
            "--manifest",
            manifestPath.string(),
            "--lock",
            lockPath.string()
        },
        error
    );
}

struct ManifestLockFlowOptions {
    bool syncIfMutable = true;
    bool prefixFrozenError = true;
    bool printFrozenNotice = false;
    std::string frozenNotice = "Modo frozen activo: lockfile no regenerado.";
};

bool ensureManifestLockFlow(const fs::path &aymcPath,
                            const aym::ProjectWorkspace &workspace,
                            bool frozen,
                            const ManifestLockFlowOptions &options,
                            std::string &error) {
    error.clear();

    if (options.syncIfMutable && !frozen) {
        if (!syncManifestAndLock(aymcPath, workspace.manifestPath, error)) {
            return false;
        }
    }

    if (!validateLockConsistency(aymcPath, workspace.manifestPath, workspace.lockPath, error)) {
        if (frozen && options.prefixFrozenError) {
            error = "modo --frozen: " + error;
        }
        return false;
    }

    if (frozen && options.printFrozenNotice) {
        std::cout << "[aym] " << options.frozenNotice << "\n";
    }
    return true;
}

char envPathSeparator() {
#ifdef _WIN32
    return ';';
#else
    return ':';
#endif
}

bool setEnvVar(const std::string &name, const std::string &value, std::string &error) {
    error.clear();
#ifdef _WIN32
    if (_putenv_s(name.c_str(), value.c_str()) != 0) {
        error = "no se pudo actualizar variable de entorno: " + name;
        return false;
    }
#else
    if (setenv(name.c_str(), value.c_str(), 1) != 0) {
        error = "no se pudo actualizar variable de entorno: " + name;
        return false;
    }
#endif
    return true;
}

void appendPathTokens(const std::string &text,
                      char separator,
                      std::vector<std::string> &tokens,
                      std::unordered_set<std::string> &seen) {
    size_t start = 0;
    while (start <= text.size()) {
        size_t pos = text.find(separator, start);
        const std::string token = trim(text.substr(start, pos == std::string::npos ? pos : pos - start));
        if (!token.empty() && seen.insert(token).second) {
            tokens.push_back(token);
        }
        if (pos == std::string::npos) {
            break;
        }
        start = pos + 1;
    }
}

bool configureDependencyEnvironment(const aym::ProjectWorkspace &workspace, std::string &error) {
    aym::DependencyCacheLayout cacheLayout;
    if (!aym::prepareProjectDependencyCache(workspace, cacheLayout, error)) {
        return false;
    }

    std::vector<std::string> pathTokens;
    std::unordered_set<std::string> seenTokens;
    for (const auto &path : cacheLayout.searchPaths) {
        const std::string value = fs::absolute(path).lexically_normal().string();
        if (!value.empty() && seenTokens.insert(value).second) {
            pathTokens.push_back(value);
        }
    }

    const char separator = envPathSeparator();
    appendPathTokens(getEnvVar("AYM_PATH"), separator, pathTokens, seenTokens);

    std::string merged;
    for (size_t i = 0; i < pathTokens.size(); ++i) {
        if (i > 0) {
            merged.push_back(separator);
        }
        merged += pathTokens[i];
    }
    if (!setEnvVar("AYM_PATH", merged, error)) {
        return false;
    }

    return true;
}

bool loadVerifiedLock(const aym::ProjectWorkspace &workspace,
                      aym::ProjectLock &lock,
                      std::string &error) {
    error.clear();
    if (!fs::exists(workspace.lockPath) || !fs::is_regular_file(workspace.lockPath)) {
        error = "lockfile no encontrado: " + workspace.lockPath.string();
        return false;
    }
    if (!aym::parseProjectLockFile(workspace.lockPath.string(), lock, error)) {
        return false;
    }
    if (!aym::verifyLockAgainstManifest(workspace.manifest, lock, error)) {
        return false;
    }
    return true;
}

bool runDependencyDoctor(const aym::ProjectWorkspace &workspace,
                         bool fix,
                         std::string &error) {
    error.clear();

    auto collectMissing = [](const aym::DependencyStoreReport &report) {
        std::vector<std::string> missing;
        for (const auto &entry : report.entries) {
            if (entry.repoModules && entry.cacheModules) {
                continue;
            }
            std::string item = entry.name + "@" + entry.resolved + " repo=" +
                               (entry.repoModules ? "ok" : "faltante") +
                               " cache=" + (entry.cacheModules ? "ok" : "faltante");
            missing.push_back(item);
        }
        return missing;
    };

    aym::DependencyStoreReport report;
    std::string inspectError;
    if (!aym::inspectProjectDependencyStore(workspace, report, inspectError)) {
        if (!fix) {
            error = "doctor: " + inspectError;
            return false;
        }
        aym::DependencyCacheLayout repairedLayout;
        if (!aym::prepareProjectDependencyCache(workspace, repairedLayout, error)) {
            return false;
        }
        if (!aym::inspectProjectDependencyStore(workspace, report, inspectError)) {
            error = "doctor tras fix: " + inspectError;
            return false;
        }
    }

    std::vector<std::string> missing = collectMissing(report);
    if (!missing.empty() && fix) {
        aym::DependencyCacheLayout repairedLayout;
        if (!aym::prepareProjectDependencyCache(workspace, repairedLayout, error)) {
            return false;
        }
        if (!aym::inspectProjectDependencyStore(workspace, report, inspectError)) {
            error = "doctor tras fix: " + inspectError;
            return false;
        }
        missing = collectMissing(report);
    }

    if (!missing.empty()) {
        error = "doctor detecto store inconsistente: " + missing.front();
        if (missing.size() > 1) {
            error += " (+" + std::to_string(missing.size() - 1) + " casos)";
        }
        return false;
    }

    return true;
}

bool buildProject(const fs::path &aymcPath,
                  const aym::ProjectWorkspace &workspace,
                  bool checkOnly,
                  std::string &error) {
    if (!fs::exists(workspace.sourcePath)) {
        error = "archivo de entrada no encontrado: " + workspace.sourcePath.string();
        return false;
    }

    std::error_code ec;
    fs::create_directories(workspace.buildDir, ec);
    if (ec) {
        error = "no se pudo crear directorio de build: " + workspace.buildDir.string();
        return false;
    }

    std::vector<std::string> args = {aymcPath.string()};
    if (checkOnly) {
        args.push_back("--check");
    } else {
        args.push_back("-o");
        args.push_back(workspace.outputBasePath.string());
    }
    args.push_back(workspace.sourcePath.string());
    return runCommand(args, error);
}

fs::path outputBinaryPath(const aym::ProjectWorkspace &workspace) {
    fs::path output = workspace.outputBasePath;
#ifdef _WIN32
    output += ".exe";
#endif
    return output;
}

bool parseManifestOption(int argc, char **argv, int &i, fs::path &manifestPath, std::string &error) {
    const std::string arg = argv[i];
    const std::string prefix = "--manifest=";
    if (arg == "--manifest") {
        if (i + 1 >= argc) {
            error = "la opcion --manifest requiere una ruta";
            return false;
        }
        manifestPath = argv[++i];
        if (manifestPath.empty()) {
            error = "la opcion --manifest requiere una ruta valida";
            return false;
        }
        return true;
    }
    if (arg.rfind(prefix, 0) == 0) {
        manifestPath = arg.substr(prefix.size());
        if (manifestPath.empty()) {
            error = "la opcion --manifest requiere una ruta valida";
            return false;
        }
        return true;
    }
    return false;
}

} // namespace

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cout << helpText();
        return 1;
    }

    const std::string command = argv[1];
    if (command == "-h" || command == "--help" || command == "help") {
        std::cout << helpText();
        return 0;
    }

    const fs::path aymcPath = resolveAymcPath();

    if (command == "new") {
        std::string name;
        fs::path basePath = fs::current_path();
        for (int i = 2; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--path") {
                if (i + 1 >= argc) {
                    std::cerr << "[aym] --path requiere una ruta\n";
                    return 1;
                }
                basePath = argv[++i];
                continue;
            }
            const std::string pathPrefix = "--path=";
            if (arg.rfind(pathPrefix, 0) == 0) {
                basePath = arg.substr(pathPrefix.size());
                if (basePath.empty()) {
                    std::cerr << "[aym] --path requiere una ruta valida\n";
                    return 1;
                }
                continue;
            }
            if (!arg.empty() && arg[0] == '-') {
                std::cerr << "[aym] opcion no reconocida: " << arg << "\n";
                return 1;
            }
            if (!name.empty()) {
                std::cerr << "[aym] usa: aym new <nombre> [--path <dir>]\n";
                return 1;
            }
            name = arg;
        }
        if (name.empty()) {
            std::cerr << "[aym] usa: aym new <nombre> [--path <dir>]\n";
            return 1;
        }

        const fs::path projectDir = fs::absolute(basePath / name);
        std::string error;
        if (!aym::createProjectScaffold(projectDir, name, error)) {
            std::cerr << "[aym] " << error << "\n";
            return 1;
        }
        std::cout << "[aym] Proyecto creado en: " << projectDir.string() << "\n";
        std::cout << "[aym] Siguiente paso: cd " << projectDir.string() << " && aym build\n";
        return 0;
    }

    if (command == "build") {
        fs::path manifestPath;
        bool checkOnly = false;
        bool doctorCheck = false;
        bool doctorFix = false;
        bool frozen = false;
        for (int i = 2; i < argc; ++i) {
            std::string error;
            if (parseManifestOption(argc, argv, i, manifestPath, error)) {
                continue;
            }
            const std::string arg = argv[i];
            if (!error.empty()) {
                std::cerr << "[aym] " << error << "\n";
                return 1;
            }
            if (arg == "--check") {
                checkOnly = true;
                continue;
            }
            if (arg == "--doctor") {
                doctorCheck = true;
                continue;
            }
            if (arg == "--doctor-fix") {
                doctorCheck = true;
                doctorFix = true;
                continue;
            }
            if (arg == "--frozen") {
                frozen = true;
                continue;
            }
            std::cerr << "[aym] opcion no reconocida: " << arg << "\n";
            return 1;
        }

        aym::ProjectWorkspace workspace;
        std::string error;
        if (!aym::loadProjectWorkspace(manifestPath, workspace, error)) {
            std::cerr << "[aym] " << error << "\n";
            return 1;
        }
        ManifestLockFlowOptions flowOptions;
        flowOptions.printFrozenNotice = true;
        if (!ensureManifestLockFlow(aymcPath, workspace, frozen, flowOptions, error)) {
            std::cerr << "[aym] " << error << "\n";
            return 1;
        }
        if (doctorCheck && !runDependencyDoctor(workspace, doctorFix, error)) {
            std::cerr << "[aym] " << error << "\n";
            return 1;
        }
        if (doctorCheck && doctorFix) {
            std::cout << "[aym] Doctor OK (fix aplicado si fue necesario).\n";
        }
        if (!configureDependencyEnvironment(workspace, error)) {
            std::cerr << "[aym] " << error << "\n";
            return 1;
        }
        if (!buildProject(aymcPath, workspace, checkOnly, error)) {
            std::cerr << "[aym] " << error << "\n";
            return 1;
        }

        if (checkOnly) {
            std::cout << "[aym] Validacion completada: " << workspace.sourcePath.string() << "\n";
        } else {
            std::cout << "[aym] Build OK: " << outputBinaryPath(workspace).string() << "\n";
        }
        return 0;
    }

    if (command == "run") {
        fs::path manifestPath;
        bool doctorCheck = false;
        bool doctorFix = false;
        bool frozen = false;
        for (int i = 2; i < argc; ++i) {
            std::string error;
            if (parseManifestOption(argc, argv, i, manifestPath, error)) {
                continue;
            }
            const std::string arg = argv[i];
            if (arg == "--doctor") {
                doctorCheck = true;
                continue;
            }
            if (arg == "--doctor-fix") {
                doctorCheck = true;
                doctorFix = true;
                continue;
            }
            if (arg == "--frozen") {
                frozen = true;
                continue;
            }
            if (!error.empty()) {
                std::cerr << "[aym] " << error << "\n";
                return 1;
            }
            std::cerr << "[aym] opcion no reconocida: " << argv[i] << "\n";
            return 1;
        }

        aym::ProjectWorkspace workspace;
        std::string error;
        if (!aym::loadProjectWorkspace(manifestPath, workspace, error)) {
            std::cerr << "[aym] " << error << "\n";
            return 1;
        }
        ManifestLockFlowOptions flowOptions;
        flowOptions.printFrozenNotice = true;
        if (!ensureManifestLockFlow(aymcPath, workspace, frozen, flowOptions, error)) {
            std::cerr << "[aym] " << error << "\n";
            return 1;
        }
        if (doctorCheck && !runDependencyDoctor(workspace, doctorFix, error)) {
            std::cerr << "[aym] " << error << "\n";
            return 1;
        }
        if (doctorCheck && doctorFix) {
            std::cout << "[aym] Doctor OK (fix aplicado si fue necesario).\n";
        }
        if (!configureDependencyEnvironment(workspace, error)) {
            std::cerr << "[aym] " << error << "\n";
            return 1;
        }
        if (!buildProject(aymcPath, workspace, false, error)) {
            std::cerr << "[aym] " << error << "\n";
            return 1;
        }

        const fs::path binary = outputBinaryPath(workspace);
        if (!fs::exists(binary)) {
            std::cerr << "[aym] binario no encontrado tras build: " << binary.string() << "\n";
            return 1;
        }
        if (!runCommand({binary.string()}, error)) {
            std::cerr << "[aym] " << error << "\n";
            return 1;
        }
        return 0;
    }

    if (command == "test") {
        fs::path manifestPath;
        bool doctorCheck = false;
        bool doctorFix = false;
        bool frozen = false;
        for (int i = 2; i < argc; ++i) {
            std::string error;
            if (parseManifestOption(argc, argv, i, manifestPath, error)) {
                continue;
            }
            const std::string arg = argv[i];
            if (arg == "--doctor") {
                doctorCheck = true;
                continue;
            }
            if (arg == "--doctor-fix") {
                doctorCheck = true;
                doctorFix = true;
                continue;
            }
            if (arg == "--frozen") {
                frozen = true;
                continue;
            }
            if (!error.empty()) {
                std::cerr << "[aym] " << error << "\n";
                return 1;
            }
            std::cerr << "[aym] opcion no reconocida: " << argv[i] << "\n";
            return 1;
        }

        aym::ProjectWorkspace workspace;
        std::string error;
        if (!aym::loadProjectWorkspace(manifestPath, workspace, error)) {
            std::cerr << "[aym] " << error << "\n";
            return 1;
        }
        ManifestLockFlowOptions flowOptions;
        flowOptions.printFrozenNotice = true;
        if (!ensureManifestLockFlow(aymcPath, workspace, frozen, flowOptions, error)) {
            std::cerr << "[aym] " << error << "\n";
            return 1;
        }
        if (doctorCheck && !runDependencyDoctor(workspace, doctorFix, error)) {
            std::cerr << "[aym] " << error << "\n";
            return 1;
        }
        if (doctorCheck && doctorFix) {
            std::cout << "[aym] Doctor OK (fix aplicado si fue necesario).\n";
        }
        if (!configureDependencyEnvironment(workspace, error)) {
            std::cerr << "[aym] " << error << "\n";
            return 1;
        }

        std::vector<fs::path> testFiles;
        if (!aym::gatherProjectTestFiles(workspace.rootDir, testFiles, error)) {
            std::cerr << "[aym] " << error << "\n";
            return 1;
        }
        if (testFiles.empty()) {
            std::cout << "[aym] No hay tests .aym en: " << (workspace.rootDir / "tests").string() << "\n";
            return 0;
        }

        size_t passed = 0;
        for (const auto &testFile : testFiles) {
            if (runCommand({aymcPath.string(), "--check", testFile.string()}, error)) {
                ++passed;
                continue;
            }
            std::cerr << "[aym] Test fallo: " << testFile.string() << "\n";
            std::cerr << "[aym] " << error << "\n";
            return 1;
        }
        std::cout << "[aym] Tests OK: " << passed << "/" << testFiles.size() << "\n";
        return 0;
    }

    if (command == "lock") {
        if (argc < 3) {
            std::cerr << "[aym] usa: aym lock <sync|check> [--manifest <ruta>] [--frozen]\n";
            return 1;
        }

        const std::string subcommand = argv[2];
        fs::path manifestPath;
        bool frozen = false;
        for (int i = 3; i < argc; ++i) {
            std::string error;
            if (parseManifestOption(argc, argv, i, manifestPath, error)) {
                continue;
            }
            const std::string arg = argv[i];
            if (!error.empty()) {
                std::cerr << "[aym] " << error << "\n";
                return 1;
            }
            if (arg == "--frozen") {
                frozen = true;
                continue;
            }
            std::cerr << "[aym] opcion no reconocida: " << arg << "\n";
            return 1;
        }

        aym::ProjectWorkspace workspace;
        std::string error;
        if (!aym::loadProjectWorkspace(manifestPath, workspace, error)) {
            std::cerr << "[aym] " << error << "\n";
            return 1;
        }

        if (subcommand == "sync") {
            ManifestLockFlowOptions flowOptions;
            flowOptions.printFrozenNotice = true;
            if (!ensureManifestLockFlow(aymcPath, workspace, frozen, flowOptions, error)) {
                std::cerr << "[aym] " << error << "\n";
                return 1;
            }

            aym::ProjectLock lock;
            if (!loadVerifiedLock(workspace, lock, error)) {
                std::cerr << "[aym] " << error << "\n";
                return 1;
            }
            std::cout << "[aym] Lockfile sincronizado. Dependencias: " << lock.dependencies.size() << "\n";
            std::cout << "[aym] Lockfile: " << workspace.lockPath.string() << "\n";
            return 0;
        }

        if (subcommand == "check") {
            ManifestLockFlowOptions flowOptions;
            flowOptions.syncIfMutable = false;
            flowOptions.prefixFrozenError = false;
            flowOptions.printFrozenNotice = true;
            flowOptions.frozenNotice = "Modo frozen activo: validacion sin cambios.";
            if (!ensureManifestLockFlow(aymcPath, workspace, frozen, flowOptions, error)) {
                std::cerr << "[aym] " << error << "\n";
                return 1;
            }

            aym::ProjectLock lock;
            if (!loadVerifiedLock(workspace, lock, error)) {
                std::cerr << "[aym] " << error << "\n";
                return 1;
            }
            std::cout << "[aym] Lockfile consistente. Dependencias: " << lock.dependencies.size() << "\n";
            std::cout << "[aym] Lockfile: " << workspace.lockPath.string() << "\n";
            return 0;
        }

        std::cerr << "[aym] subcomando lock no reconocido: " << subcommand << "\n";
        std::cerr << "[aym] usa: aym lock <sync|check> [--manifest <ruta>] [--frozen]\n";
        return 1;
    }

    if (command == "cache") {
        if (argc < 3) {
            std::cerr << "[aym] usa: aym cache <status|sync|clean|doctor> [--manifest <ruta>]\n";
            return 1;
        }

        const std::string subcommand = argv[2];
        fs::path manifestPath;
        bool cleanRepo = false;
        bool cleanCache = false;
        bool cleanModeExplicit = false;
        bool doctorFix = false;
        bool frozen = false;

        for (int i = 3; i < argc; ++i) {
            std::string error;
            if (parseManifestOption(argc, argv, i, manifestPath, error)) {
                continue;
            }
            const std::string arg = argv[i];
            if (!error.empty()) {
                std::cerr << "[aym] " << error << "\n";
                return 1;
            }
            if (subcommand == "clean") {
                if (arg == "--repo") {
                    cleanRepo = true;
                    cleanModeExplicit = true;
                    continue;
                }
                if (arg == "--cache") {
                    cleanCache = true;
                    cleanModeExplicit = true;
                    continue;
                }
                if (arg == "--all") {
                    cleanRepo = true;
                    cleanCache = true;
                    cleanModeExplicit = true;
                    continue;
                }
            }
            if (subcommand == "doctor") {
                if (arg == "--fix") {
                    doctorFix = true;
                    continue;
                }
            }
            if (subcommand == "sync") {
                if (arg == "--frozen") {
                    frozen = true;
                    continue;
                }
            }
            std::cerr << "[aym] opcion no reconocida: " << arg << "\n";
            return 1;
        }

        aym::ProjectWorkspace workspace;
        std::string error;
        if (!aym::loadProjectWorkspace(manifestPath, workspace, error)) {
            std::cerr << "[aym] " << error << "\n";
            return 1;
        }

        if (subcommand == "status") {
            aym::DependencyStoreReport report;
            if (!aym::inspectProjectDependencyStore(workspace, report, error)) {
                std::cerr << "[aym] " << error << "\n";
                return 1;
            }

            std::cout << "[aym] Repo local: " << report.layout.repoRoot.string() << "\n";
            std::cout << "[aym] Cache local: " << report.layout.cacheRoot.string() << "\n";
            std::cout << "[aym] Repo existe: " << (fs::exists(report.layout.repoRoot) ? "si" : "no") << "\n";
            std::cout << "[aym] Cache existe: " << (fs::exists(report.layout.cacheRoot) ? "si" : "no") << "\n";
            std::cout << "[aym] Dependencias lock: " << report.entries.size() << "\n";
            for (const auto &entry : report.entries) {
                std::cout << "[aym]  - " << entry.name << "@" << entry.resolved
                          << " repo=" << (entry.repoModules ? "ok" : "faltante")
                          << " cache=" << (entry.cacheModules ? "ok" : "faltante")
                          << "\n";
            }
            return 0;
        }

        if (subcommand == "doctor") {
            auto printDoctor = [](const aym::DependencyStoreReport &report) {
                std::cout << "[aym] Repo local: " << report.layout.repoRoot.string() << "\n";
                std::cout << "[aym] Cache local: " << report.layout.cacheRoot.string() << "\n";
                std::cout << "[aym] Dependencias lock: " << report.entries.size() << "\n";
            };

            aym::DependencyStoreReport report;
            if (!aym::inspectProjectDependencyStore(workspace, report, error)) {
                if (!doctorFix) {
                    std::cerr << "[aym] Doctor detecto error: " << error << "\n";
                    std::cerr << "[aym] Ejecuta 'aym cache doctor --fix' para intentar repararlo.\n";
                    return 1;
                }

                if (!syncManifestAndLock(aymcPath, workspace.manifestPath, error)) {
                    std::cerr << "[aym] " << error << "\n";
                    return 1;
                }
                if (!aym::loadProjectWorkspace(workspace.manifestPath, workspace, error)) {
                    std::cerr << "[aym] " << error << "\n";
                    return 1;
                }
                ManifestLockFlowOptions flowOptions;
                flowOptions.syncIfMutable = false;
                if (!ensureManifestLockFlow(aymcPath, workspace, false, flowOptions, error)) {
                    std::cerr << "[aym] " << error << "\n";
                    return 1;
                }
                aym::DependencyCacheLayout repaired;
                if (!aym::prepareProjectDependencyCache(workspace, repaired, error)) {
                    std::cerr << "[aym] " << error << "\n";
                    return 1;
                }
                if (!aym::inspectProjectDependencyStore(workspace, report, error)) {
                    std::cerr << "[aym] Doctor fallo tras fix: " << error << "\n";
                    return 1;
                }
            }

            size_t missingRepo = 0;
            size_t missingCache = 0;
            for (const auto &entry : report.entries) {
                if (!entry.repoModules) {
                    ++missingRepo;
                }
                if (!entry.cacheModules) {
                    ++missingCache;
                }
            }
            if ((missingRepo > 0 || missingCache > 0) && doctorFix) {
                aym::DependencyCacheLayout repaired;
                if (!aym::prepareProjectDependencyCache(workspace, repaired, error)) {
                    std::cerr << "[aym] " << error << "\n";
                    return 1;
                }
                if (!aym::inspectProjectDependencyStore(workspace, report, error)) {
                    std::cerr << "[aym] Doctor fallo tras fix: " << error << "\n";
                    return 1;
                }
                missingRepo = 0;
                missingCache = 0;
                for (const auto &entry : report.entries) {
                    if (!entry.repoModules) {
                        ++missingRepo;
                    }
                    if (!entry.cacheModules) {
                        ++missingCache;
                    }
                }
            }

            printDoctor(report);
            if (missingRepo == 0 && missingCache == 0) {
                std::cout << "[aym] Doctor OK: store local consistente.\n";
                return 0;
            }

            for (const auto &entry : report.entries) {
                if (entry.repoModules && entry.cacheModules) {
                    continue;
                }
                std::cout << "[aym]  - " << entry.name << "@" << entry.resolved
                          << " repo=" << (entry.repoModules ? "ok" : "faltante")
                          << " cache=" << (entry.cacheModules ? "ok" : "faltante")
                          << "\n";
            }
            if (doctorFix) {
                std::cerr << "[aym] Doctor: aun hay inconsistencias tras --fix.\n";
            } else {
                std::cerr << "[aym] Doctor detecto inconsistencias. Ejecuta 'aym cache doctor --fix'.\n";
            }
            return 1;
        }

        if (subcommand == "sync") {
            ManifestLockFlowOptions flowOptions;
            flowOptions.printFrozenNotice = true;
            if (!ensureManifestLockFlow(aymcPath, workspace, frozen, flowOptions, error)) {
                std::cerr << "[aym] " << error << "\n";
                return 1;
            }

            aym::DependencyCacheLayout layout;
            if (!aym::prepareProjectDependencyCache(workspace, layout, error)) {
                std::cerr << "[aym] " << error << "\n";
                return 1;
            }

            aym::ProjectLock lock;
            if (!loadVerifiedLock(workspace, lock, error)) {
                std::cerr << "[aym] " << error << "\n";
                return 1;
            }
            std::cout << "[aym] Cache sincronizada. Dependencias: " << lock.dependencies.size() << "\n";
            std::cout << "[aym] Repo local: " << layout.repoRoot.string() << "\n";
            std::cout << "[aym] Cache local: " << layout.cacheRoot.string() << "\n";
            return 0;
        }

        if (subcommand == "clean") {
            if (!cleanModeExplicit) {
                cleanCache = true;
            }
            if (!aym::cleanProjectDependencyCache(workspace, cleanRepo, cleanCache, error)) {
                std::cerr << "[aym] " << error << "\n";
                return 1;
            }

            aym::DependencyCacheLayout layout;
            if (!aym::resolveProjectDependencyLayout(workspace, layout, error)) {
                std::cerr << "[aym] " << error << "\n";
                return 1;
            }
            std::cout << "[aym] Limpieza completada.\n";
            if (cleanCache) {
                std::cout << "[aym] Cache limpia: " << layout.cacheRoot.string() << "\n";
            }
            if (cleanRepo) {
                std::cout << "[aym] Repo limpio: " << layout.repoRoot.string() << "\n";
            }
            return 0;
        }

        std::cerr << "[aym] subcomando cache no reconocido: " << subcommand << "\n";
        std::cerr << "[aym] usa: aym cache <status|sync|clean|doctor> [--manifest <ruta>]\n";
        return 1;
    }

    if (command == "add") {
        fs::path manifestPath;
        std::string dependency;
        std::string requirement = "1.0.0";
        bool frozen = false;

        for (int i = 2; i < argc; ++i) {
            std::string error;
            if (parseManifestOption(argc, argv, i, manifestPath, error)) {
                continue;
            }
            const std::string arg = argv[i];
            if (!error.empty()) {
                std::cerr << "[aym] " << error << "\n";
                return 1;
            }
            if (arg == "--frozen") {
                frozen = true;
                continue;
            }
            if (!arg.empty() && arg[0] == '-') {
                std::cerr << "[aym] opcion no reconocida: " << arg << "\n";
                return 1;
            }
            if (dependency.empty()) {
                dependency = arg;
                continue;
            }
            if (requirement == "1.0.0") {
                requirement = arg;
                continue;
            }
            std::cerr << "[aym] usa: aym add <dep> [requirement] [--manifest <ruta>] [--frozen]\n";
            return 1;
        }

        if (dependency.empty()) {
            std::cerr << "[aym] usa: aym add <dep> [requirement] [--manifest <ruta>] [--frozen]\n";
            return 1;
        }
        if (frozen) {
            std::cerr << "[aym] modo --frozen: 'add' no permite modificar dependencias.\n";
            std::cerr << "[aym] Ejecuta 'aym add' sin --frozen para actualizar el manifest/lock.\n";
            return 1;
        }

        aym::ProjectWorkspace workspace;
        std::string error;
        if (!aym::loadProjectWorkspace(manifestPath, workspace, error)) {
            std::cerr << "[aym] " << error << "\n";
            return 1;
        }
        if (!aym::upsertProjectDependency(workspace.manifestPath, dependency, requirement, error)) {
            std::cerr << "[aym] " << error << "\n";
            return 1;
        }
        if (!syncManifestAndLock(aymcPath, workspace.manifestPath, error)) {
            std::cerr << "[aym] " << error << "\n";
            return 1;
        }
        if (!aym::loadProjectWorkspace(workspace.manifestPath, workspace, error)) {
            std::cerr << "[aym] " << error << "\n";
            return 1;
        }
        ManifestLockFlowOptions flowOptions;
        flowOptions.syncIfMutable = false;
        if (!ensureManifestLockFlow(aymcPath, workspace, false, flowOptions, error)) {
            std::cerr << "[aym] " << error << "\n";
            return 1;
        }

        aym::DependencyCacheLayout cacheLayout;
        if (!aym::prepareProjectDependencyCache(workspace, cacheLayout, error)) {
            std::cerr << "[aym] " << error << "\n";
            return 1;
        }

        std::cout << "[aym] Dependencia actualizada: " << dependency << " = \"" << requirement << "\"\n";
        std::cout << "[aym] Repo local: " << cacheLayout.repoRoot.string() << "\n";
        std::cout << "[aym] Cache local: " << cacheLayout.cacheRoot.string() << "\n";
        return 0;
    }

    std::cerr << "[aym] comando no reconocido: " << command << "\n";
    std::cout << helpText();
    return 1;
}
