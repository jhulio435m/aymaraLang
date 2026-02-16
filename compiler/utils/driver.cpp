#include "driver.h"

#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include "module_resolver.h"
#include "utils.h"

namespace aym {

namespace {

std::string toLowerAscii(std::string value) {
    for (char &ch : value) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return value;
}

} // namespace

CompileOptions makeDefaultCompileOptions() {
    CompileOptions options;
#ifdef _WIN32
    options.windowsTarget = true;
#else
    options.windowsTarget = false;
#endif
    return options;
}

CliParseResult parseCompileOptions(int argc, char **argv, CompileOptions &options, std::string &errorMsg) {
    options = makeDefaultCompileOptions();
    auto parseToolTimeoutMs = [&](const std::string &value) -> bool {
        if (value.empty()) {
            errorMsg = "La opcion --tool-timeout-ms requiere un valor entero >= 0.";
            return false;
        }
        try {
            size_t consumed = 0;
            const long long parsed = std::stoll(value, &consumed);
            if (consumed != value.size() || parsed < 0) {
                errorMsg = "Valor invalido para --tool-timeout-ms: " + value;
                return false;
            }
            options.toolTimeoutMs = parsed;
            return true;
        } catch (const std::exception &) {
            errorMsg = "Valor invalido para --tool-timeout-ms: " + value;
            return false;
        }
    };
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        const std::string astJsonPrefix = "--emit-ast-json=";
        const std::string diagnosticsJsonPrefix = "--diagnostics-json=";
        const std::string timePipelineJsonPrefix = "--time-pipeline-json=";
        const std::string toolTimeoutPrefix = "--tool-timeout-ms=";
        const std::string backendPrefix = "--backend=";
        const std::string checkManifestPrefix = "--check-manifest=";
        const std::string emitLockPrefix = "--emit-lock=";
        const std::string checkLockPrefix = "--check-lock=";
        const std::string manifestPrefix = "--manifest=";
        const std::string lockPrefix = "--lock=";
        if (arg == "-h" || arg == "--help") {
            return CliParseResult::ShowHelp;
        }
        if (arg == "-o") {
            if (i + 1 >= argc) {
                errorMsg = "La opcion -o requiere una ruta de salida.";
                return CliParseResult::Error;
            }
            options.output = argv[++i];
            options.outputProvided = true;
            continue;
        }
        if (arg == "--backend") {
            if (i + 1 >= argc) {
                errorMsg = "La opcion --backend requiere un valor (native|ir).";
                return CliParseResult::Error;
            }
            options.backend = argv[++i];
            if (options.backend.empty() || options.backend[0] == '-') {
                errorMsg = "La opcion --backend requiere un valor valido (native|ir).";
                return CliParseResult::Error;
            }
            continue;
        }
        if (arg.rfind(backendPrefix, 0) == 0) {
            options.backend = arg.substr(backendPrefix.size());
            if (options.backend.empty()) {
                errorMsg = "La opcion --backend requiere un valor valido (native|ir).";
                return CliParseResult::Error;
            }
            continue;
        }
        if (arg == "--debug") {
            options.debug = true;
            continue;
        }
        if (arg == "--dump-ast") {
            options.dumpAst = true;
            continue;
        }
        if (arg == "--check") {
            options.checkOnly = true;
            continue;
        }
        if (arg == "--emit-asm") {
            options.emitAsm = true;
            continue;
        }
        if (arg == "--compile-only") {
            options.compileOnly = true;
            continue;
        }
        if (arg == "--link-only") {
            options.linkOnly = true;
            continue;
        }
        if (arg == "--time-pipeline") {
            options.timePipeline = true;
            continue;
        }
        if (arg == "--time-pipeline-json") {
            options.emitTimePipelineJson = true;
            continue;
        }
        if (arg.rfind(timePipelineJsonPrefix, 0) == 0) {
            options.emitTimePipelineJson = true;
            options.timePipelineJsonPath = arg.substr(timePipelineJsonPrefix.size());
            if (options.timePipelineJsonPath.empty()) {
                errorMsg = "La opcion --time-pipeline-json requiere una ruta valida.";
                return CliParseResult::Error;
            }
            continue;
        }
        if (arg == "--tool-timeout-ms") {
            if (i + 1 >= argc) {
                errorMsg = "La opcion --tool-timeout-ms requiere un valor entero >= 0.";
                return CliParseResult::Error;
            }
            if (!parseToolTimeoutMs(argv[++i])) {
                return CliParseResult::Error;
            }
            continue;
        }
        if (arg.rfind(toolTimeoutPrefix, 0) == 0) {
            if (!parseToolTimeoutMs(arg.substr(toolTimeoutPrefix.size()))) {
                return CliParseResult::Error;
            }
            continue;
        }
        if (arg == "--emit-ast-json") {
            options.emitAstJson = true;
            continue;
        }
        if (arg.rfind(astJsonPrefix, 0) == 0) {
            options.emitAstJson = true;
            options.astJsonPath = arg.substr(astJsonPrefix.size());
            if (options.astJsonPath.empty()) {
                errorMsg = "La opcion --emit-ast-json requiere una ruta valida.";
                return CliParseResult::Error;
            }
            continue;
        }
        if (arg == "--diagnostics-json") {
            options.emitDiagnosticsJson = true;
            continue;
        }
        if (arg.rfind(diagnosticsJsonPrefix, 0) == 0) {
            options.emitDiagnosticsJson = true;
            options.diagnosticsJsonPath = arg.substr(diagnosticsJsonPrefix.size());
            if (options.diagnosticsJsonPath.empty()) {
                errorMsg = "La opcion --diagnostics-json requiere una ruta valida.";
                return CliParseResult::Error;
            }
            continue;
        }
        if (arg == "--check-manifest") {
            options.checkManifest = true;
            continue;
        }
        if (arg.rfind(checkManifestPrefix, 0) == 0) {
            options.checkManifest = true;
            options.manifestPath = arg.substr(checkManifestPrefix.size());
            if (options.manifestPath.empty()) {
                errorMsg = "La opcion --check-manifest requiere una ruta valida.";
                return CliParseResult::Error;
            }
            continue;
        }
        if (arg == "--manifest") {
            if (i + 1 >= argc) {
                errorMsg = "La opcion --manifest requiere una ruta.";
                return CliParseResult::Error;
            }
            options.manifestPath = argv[++i];
            if (options.manifestPath.empty()) {
                errorMsg = "La opcion --manifest requiere una ruta valida.";
                return CliParseResult::Error;
            }
            continue;
        }
        if (arg.rfind(manifestPrefix, 0) == 0) {
            options.manifestPath = arg.substr(manifestPrefix.size());
            if (options.manifestPath.empty()) {
                errorMsg = "La opcion --manifest requiere una ruta valida.";
                return CliParseResult::Error;
            }
            continue;
        }
        if (arg == "--emit-lock") {
            options.emitLock = true;
            continue;
        }
        if (arg.rfind(emitLockPrefix, 0) == 0) {
            options.emitLock = true;
            options.lockPath = arg.substr(emitLockPrefix.size());
            if (options.lockPath.empty()) {
                errorMsg = "La opcion --emit-lock requiere una ruta valida.";
                return CliParseResult::Error;
            }
            continue;
        }
        if (arg == "--check-lock") {
            options.checkLock = true;
            continue;
        }
        if (arg.rfind(checkLockPrefix, 0) == 0) {
            options.checkLock = true;
            options.lockPath = arg.substr(checkLockPrefix.size());
            if (options.lockPath.empty()) {
                errorMsg = "La opcion --check-lock requiere una ruta valida.";
                return CliParseResult::Error;
            }
            continue;
        }
        if (arg == "--lock") {
            if (i + 1 >= argc) {
                errorMsg = "La opcion --lock requiere una ruta.";
                return CliParseResult::Error;
            }
            options.lockPath = argv[++i];
            if (options.lockPath.empty()) {
                errorMsg = "La opcion --lock requiere una ruta valida.";
                return CliParseResult::Error;
            }
            continue;
        }
        if (arg.rfind(lockPrefix, 0) == 0) {
            options.lockPath = arg.substr(lockPrefix.size());
            if (options.lockPath.empty()) {
                errorMsg = "La opcion --lock requiere una ruta valida.";
                return CliParseResult::Error;
            }
            continue;
        }
        if (arg == "--windows") {
            options.windowsTarget = true;
            continue;
        }
        if (arg == "--linux") {
            options.windowsTarget = false;
            continue;
        }
        if (arg == "--seed") {
            if (i + 1 >= argc) {
                errorMsg = "La opcion --seed requiere un valor entero.";
                return CliParseResult::Error;
            }
            std::string seedArg = argv[++i];
            try {
                size_t consumed = 0;
                options.seed = std::stol(seedArg, &consumed);
                if (consumed != seedArg.size()) {
                    errorMsg = "Valor invalido para --seed: " + seedArg;
                    return CliParseResult::Error;
                }
                options.seedProvided = true;
            } catch (const std::exception &) {
                errorMsg = "Valor invalido para --seed: " + seedArg;
                return CliParseResult::Error;
            }
            continue;
        }
        if (!arg.empty() && arg[0] == '-') {
            errorMsg = "Opcion no reconocida: " + arg;
            return CliParseResult::Error;
        }
        options.inputs.push_back(arg);
    }

    options.backend = toLowerAscii(options.backend);
    if (options.backend != "native" && options.backend != "ir") {
        errorMsg = "Backend no soportado: " + options.backend + ". Usa --backend native o --backend ir.";
        return CliParseResult::Error;
    }

    if (options.compileOnly && options.linkOnly) {
        errorMsg = "No se puede usar --compile-only y --link-only al mismo tiempo.";
        return CliParseResult::Error;
    }
    if (options.checkOnly && (options.compileOnly || options.linkOnly)) {
        errorMsg = "La opcion --check no es compatible con --compile-only/--link-only.";
        return CliParseResult::Error;
    }
    if (options.checkOnly && (options.timePipeline || options.emitTimePipelineJson)) {
        errorMsg = "La opcion --check no ejecuta backend y no es compatible con --time-pipeline/--time-pipeline-json.";
        return CliParseResult::Error;
    }
    if (options.linkOnly && !options.inputs.empty()) {
        errorMsg = "La opcion --link-only no acepta archivos de entrada.";
        return CliParseResult::Error;
    }
    if (options.linkOnly && !options.outputProvided) {
        errorMsg = "La opcion --link-only requiere -o <ruta-base> para ubicar el objeto a enlazar.";
        return CliParseResult::Error;
    }
    if (options.linkOnly && (options.debug || options.dumpAst || options.emitAstJson)) {
        errorMsg = "Las opciones --debug/--dump-ast/--emit-ast-json requieren compilacion completa y no son compatibles con --link-only.";
        return CliParseResult::Error;
    }

    if (options.inputs.empty() && !options.checkManifest && !options.emitLock && !options.checkLock && !options.linkOnly) {
        errorMsg = "Se requiere un archivo de entrada";
        return CliParseResult::Error;
    }
    return CliParseResult::Ok;
}

const char *compileHelpText() {
    return "Uso: aymc [opciones] archivo.aym ...\n"
           "\n"
           "Opciones:\n"
           "  -h, --help                   Muestra esta ayuda\n"
           "  -o <ruta>                    Define nombre/ruta del ejecutable\n"
           "  --backend <nombre>           Selecciona backend (native|ir)\n"
           "  --debug                      Imprime tokens en consola\n"
           "  --dump-ast                   Imprime total de nodos AST\n"
           "  --check                      Solo valida sintaxis/semantica (sin generar binario)\n"
           "  --emit-asm                   Conserva el archivo ASM intermedio\n"
           "  --compile-only               Genera ASM/objeto y omite el enlace final\n"
           "  --link-only                  Enlaza un objeto existente (requiere -o)\n"
           "  --time-pipeline              Reporta tiempos por etapa del backend\n"
           "  --time-pipeline-json[=ruta]  Exporta metricas de pipeline a JSON\n"
           "  --tool-timeout-ms <ms>       Timeout maximo por comando externo (0 = sin limite)\n"
           "  --emit-ast-json[=ruta]       Exporta AST en formato JSON\n"
           "  --diagnostics-json[=ruta]    Exporta diagnosticos en JSON\n"
           "  --check-manifest[=ruta]      Valida aym.toml (por defecto ./aym.toml)\n"
           "  --manifest <ruta>            Define ruta de manifest para check/lock\n"
           "  --emit-lock[=ruta]           Genera aym.lock desde el manifest\n"
           "  --check-lock[=ruta]          Valida aym.lock (por defecto ./aym.lock)\n"
           "  --lock <ruta>                Define ruta de lockfile para check-lock\n"
           "  --windows                    Fuerza objetivo Windows\n"
           "  --linux                      Fuerza objetivo Linux\n"
           "  --seed <valor>               Fija semilla del PRNG\n"
           "\n"
           "Ejemplos:\n"
           "  aymc programa.aym\n"
           "  aymc --backend native programa.aym\n"
           "  aymc --check --diagnostics-json programa.aym\n"
           "  aymc --emit-asm -o build/app programa.aym\n"
           "  aymc --compile-only -o build/app programa.aym\n"
           "  aymc --link-only -o build/app\n"
           "  aymc --time-pipeline -o build/app programa.aym\n"
           "  aymc --time-pipeline-json -o build/app programa.aym\n"
           "  aymc --tool-timeout-ms 30000 -o build/app programa.aym\n"
           "  aymc --check-manifest --emit-lock\n"
           "  aymc --check-manifest --check-lock\n";
}

void finalizeOutputPath(CompileOptions &options) {
    if (options.outputProvided || options.inputs.empty()) {
        return;
    }
    fs::path inputPath = fs::path(options.inputs.front());
    fs::path base = inputPath.stem();
    if (inputPath.has_parent_path()) {
        options.output = (inputPath.parent_path() / base).string();
    } else {
        options.output = base.string();
    }
}

bool loadInputSources(const std::vector<std::string> &inputs, std::string &source, std::string &failedPath) {
    source.clear();
    failedPath.clear();
    for (const auto &in : inputs) {
        std::ifstream file(in);
        if (!file.is_open()) {
            failedPath = in;
            return false;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        source += buffer.str();
        source += "\n";
    }
    return true;
}

fs::path detectEntryDirectory(const std::vector<std::string> &inputs) {
    fs::path entryDir = fs::current_path();
    if (inputs.empty()) {
        return entryDir;
    }
    fs::path first = fs::path(inputs.front());
    if (!first.is_absolute()) {
        first = fs::absolute(first);
    }
    if (fs::is_directory(first)) {
        return first;
    }
    if (first.has_parent_path()) {
        return first.parent_path();
    }
    return entryDir;
}

void registerInputSearchPaths(const std::vector<std::string> &inputs, ModuleResolver &resolver) {
    for (const auto &in : inputs) {
        fs::path path = fs::path(in);
        if (!path.is_absolute()) {
            path = fs::absolute(path);
        }
        if (path.has_parent_path()) {
            resolver.addSearchPath(path.parent_path());
        }
    }
}

fs::path findRuntimeDirectory() {
    fs::path runtimeDir = fs::path(executableDir()) / "runtime";
    if (fs::exists(runtimeDir)) {
        return runtimeDir;
    }
    runtimeDir = fs::path(executableDir()) / ".." / "share" / "aymaraLang" / "runtime";
    if (fs::exists(runtimeDir)) {
        return runtimeDir;
    }
    runtimeDir = fs::path("runtime");
    return runtimeDir;
}

} // namespace aym
