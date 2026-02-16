#include "backend.h"

#include "../utils/fs.h"

#include <chrono>
#include <cctype>
#include <fstream>
#include <iostream>

namespace aym {

namespace {

std::string toLowerAscii(std::string value) {
    for (char &ch : value) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return value;
}

std::string jsonEscape(const std::string &value) {
    std::string out;
    out.reserve(value.size() + 8);
    for (char ch : value) {
        switch (ch) {
            case '\\':
                out += "\\\\";
                break;
            case '"':
                out += "\\\"";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\r':
                out += "\\r";
                break;
            case '\t':
                out += "\\t";
                break;
            default:
                out.push_back(ch);
                break;
        }
    }
    return out;
}

bool writeIrPipelineJson(const fs::path &jsonPath,
                         CodegenPipelineMode mode,
                         bool windowsTarget,
                         long long toolTimeoutMs,
                         long long irGenMs,
                         long long totalMs,
                         const fs::path &irPath,
                         std::string &error) {
    error.clear();
    std::error_code ec;
    if (jsonPath.has_parent_path()) {
        fs::create_directories(jsonPath.parent_path(), ec);
        if (ec) {
            error = "No se pudo crear directorio para metrics JSON: " + jsonPath.parent_path().string();
            return false;
        }
    }

    std::ofstream out(jsonPath);
    if (!out.is_open()) {
        error = "No se pudo escribir metrics JSON en: " + jsonPath.string();
        return false;
    }

    ec.clear();
    const bool irExists = fs::exists(irPath, ec);
    const char *modeLabel = (mode == CodegenPipelineMode::CompileOnly) ? "compile-only"
                         : (mode == CodegenPipelineMode::LinkOnly) ? "link-only"
                         : "full";
    out << "{\n";
    out << "  \"schema\": \"aymc.pipeline.v1\",\n";
    out << "  \"mode\": \"" << modeLabel << "\",\n";
    out << "  \"backend\": \"ir\",\n";
    out << "  \"windows_target\": " << (windowsTarget ? "true" : "false") << ",\n";
    out << "  \"tool_timeout_ms\": " << toolTimeoutMs << ",\n";
    out << "  \"timing_ms\": {\n";
    out << "    \"assemble\": 0,\n";
    out << "    \"runtime_compile\": 0,\n";
    out << "    \"link\": 0,\n";
    out << "    \"ir_gen\": " << irGenMs << ",\n";
    out << "    \"total\": " << totalMs << "\n";
    out << "  },\n";
    out << "  \"runtime\": {\n";
    out << "    \"runtime_c\": \"n/a\",\n";
    out << "    \"runtime_math\": \"n/a\"\n";
    out << "  },\n";
    out << "  \"artifacts\": {\n";
    out << "    \"ir\": \"" << jsonEscape(irPath.string()) << "\",\n";
    out << "    \"ir_exists\": " << (irExists ? "true" : "false") << "\n";
    out << "  },\n";
    out << "  \"commands\": [\n";
    out << "    {\n";
    out << "      \"stage\": \"ir-gen\",\n";
    out << "      \"status\": \"ok\",\n";
    out << "      \"exit_reason\": \"ok\",\n";
    out << "      \"command\": \"\",\n";
    out << "      \"exit_code\": 0,\n";
    out << "      \"timed_out\": false,\n";
    out << "      \"elapsed_ms\": " << irGenMs << ",\n";
    out << "      \"detail\": \"\",\n";
    out << "      \"stdout_truncated\": false,\n";
    out << "      \"stderr_truncated\": false,\n";
    out << "      \"stdout\": \"\",\n";
    out << "      \"stderr\": \"\"\n";
    out << "    }\n";
    out << "  ],\n";
    out << "  \"phase_summary\": {\n";
    out << "    \"commands_total\": 1,\n";
    out << "    \"ok\": 1,\n";
    out << "    \"failed\": 0,\n";
    out << "    \"cache_hit\": 0,\n";
    out << "    \"timed_out\": 0,\n";
    out << "    \"reason_ok\": 1,\n";
    out << "    \"reason_nonzero_exit\": 0,\n";
    out << "    \"reason_timeout\": 0,\n";
    out << "    \"reason_missing_input\": 0,\n";
    out << "    \"reason_cache_hit\": 0,\n";
    out << "    \"reason_internal_error\": 0,\n";
    out << "    \"reason_other\": 0,\n";
    out << "    \"elapsed_ms_sum\": " << irGenMs << "\n";
    out << "  },\n";
    out << "  \"result\": {\n";
    out << "    \"success\": true,\n";
    out << "    \"failed_stage\": \"none\",\n";
    out << "    \"message\": \"\",\n";
    out << "    \"command\": \"\",\n";
    out << "    \"exit_code\": 0,\n";
    out << "    \"detail\": \"\"\n";
    out << "  }\n";
    out << "}\n";

    if (!out.good()) {
        error = "Fallo escribiendo metrics JSON en: " + jsonPath.string();
        return false;
    }
    return true;
}

bool emitIrPrototype(const fs::path &irPath,
                     CodegenPipelineMode mode,
                     size_t nodeCount,
                     size_t globalCount,
                     size_t functionCount,
                     std::string &error) {
    error.clear();
    std::error_code ec;
    if (irPath.has_parent_path()) {
        fs::create_directories(irPath.parent_path(), ec);
        if (ec) {
            error = "No se pudo crear directorio de salida IR: " + irPath.parent_path().string();
            return false;
        }
    }

    std::ofstream out(irPath);
    if (!out.is_open()) {
        error = "No se pudo generar artefacto IR: " + irPath.string();
        return false;
    }
    const char *modeLabel = (mode == CodegenPipelineMode::CompileOnly) ? "compile-only"
                         : (mode == CodegenPipelineMode::LinkOnly) ? "link-only"
                         : "full";
    out << "# AymaraLang IR Prototype\n";
    out << "backend = ir\n";
    out << "mode = " << modeLabel << "\n";
    out << "nodes = " << nodeCount << "\n";
    out << "globals = " << globalCount << "\n";
    out << "functions = " << functionCount << "\n";
    out << "\n";
    out << "; Placeholder de IR textual para evolucion multi-backend.\n";
    out << "; En esta fase no hay lowering a codigo nativo desde IR.\n";

    if (!out.good()) {
        error = "Fallo escribiendo artefacto IR: " + irPath.string();
        return false;
    }
    return true;
}

} // namespace

bool parseBackendKind(const std::string &value, BackendKind &kind, std::string &errorMessage) {
    const std::string backend = toLowerAscii(value);
    if (backend == "native") {
        kind = BackendKind::Native;
        errorMessage.clear();
        return true;
    }
    if (backend == "ir") {
        kind = BackendKind::Ir;
        errorMessage.clear();
        return true;
    }
    errorMessage = "Backend no soportado: " + value + ". Usa --backend native o --backend ir.";
    return false;
}

const char *backendKindName(BackendKind kind) {
    switch (kind) {
        case BackendKind::Native:
            return "native";
        case BackendKind::Ir:
            return "ir";
    }
    return "unknown";
}

bool runBackendCompile(BackendKind kind,
                       const std::vector<std::unique_ptr<Node>> &nodes,
                       const std::string &outputPath,
                       const std::unordered_set<std::string> &globals,
                       const std::unordered_map<std::string, std::vector<std::string>> &paramTypes,
                       const std::unordered_map<std::string, std::string> &functionReturnTypes,
                       const std::unordered_map<std::string, std::string> &globalTypes,
                       bool windowsTarget,
                       long seed,
                       const std::string &runtimeDir,
                       bool keepAsm,
                       CodegenPipelineMode mode,
                       bool timePipeline,
                       const std::string &timePipelineJsonPath,
                       long long toolTimeoutMs,
                       std::string &errorMessage) {
    errorMessage.clear();
    if (kind == BackendKind::Native) {
        CodeGenerator generator;
        const bool ok = generator.generate(nodes,
                                           outputPath,
                                           globals,
                                           paramTypes,
                                           functionReturnTypes,
                                           globalTypes,
                                           windowsTarget,
                                           seed,
                                           runtimeDir,
                                           keepAsm,
                                           mode,
                                           timePipeline,
                                           timePipelineJsonPath,
                                           toolTimeoutMs,
                                           &errorMessage);
        if (!ok && errorMessage.empty()) {
            errorMessage = "Fallo backend nativo sin detalle.";
        }
        return ok;
    }
    if (mode == CodegenPipelineMode::LinkOnly) {
        errorMessage = "Backend 'ir' no soporta --link-only en esta fase. Usa --backend=native para enlace.";
        return false;
    }

    const auto started = std::chrono::steady_clock::now();
    fs::path irPath = fs::path(outputPath);
    irPath.replace_extension(".ir");
    if (irPath.empty()) {
        errorMessage = "Ruta de salida invalida para backend ir.";
        return false;
    }

    if (!emitIrPrototype(irPath,
                         mode,
                         nodes.size(),
                         globals.size(),
                         functionReturnTypes.size(),
                         errorMessage)) {
        return false;
    }

    const auto finished = std::chrono::steady_clock::now();
    const auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(finished - started).count();
    const long long irGenMs = totalMs;

    if (timePipeline) {
        std::cout << "[aymc] etapa ir-gen: " << irGenMs << " ms" << std::endl;
        std::cout << "[aymc] pipeline(ms): ensamblado=0, runtime=0, enlace=0, total=" << totalMs << std::endl;
    }

    if (!timePipelineJsonPath.empty()) {
        std::string jsonError;
        if (!writeIrPipelineJson(fs::path(timePipelineJsonPath),
                                 mode,
                                 windowsTarget,
                                 toolTimeoutMs,
                                 irGenMs,
                                 totalMs,
                                 irPath,
                                 jsonError)) {
            errorMessage = jsonError;
            return false;
        }
        std::cout << "[aymc] Pipeline JSON generado: " << timePipelineJsonPath << std::endl;
    }

    std::cout << "[aymc] IR generado: " << irPath.string() << std::endl;
    if (mode == CodegenPipelineMode::Full) {
        std::cout << "[aymc] backend=ir activo: fase experimental (sin ejecutable nativo)." << std::endl;
    }
    return true;
}

} // namespace aym
