#include "codegen_impl.h"
#include "../utils/fs.h"
#include "../utils/process.h"
#include <chrono>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>

namespace aym {

namespace {

std::string runtimeCacheKey(const fs::path &runtimeDir, bool windowsTarget) {
    std::string key = runtimeDir.lexically_normal().string();
    key += windowsTarget ? "|win64" : "|elf64";
    return std::to_string(std::hash<std::string>{}(key));
}

fs::path runtimeCacheDir(const fs::path &fallbackDir) {
    std::error_code ec;
    fs::path base = fs::temp_directory_path(ec);
    if (ec || base.empty()) {
        return fallbackDir;
    }
    fs::path cache = base / "aymc" / "runtime-cache";
    fs::create_directories(cache, ec);
    if (ec || cache.empty()) {
        return fallbackDir;
    }
    return cache;
}

bool needsRebuildObject(const fs::path &source, const fs::path &object) {
    std::error_code ec;
    if (!fs::exists(object, ec)) {
        return true;
    }
    ec.clear();
    const auto srcTime = fs::last_write_time(source, ec);
    if (ec) {
        return true;
    }
    ec.clear();
    const auto objTime = fs::last_write_time(object, ec);
    if (ec) {
        return true;
    }
    return objTime < srcTime;
}

struct PipelineFailureInfo {
    bool success = true;
    std::string stage = "none";
    std::string message;
    std::string command;
    int exitCode = 0;
    std::string detail;
};

struct PipelineCommandTrace {
    std::string stage;
    std::string status;
    std::string exitReason;
    std::string command;
    int exitCode = 0;
    bool timedOut = false;
    long long elapsedMs = 0;
    std::string detail;
    bool stdoutTruncated = false;
    bool stderrTruncated = false;
    std::string stdoutText;
    std::string stderrText;
};

std::string normalizeExitReason(const std::string &status,
                                bool timedOut,
                                int exitCode,
                                const std::string &detail,
                                const std::string &hint) {
    if (!hint.empty()) {
        return hint;
    }
    if (status == "cache-hit") {
        return "cache_hit";
    }
    if (status == "ok") {
        return "ok";
    }
    if (timedOut) {
        return "timeout";
    }
    if (detail == "objeto faltante" || detail == "fuente runtime faltante") {
        return "missing_input";
    }
    if (status == "failed" && exitCode > 0) {
        return "nonzero_exit";
    }
    return "internal_error";
}

void appendCommandTrace(std::vector<PipelineCommandTrace> &commands,
                        const std::string &stage,
                        const std::string &status,
                        const std::string &command,
                        int exitCode,
                        bool timedOut,
                        long long elapsedMs,
                        const std::string &detail,
                        bool stdoutTruncated = false,
                        bool stderrTruncated = false,
                        const std::string &stdoutText = "",
                        const std::string &stderrText = "",
                        const std::string &exitReasonHint = "") {
    PipelineCommandTrace trace;
    trace.stage = stage;
    trace.status = status;
    trace.exitReason = normalizeExitReason(status, timedOut, exitCode, detail, exitReasonHint);
    trace.command = command;
    trace.exitCode = exitCode;
    trace.timedOut = timedOut;
    trace.elapsedMs = elapsedMs;
    trace.detail = detail;
    trace.stdoutTruncated = stdoutTruncated;
    trace.stderrTruncated = stderrTruncated;
    trace.stdoutText = stdoutText;
    trace.stderrText = stderrText;
    commands.push_back(std::move(trace));
}

void setFailure(PipelineFailureInfo &failure,
                const std::string &stage,
                const std::string &message,
                const ProcessResult *process = nullptr,
                const std::string &detail = "") {
    failure.success = false;
    failure.stage = stage;
    failure.message = message;
    failure.command = (process != nullptr) ? process->command : "";
    failure.exitCode = (process != nullptr) ? process->exitCode : -1;
    failure.detail = detail.empty() ? ((process != nullptr) ? process->error : "") : detail;
}

std::string composePipelineFailureMessage(const PipelineFailureInfo &failure) {
    std::ostringstream out;
    out << "Fallo en etapa backend '" << failure.stage << "': " << failure.message;
    if (!failure.command.empty()) {
        out << " | comando: " << failure.command;
    }
    if (failure.exitCode != 0) {
        out << " | exit_code: " << failure.exitCode;
    }
    if (!failure.detail.empty()) {
        out << " | detalle: " << failure.detail;
    }
    return out.str();
}

bool runCommand(const std::vector<std::string> &args,
                const std::string &errorMessage,
                const std::string &stageLabel,
                bool printTiming,
                long long toolTimeoutMs,
                long long *elapsedMs = nullptr,
                ProcessResult *processOut = nullptr) {
    (void)errorMessage;
    const auto started = std::chrono::steady_clock::now();
    ProcessResult process;
    ProcessOptions options;
    options.timeoutMs = toolTimeoutMs;
    options.captureOutput = true;
    options.maxCaptureBytes = 8192;
    const bool ok = runProcessCommand(args, process, options);
    const auto finished = std::chrono::steady_clock::now();
    const auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(finished - started).count();
    if (elapsedMs != nullptr) {
        *elapsedMs = durationMs;
    }
    if (printTiming) {
        std::cout << "[aymc] etapa " << stageLabel << ": " << durationMs << " ms" << std::endl;
    }
    if (processOut != nullptr) {
        *processOut = process;
    }
    if (!ok) {
        return false;
    }
    return true;
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

const char *pipelineModeLabel(CodegenPipelineMode mode) {
    switch (mode) {
        case CodegenPipelineMode::Full:
            return "full";
        case CodegenPipelineMode::CompileOnly:
            return "compile-only";
        case CodegenPipelineMode::LinkOnly:
            return "link-only";
    }
    return "unknown";
}

bool writePipelineMetricsJson(const fs::path &jsonPath,
                              CodegenPipelineMode mode,
                              bool windowsTarget,
                              long long toolTimeoutMs,
                              long long asmMs,
                              long long runtimeMs,
                              long long linkMs,
                              long long totalMs,
                              const std::string &runtimeCStatus,
                              const std::string &runtimeMathStatus,
                              const fs::path &asmPath,
                              const fs::path &objPath,
                              const fs::path &binPath,
                              const std::vector<PipelineCommandTrace> &commands,
                              const PipelineFailureInfo &failure,
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

    ec.clear();
    const bool asmExists = !asmPath.empty() && fs::exists(asmPath, ec);
    ec.clear();
    const bool objExists = !objPath.empty() && fs::exists(objPath, ec);
    ec.clear();
    const bool binExists = !binPath.empty() && fs::exists(binPath, ec);

    std::ofstream out(jsonPath);
    if (!out.is_open()) {
        error = "No se pudo escribir metrics JSON en: " + jsonPath.string();
        return false;
    }

    size_t okCount = 0;
    size_t failedCount = 0;
    size_t cacheHitCount = 0;
    size_t timedOutCount = 0;
    size_t reasonOkCount = 0;
    size_t reasonNonzeroExitCount = 0;
    size_t reasonTimeoutCount = 0;
    size_t reasonMissingInputCount = 0;
    size_t reasonCacheHitCount = 0;
    size_t reasonInternalErrorCount = 0;
    size_t reasonOtherCount = 0;
    long long commandsElapsedMs = 0;
    for (const auto &trace : commands) {
        if (trace.status == "ok") {
            ++okCount;
        } else if (trace.status == "failed") {
            ++failedCount;
        } else if (trace.status == "cache-hit") {
            ++cacheHitCount;
        }
        if (trace.timedOut) {
            ++timedOutCount;
        }
        if (trace.exitReason == "ok") {
            ++reasonOkCount;
        } else if (trace.exitReason == "nonzero_exit") {
            ++reasonNonzeroExitCount;
        } else if (trace.exitReason == "timeout") {
            ++reasonTimeoutCount;
        } else if (trace.exitReason == "missing_input") {
            ++reasonMissingInputCount;
        } else if (trace.exitReason == "cache_hit") {
            ++reasonCacheHitCount;
        } else if (trace.exitReason == "internal_error") {
            ++reasonInternalErrorCount;
        } else {
            ++reasonOtherCount;
        }
        if (trace.elapsedMs > 0) {
            commandsElapsedMs += trace.elapsedMs;
        }
    }

    out << "{\n";
    out << "  \"schema\": \"aymc.pipeline.v1\",\n";
    out << "  \"mode\": \"" << pipelineModeLabel(mode) << "\",\n";
    out << "  \"windows_target\": " << (windowsTarget ? "true" : "false") << ",\n";
    out << "  \"tool_timeout_ms\": " << toolTimeoutMs << ",\n";
    out << "  \"timing_ms\": {\n";
    out << "    \"assemble\": " << asmMs << ",\n";
    out << "    \"runtime_compile\": " << runtimeMs << ",\n";
    out << "    \"link\": " << linkMs << ",\n";
    out << "    \"total\": " << totalMs << "\n";
    out << "  },\n";
    out << "  \"runtime\": {\n";
    out << "    \"runtime_c\": \"" << jsonEscape(runtimeCStatus) << "\",\n";
    out << "    \"runtime_math\": \"" << jsonEscape(runtimeMathStatus) << "\"\n";
    out << "  },\n";
    out << "  \"artifacts\": {\n";
    out << "    \"asm\": \"" << jsonEscape(asmPath.string()) << "\",\n";
    out << "    \"object\": \"" << jsonEscape(objPath.string()) << "\",\n";
    out << "    \"binary\": \"" << jsonEscape(binPath.string()) << "\",\n";
    out << "    \"asm_exists\": " << (asmExists ? "true" : "false") << ",\n";
    out << "    \"object_exists\": " << (objExists ? "true" : "false") << ",\n";
    out << "    \"binary_exists\": " << (binExists ? "true" : "false") << "\n";
    out << "  },\n";
    out << "  \"commands\": [\n";
    for (size_t i = 0; i < commands.size(); ++i) {
        const auto &trace = commands[i];
        out << "    {\n";
        out << "      \"stage\": \"" << jsonEscape(trace.stage) << "\",\n";
        out << "      \"status\": \"" << jsonEscape(trace.status) << "\",\n";
        out << "      \"exit_reason\": \"" << jsonEscape(trace.exitReason) << "\",\n";
        out << "      \"command\": \"" << jsonEscape(trace.command) << "\",\n";
        out << "      \"exit_code\": " << trace.exitCode << ",\n";
        out << "      \"timed_out\": " << (trace.timedOut ? "true" : "false") << ",\n";
        out << "      \"elapsed_ms\": " << trace.elapsedMs << ",\n";
        out << "      \"detail\": \"" << jsonEscape(trace.detail) << "\",\n";
        out << "      \"stdout_truncated\": " << (trace.stdoutTruncated ? "true" : "false") << ",\n";
        out << "      \"stderr_truncated\": " << (trace.stderrTruncated ? "true" : "false") << ",\n";
        out << "      \"stdout\": \"" << jsonEscape(trace.stdoutText) << "\",\n";
        out << "      \"stderr\": \"" << jsonEscape(trace.stderrText) << "\"\n";
        out << "    }";
        if (i + 1 < commands.size()) {
            out << ",";
        }
        out << "\n";
    }
    out << "  ],\n";
    out << "  \"phase_summary\": {\n";
    out << "    \"commands_total\": " << commands.size() << ",\n";
    out << "    \"ok\": " << okCount << ",\n";
    out << "    \"failed\": " << failedCount << ",\n";
    out << "    \"cache_hit\": " << cacheHitCount << ",\n";
    out << "    \"timed_out\": " << timedOutCount << ",\n";
    out << "    \"reason_ok\": " << reasonOkCount << ",\n";
    out << "    \"reason_nonzero_exit\": " << reasonNonzeroExitCount << ",\n";
    out << "    \"reason_timeout\": " << reasonTimeoutCount << ",\n";
    out << "    \"reason_missing_input\": " << reasonMissingInputCount << ",\n";
    out << "    \"reason_cache_hit\": " << reasonCacheHitCount << ",\n";
    out << "    \"reason_internal_error\": " << reasonInternalErrorCount << ",\n";
    out << "    \"reason_other\": " << reasonOtherCount << ",\n";
    out << "    \"elapsed_ms_sum\": " << commandsElapsedMs << "\n";
    out << "  },\n";
    out << "  \"result\": {\n";
    out << "    \"success\": " << (failure.success ? "true" : "false") << ",\n";
    out << "    \"failed_stage\": \"" << jsonEscape(failure.stage) << "\",\n";
    out << "    \"message\": \"" << jsonEscape(failure.message) << "\",\n";
    out << "    \"command\": \"" << jsonEscape(failure.command) << "\",\n";
    out << "    \"exit_code\": " << failure.exitCode << ",\n";
    out << "    \"detail\": \"" << jsonEscape(failure.detail) << "\"\n";
    out << "  }\n";
    out << "}\n";

    if (!out.good()) {
        error = "Fallo escribiendo metrics JSON en: " + jsonPath.string();
        return false;
    }
    return true;
}

} // namespace

void CodeGenImpl::collectProgramItems(const std::vector<std::unique_ptr<Node>> &nodes) {
    for (const auto &n : nodes) {
        if (auto *fn = dynamic_cast<FunctionStmt*>(n.get())) {
            FunctionInfo info;
            info.name = fn->getName();
            info.params = fn->getParams();
            info.body = fn->getBody();
            assignTrySlots(fn->getBody());
            for (const auto &param : fn->getParams()) {
                info.locals.push_back(param.name);
            }
            for (const auto &p : fn->getParams()) {
                info.stringLocals[p.name] = false;
            }
            collectLocals(fn->getBody(), info.locals, info.stringLocals, info.localTypes);
            functions.push_back(std::move(info));
        } else if (auto *cls = dynamic_cast<ClassStmt*>(n.get())) {
            registerClass(cls);
        } else {
            assignTrySlots(static_cast<Stmt*>(n.get()));
            mainStmts.push_back(static_cast<const Stmt*>(n.get()));
            collectGlobal(static_cast<const Stmt*>(n.get()));
        }
    }
}

void CodeGenImpl::emitRuntimePrelude() {
    out << "extern printf\n";
    out << "extern scanf\n";
    out << "extern strlen\n";
    out << "extern strcmp\n";
    out << "extern aym_random\n";
    out << "extern aym_srand\n";
    out << "extern aym_sleep\n";
    out << "extern aym_term_clear\n";
    out << "extern aym_term_move\n";
    out << "extern aym_term_color\n";
    out << "extern aym_term_reset\n";
    out << "extern aym_term_cursor\n";
    out << "extern aym_key_poll\n";
    out << "extern aym_time_ms\n";
    out << "extern aym_gfx_open\n";
    out << "extern aym_gfx_is_open\n";
    out << "extern aym_gfx_clear\n";
    out << "extern aym_gfx_set_color\n";
    out << "extern aym_gfx_rect4\n";
    out << "extern aym_gfx_rect\n";
    out << "extern aym_gfx_text3\n";
    out << "extern aym_gfx_text\n";
    out << "extern aym_gfx_present\n";
    out << "extern aym_gfx_close\n";
    out << "extern aym_gfx_key_down\n";
    out << "extern aym_set_args\n";
    out << "extern aym_argc\n";
    out << "extern aym_argv_get\n";
    out << "extern aym_assert\n";
    out << "extern aym_hof_map\n";
    out << "extern aym_hof_filter\n";
    out << "extern aym_hof_reduce\n";
    out << "extern aym_fs_read_text\n";
    out << "extern aym_fs_write_text\n";
    out << "extern aym_fs_exists\n";
    out << "extern aym_array_new\n";
    out << "extern aym_array_get\n";
    out << "extern aym_array_set\n";
    out << "extern aym_array_free\n";
    out << "extern aym_array_length\n";
    out << "extern aym_array_push\n";
    out << "extern aym_array_pop\n";
    out << "extern aym_array_remove_at\n";
    out << "extern aym_array_contains_int\n";
    out << "extern aym_array_contains_str\n";
    out << "extern aym_array_find_int\n";
    out << "extern aym_array_find_str\n";
    out << "extern aym_array_sort_int\n";
    out << "extern aym_array_sort_str\n";
    out << "extern aym_array_unique_int\n";
    out << "extern aym_array_unique_str\n";
    out << "extern aym_map_new\n";
    out << "extern aym_map_set\n";
    out << "extern aym_map_get\n";
    out << "extern aym_map_get_default\n";
    out << "extern aym_map_contains\n";
    out << "extern aym_map_size\n";
    out << "extern aym_map_keys\n";
    out << "extern aym_map_values\n";
    out << "extern aym_map_delete\n";
    out << "extern aym_map_key_at\n";
    out << "extern aym_map_value_at\n";
    out << "extern aym_map_value_is_string\n";
    out << "extern aym_map_value_is_string_key\n";
    out << "extern aym_str_concat\n";
    out << "extern aym_str_trim\n";
    out << "extern aym_str_split\n";
    out << "extern aym_str_join\n";
    out << "extern aym_str_replace\n";
    out << "extern aym_str_contains\n";
    out << "extern aym_to_string\n";
    out << "extern aym_to_number\n";
    out << "extern aym_try_push\n";
    out << "extern aym_try_pop\n";
    out << "extern aym_try_env\n";
    out << "extern aym_try_get_exception\n";
    out << "extern aym_throw\n";
    out << "extern aym_exception_new\n";
    out << "extern aym_exception_type\n";
    out << "extern aym_exception_message\n";
    out << "extern setjmp\n";
    out << "section .data\n";
    out << "fmt_int: db \"%ld\",10,0\n";
    out << "fmt_str: db \"%s\",10,0\n";
    out << "fmt_raw: db \"%s\",0\n";
    out << "fmt_int_raw: db \"%ld\",0\n";
    out << "fmt_read_int: db \"%ld\",0\n";
    out << "fmt_read_str: db \"%255s\",0\n";
    out << "input_val: dq 0\n";
    out << "input_buf: times 256 db 0\n";
    out << "print_sep: db \" \",0\n";
    out << "print_term: db 10,0\n";
    out << "list_open: db \"[\",0\n";
    out << "list_sep: db \", \",0\n";
    out << "list_close: db \"]\",0\n";
    out << "list_quote: db 34,0\n";
    out << "map_open: db \"{\",0\n";
    out << "map_sep: db \", \",0\n";
    out << "map_close: db \"}\",0\n";
    out << "map_colon: db \": \",0\n";
    out << "bool_true: db \"utji\",0\n";
    out << "bool_false: db \"janiutji\",0\n";

    for (size_t i = 0; i < strings.size(); ++i) {
        out << "str" << i << ": db " << toAsmBytes(strings[i]) << "\n";
    }
    for (const auto &g : globals) {
        out << g << ": dq 0\n";
    }

    out << "section .text\n";
    out << "global main\n";
}

void CodeGenImpl::emitMainEntry() {
    for (const auto &f : functions) emitFunction(f);

    out << "main:\n";
    out << "    push rbp\n";
    out << "    mov rbp, rsp\n";
    // Preserve callee-saved registers used across generated code.
    out << "    push rbx\n";
    out << "    push r12\n";
    out << "    push r13\n";
    out << "    push r14\n";
    out << "    push r15\n";
    currentParamStrings.clear();
    currentLocalStrings.clear();
    currentParamTypes.clear();
    currentLocalTypes.clear();
    if (!mainStmts.empty()) {
        std::vector<std::string> mainLocals;
        for (const auto *s : mainStmts) {
            collectLocals(s, mainLocals, currentLocalStrings, currentLocalTypes);
        }
    }
    int stackReserve = this->windows ? 40 : 8;
    out << "    sub rsp, " << stackReserve << "\n";
    // Capture argc/argv from process entry for builtins arg_cantidad/arg_obtener.
    out << "    call aym_set_args\n";
    if (seed >= 0) {
        out << "    mov " << reg1(this->windows) << ", " << seed << "\n";
        out << "    call aym_srand\n";
    }
    std::string mainEnd = genLabel("endmain");
    for (const auto *s : mainStmts) emitStmt(s, nullptr, mainEnd);
    out << mainEnd << ":\n";
    out << "    add rsp, " << stackReserve << "\n";
    out << "    pop r15\n";
    out << "    pop r14\n";
    out << "    pop r13\n";
    out << "    pop r12\n";
    out << "    pop rbx\n";
    out << "    pop rbp\n";
    out << "    mov eax,0\n";
    out << "    ret\n";
}

bool CodeGenImpl::assembleAndLinkOutput(const std::string &path,
                                        const std::string &runtimeDirIn,
                                        bool keepAsmIn,
                                        CodegenPipelineMode modeIn,
                                        std::string *errorMessageOut) {
    if (errorMessageOut != nullptr) {
        errorMessageOut->clear();
    }
    auto publishError = [&](const std::string &message) {
        if (errorMessageOut != nullptr) {
            *errorMessageOut = message;
        } else {
            std::cerr << message << std::endl;
        }
    };

    const auto pipelineStart = std::chrono::steady_clock::now();
    long long asmMs = 0;
    long long runtimeCompileMs = 0;
    long long linkMs = 0;
    std::string runtimeCStatus = "skipped";
    std::string runtimeMathStatus = "skipped";
    PipelineFailureInfo failure;
    std::vector<PipelineCommandTrace> commandTraces;

    if (modeIn != CodegenPipelineMode::LinkOnly && !windows)
        out << "section .note.GNU-stack noalloc noexec nowrite progbits\n";

    if (out.is_open()) {
        out.close();
    }

    fs::path asmPath = fs::path(path);
    fs::path outputDir = asmPath.has_parent_path() ? asmPath.parent_path() : fs::current_path();
    if (!outputDir.empty()) {
        fs::create_directories(outputDir);
    }

    fs::path obj = asmPath;
    obj.replace_extension(windows ? ".obj" : ".o");
    fs::path base = asmPath.stem();
    fs::path bin = outputDir / base;
    if (windows) bin.replace_extension(".exe");

    auto writePipelineJsonIfNeeded = [&](long long totalMs, std::string &jsonError) -> bool {
        jsonError.clear();
        if (timePipelineJsonPath.empty()) {
            return true;
        }
        fs::path jsonPath = fs::path(timePipelineJsonPath);
        if (!writePipelineMetricsJson(jsonPath,
                                      modeIn,
                                      windows,
                                      toolTimeoutMs,
                                      asmMs,
                                      runtimeCompileMs,
                                      linkMs,
                                      totalMs,
                                      runtimeCStatus,
                                      runtimeMathStatus,
                                      asmPath,
                                      obj,
                                      modeIn == CodegenPipelineMode::CompileOnly ? fs::path() : bin,
                                      commandTraces,
                                      failure,
                                      jsonError)) {
            return false;
        }
        std::cout << "[aymc] Pipeline JSON generado: " << jsonPath.string() << std::endl;
        return true;
    };

    auto currentTotalMs = [&]() -> long long {
        const auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now - pipelineStart).count();
    };

    auto finalizeFailure = [&]() -> bool {
        if (failure.success) {
            setFailure(failure, "unknown", "Fallo de backend sin detalle.");
        }
        const auto totalMs = currentTotalMs();
        if (timePipeline) {
            std::cout << "[aymc] pipeline(ms): ensamblado=" << asmMs
                      << ", runtime=" << runtimeCompileMs
                      << ", enlace=" << linkMs
                      << ", total=" << totalMs
                      << std::endl;
        }

        std::string jsonError;
        if (!writePipelineJsonIfNeeded(totalMs, jsonError)) {
            publishError(composePipelineFailureMessage(failure) + " | " + jsonError);
            return false;
        }
        publishError(composePipelineFailureMessage(failure));
        return false;
    };

    if (modeIn != CodegenPipelineMode::LinkOnly) {
        std::vector<std::string> cmdAssemble;
        if (windows) {
            cmdAssemble = {"nasm", "-f", "win64", asmPath.string(), "-o", obj.string()};
        } else {
            cmdAssemble = {"nasm", "-felf64", asmPath.string(), "-o", obj.string()};
        }
        ProcessResult process;
        if (!runCommand(cmdAssemble,
                        "Error ensamblando " + path,
                        "ensamblado",
                        timePipeline,
                        toolTimeoutMs,
                        &asmMs,
                        &process)) {
            appendCommandTrace(commandTraces,
                               "ensamblado",
                               "failed",
                               process.command,
                               process.exitCode,
                               process.timedOut,
                               asmMs,
                               process.error,
                               process.stdoutTruncated,
                               process.stderrTruncated,
                               process.stdoutText,
                               process.stderrText);
            setFailure(failure, "ensamblado", "Error ensamblando " + path, &process);
            return finalizeFailure();
        }
        appendCommandTrace(commandTraces,
                           "ensamblado",
                           "ok",
                           process.command,
                           process.exitCode,
                           process.timedOut,
                           asmMs,
                           process.error,
                           process.stdoutTruncated,
                           process.stderrTruncated,
                           process.stdoutText,
                           process.stderrText);
    } else if (!fs::exists(obj)) {
        appendCommandTrace(commandTraces,
                           "pre-link",
                           "failed",
                           "",
                           -1,
                           false,
                           0,
                           "objeto faltante");
        setFailure(failure,
                   "pre-link",
                   "No se encontro el objeto para enlazar: " + obj.string(),
                   nullptr,
                   "objeto faltante");
        return finalizeFailure();
    }

    if (modeIn == CodegenPipelineMode::CompileOnly) {
        if (keepAsmIn) {
            std::cout << "[aymc] ASM generado: " << asmPath.string() << std::endl;
        } else {
            std::error_code ec;
            fs::remove(asmPath, ec);
        }
        const auto pipelineEnd = std::chrono::steady_clock::now();
        const auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(pipelineEnd - pipelineStart).count();
        if (timePipeline) {
            std::cout << "[aymc] pipeline(ms): ensamblado=" << asmMs
                      << ", runtime=0"
                  << ", enlace=0"
                  << ", total=" << totalMs
                  << std::endl;
        }
        std::string jsonError;
        if (!writePipelineJsonIfNeeded(totalMs, jsonError)) {
            publishError(jsonError);
            return false;
        }
        std::cout << "[aymc] Objeto generado: " << obj.string() << std::endl;
        return true;
    }

    std::vector<std::string> cmd2;
    fs::path runtimeDir = runtimeDirIn.empty() ? fs::path("runtime") : fs::path(runtimeDirIn);
    fs::path runtimeC = runtimeDir / "runtime.c";
    fs::path mathC = runtimeDir / "math.c";
    fs::path cacheDir = runtimeCacheDir(outputDir);
    std::string runtimeTag = runtimeCacheKey(runtimeDir, windows);
    fs::path runtimeObj = cacheDir / ("__aym_runtime_" + runtimeTag + (windows ? ".obj" : ".o"));
    fs::path mathObj = cacheDir / ("__aym_math_" + runtimeTag + (windows ? ".obj" : ".o"));
    auto compileRuntimeObject = [&](const fs::path &source,
                                    const fs::path &targetObj,
                                    const std::string &label,
                                    std::string &status) {
        if (!fs::exists(source)) {
            appendCommandTrace(commandTraces,
                               label,
                               "failed",
                               "",
                               -1,
                               false,
                               0,
                               "fuente runtime faltante");
            setFailure(failure,
                       label,
                       "No se encontro runtime fuente: " + source.string(),
                       nullptr,
                       "fuente runtime faltante");
            return false;
        }
        if (!needsRebuildObject(source, targetObj)) {
            status = "cache-hit";
            appendCommandTrace(commandTraces,
                               label,
                               "cache-hit",
                               "",
                               0,
                               false,
                               0,
                               "cache-hit");
            if (timePipeline) {
                std::cout << "[aymc] etapa " << label << ": cache-hit" << std::endl;
            }
            return true;
        }
        const std::vector<std::string> compileCmd = {"gcc", "-c", source.string(), "-o", targetObj.string()};
        long long commandMs = 0;
        ProcessResult process;
        if (!runCommand(compileCmd,
                        "Error compilando runtime: " + source.string(),
                        label,
                        timePipeline,
                        toolTimeoutMs,
                        &commandMs,
                        &process)) {
            appendCommandTrace(commandTraces,
                               label,
                               "failed",
                               process.command,
                               process.exitCode,
                               process.timedOut,
                               commandMs,
                               process.error,
                               process.stdoutTruncated,
                               process.stderrTruncated,
                               process.stdoutText,
                               process.stderrText);
            setFailure(failure, label, "Error compilando runtime: " + source.string(), &process);
            return false;
        }
        appendCommandTrace(commandTraces,
                           label,
                           "ok",
                           process.command,
                           process.exitCode,
                           process.timedOut,
                           commandMs,
                           process.error,
                           process.stdoutTruncated,
                           process.stderrTruncated,
                           process.stdoutText,
                           process.stderrText);
        status = "compiled";
        runtimeCompileMs += commandMs;
        return true;
    };
    if (!compileRuntimeObject(runtimeC, runtimeObj, "runtime-c", runtimeCStatus) ||
        !compileRuntimeObject(mathC, mathObj, "runtime-math", runtimeMathStatus)) {
        return finalizeFailure();
    }
    if (windows)
        cmd2 = {"gcc", obj.string(), runtimeObj.string(), mathObj.string(), "-o", bin.string(), "-lm", "-lgdi32", "-luser32"};
    else
        cmd2 = {"gcc", "-no-pie", obj.string(), runtimeObj.string(), mathObj.string(), "-o", bin.string(), "-lm", "-lc"};
    ProcessResult linkProcess;
    if (!runCommand(cmd2,
                    "Error enlazando " + obj.string(),
                    "enlace",
                    timePipeline,
                    toolTimeoutMs,
                    &linkMs,
                    &linkProcess)) {
        appendCommandTrace(commandTraces,
                           "enlace",
                           "failed",
                           linkProcess.command,
                           linkProcess.exitCode,
                           linkProcess.timedOut,
                           linkMs,
                           linkProcess.error,
                           linkProcess.stdoutTruncated,
                           linkProcess.stderrTruncated,
                           linkProcess.stdoutText,
                           linkProcess.stderrText);
        setFailure(failure, "enlace", "Error enlazando " + obj.string(), &linkProcess);
        return finalizeFailure();
    }
    appendCommandTrace(commandTraces,
                       "enlace",
                       "ok",
                       linkProcess.command,
                       linkProcess.exitCode,
                       linkProcess.timedOut,
                       linkMs,
                       linkProcess.error,
                       linkProcess.stdoutTruncated,
                       linkProcess.stderrTruncated,
                       linkProcess.stdoutText,
                       linkProcess.stderrText);
    if (windows && !keepAsmIn && modeIn == CodegenPipelineMode::Full) {
        std::error_code ec;
        fs::remove(asmPath, ec);
        fs::remove(obj, ec);
    }
    if (keepAsmIn && modeIn == CodegenPipelineMode::Full) {
        std::cout << "[aymc] ASM generado: " << asmPath.string() << std::endl;
    }

    const auto pipelineEnd = std::chrono::steady_clock::now();
    const auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(pipelineEnd - pipelineStart).count();
    if (timePipeline) {
        std::cout << "[aymc] pipeline(ms): ensamblado=" << asmMs
                  << ", runtime=" << runtimeCompileMs
                  << ", enlace=" << linkMs
                  << ", total=" << totalMs
                  << std::endl;
    }
    std::string jsonError;
    if (!writePipelineJsonIfNeeded(totalMs, jsonError)) {
        publishError(jsonError);
        return false;
    }

    std::cout << "[aymc] Ejecutable generado: " << bin.string() << std::endl;
    return true;
}

} // namespace aym
