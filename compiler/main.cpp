#include "lexer/lexer.h"
#include "parser/parser.h"
#include "ast/ast_json.h"
#include "backend/backend.h"
#include "codegen/codegen.h"
#include "utils/utils.h"
#include "utils/driver.h"
#include "utils/diagnostic.h"
#include "utils/project_manifest.h"
#include "semantic/semantic.h"
#include "utils/error.h"
#include "utils/module_resolver.h"
#include <iostream>
#include <vector>
#include <string>
#include <exception>
#include <unordered_map>
#include <unordered_set>

int main(int argc, char** argv) {
    try {
        aym::CompileOptions options = aym::makeDefaultCompileOptions();
        std::string parseError;
        const auto parseResult = aym::parseCompileOptions(argc, argv, options, parseError);
        if (parseResult == aym::CliParseResult::ShowHelp) {
            std::cout << aym::compileHelpText();
            return 0;
        }
        if (parseResult == aym::CliParseResult::Error) {
            aym::error(parseError);
            return 1;
        }
        aym::BackendKind backendKind = aym::BackendKind::Native;
        std::string backendParseError;
        if (!aym::parseBackendKind(options.backend, backendKind, backendParseError)) {
            aym::error(backendParseError);
            return 1;
        }

        aym::finalizeOutputPath(options);

        aym::DiagnosticEngine diagnostics;
        auto emitDiagnosticsIfRequested = [&]() -> bool {
            if (!options.emitDiagnosticsJson) {
                return true;
            }
            const std::string diagnosticsBase = options.output.empty() ? "aymc" : options.output;
            const std::string diagnosticsPath = options.diagnosticsJsonPath.empty()
                                              ? (diagnosticsBase + ".diagnostics.json")
                                              : options.diagnosticsJsonPath;
            std::string diagError;
            if (!diagnostics.writeJsonFile(diagnosticsPath, diagError)) {
                aym::error(diagError);
                return false;
            }
            std::cout << "[aymc] Diagnostics JSON generado: " << diagnosticsPath << std::endl;
            return true;
        };

        if (options.checkManifest || options.emitLock || options.checkLock) {
            const std::string manifestPath = options.manifestPath.empty()
                                           ? aym::defaultManifestPath()
                                           : options.manifestPath;
            const std::string lockPath = options.lockPath.empty()
                                       ? aym::defaultLockPathForManifest(manifestPath)
                                       : options.lockPath;

            aym::ProjectManifest manifest;
            bool manifestLoaded = false;
            if (options.checkManifest || options.emitLock) {
                aym::ManifestError manifestError;
                if (!aym::parseProjectManifestFileDetailed(manifestPath, manifest, manifestError)) {
                    diagnostics.clear();
                    diagnostics.error(manifestError.code.empty() ? "AYM5001" : manifestError.code,
                                      manifestError.message,
                                      manifestError.line,
                                      manifestError.column);
                    diagnostics.printAll(std::cerr);
                    if (!emitDiagnosticsIfRequested()) {
                        return 1;
                    }
                    return 1;
                }
                manifestLoaded = true;
                std::cout << "[aymc] Manifest valido: " << manifestPath << std::endl;
            }

            if (options.emitLock) {
                aym::ManifestError lockError;
                if (!aym::writeProjectLockFileDetailed(manifest, lockPath, lockError)) {
                    diagnostics.clear();
                    diagnostics.error(lockError.code.empty() ? "AYM5004" : lockError.code,
                                      lockError.message,
                                      lockError.line,
                                      lockError.column);
                    diagnostics.printAll(std::cerr);
                    if (!emitDiagnosticsIfRequested()) {
                        return 1;
                    }
                    return 1;
                }
                std::cout << "[aymc] Lockfile generado: " << lockPath << std::endl;
            }

            if (options.checkLock) {
                aym::ProjectLock lock;
                aym::ManifestError lockParseError;
                if (!aym::parseProjectLockFileDetailed(lockPath, lock, lockParseError)) {
                    diagnostics.clear();
                    diagnostics.error(lockParseError.code.empty() ? "AYM5005" : lockParseError.code,
                                      lockParseError.message,
                                      lockParseError.line,
                                      lockParseError.column);
                    diagnostics.printAll(std::cerr);
                    if (!emitDiagnosticsIfRequested()) {
                        return 1;
                    }
                    return 1;
                }
                std::cout << "[aymc] Lockfile valido: " << lockPath << std::endl;

                if (manifestLoaded) {
                    aym::ManifestError lockVerifyError;
                    if (!aym::verifyLockAgainstManifestDetailed(manifest, lock, lockVerifyError)) {
                        diagnostics.clear();
                        diagnostics.error(lockVerifyError.code.empty() ? "AYM5007" : lockVerifyError.code,
                                          lockVerifyError.message,
                                          lockVerifyError.line,
                                          lockVerifyError.column);
                        diagnostics.printAll(std::cerr);
                        if (!emitDiagnosticsIfRequested()) {
                            return 1;
                        }
                        return 1;
                    }
                    std::cout << "[aymc] Manifest y lockfile consistentes." << std::endl;
                }
            }

            if (options.inputs.empty()) {
                diagnostics.clear();
                if (!emitDiagnosticsIfRequested()) {
                    return 1;
                }
                return 0;
            }
        }

        aym::CodegenPipelineMode pipelineMode = aym::CodegenPipelineMode::Full;
        if (options.compileOnly) {
            pipelineMode = aym::CodegenPipelineMode::CompileOnly;
        } else if (options.linkOnly) {
            pipelineMode = aym::CodegenPipelineMode::LinkOnly;
        }
        const std::string pipelineMetricsBase = options.output.empty() ? "aymc" : options.output;
        const std::string pipelineMetricsJsonPath = options.emitTimePipelineJson
                                                  ? (options.timePipelineJsonPath.empty()
                                                      ? (pipelineMetricsBase + ".pipeline.json")
                                                      : options.timePipelineJsonPath)
                                                  : "";

        if (options.linkOnly) {
            std::string runtimeDirString;
            if (backendKind == aym::BackendKind::Native) {
                fs::path runtimeDir = aym::findRuntimeDirectory();
                if (!fs::exists(runtimeDir)) {
                    aym::error("No se encontro el runtime. Busca runtime/runtime.c junto al compilador o instala el paquete completo.");
                    return 1;
                }
                runtimeDirString = runtimeDir.string();
            }

            const std::vector<std::unique_ptr<aym::Node>> emptyNodes;
            const std::unordered_set<std::string> emptyGlobals;
            const std::unordered_map<std::string, std::vector<std::string>> emptyParamTypes;
            const std::unordered_map<std::string, std::string> emptyFunctionReturnTypes;
            const std::unordered_map<std::string, std::string> emptyGlobalTypes;
            std::string backendError;
            bool ok = aym::runBackendCompile(backendKind,
                                             emptyNodes,
                                             options.output + ".asm",
                                             emptyGlobals,
                                             emptyParamTypes,
                                             emptyFunctionReturnTypes,
                                             emptyGlobalTypes,
                                             options.windowsTarget,
                                             options.seedProvided ? options.seed : -1,
                                             runtimeDirString,
                                             options.emitAsm,
                                             pipelineMode,
                                             options.timePipeline,
                                             pipelineMetricsJsonPath,
                                             options.toolTimeoutMs,
                                             backendError);
            if (!ok) {
                if (!backendError.empty()) {
                    aym::error(backendError);
                } else {
                    aym::error("Fallo el enlace nativo.");
                }
                return 1;
            }

            if (!emitDiagnosticsIfRequested()) {
                return 1;
            }
            return 0;
        }

        std::string source;
        std::string failedPath;
        if (!aym::loadInputSources(options.inputs, source, failedPath)) {
            aym::error("No se pudo abrir el archivo: " + failedPath);
            return 1;
        }

        aym::Lexer lexer(source);
        auto tokens = lexer.tokenize();
        if (options.debug) {
            for (const auto &t : tokens) std::cout << static_cast<int>(t.type) << ":" << t.text << std::endl;
        }

        aym::Parser parser(tokens, &diagnostics);
        auto nodes = parser.parse();
        if (parser.hasError()) {
            diagnostics.printAll(std::cerr);
            if (!emitDiagnosticsIfRequested()) {
                return 1;
            }
            return 1;
        }

        fs::path entryDir = aym::detectEntryDirectory(options.inputs);
        aym::ModuleResolver resolver(entryDir);
        aym::registerInputSearchPaths(options.inputs, resolver);
        try {
            resolver.resolve(nodes, entryDir);
        } catch (const aym::ModuleResolverError &ex) {
            diagnostics.clear();
            diagnostics.error(ex.code(), ex.what(), ex.line(), ex.column());
            diagnostics.printAll(std::cerr);
            if (!emitDiagnosticsIfRequested()) {
                return 1;
            }
            return 1;
        } catch (const std::exception &ex) {
            diagnostics.clear();
            diagnostics.error("AYM4001", ex.what());
            diagnostics.printAll(std::cerr);
            if (!emitDiagnosticsIfRequested()) {
                return 1;
            }
            return 1;
        }
        if (options.dumpAst) {
            std::cout << "AST nodos: " << nodes.size() << std::endl;
        }
        if (options.emitAstJson) {
            const std::string astJsonPath = options.astJsonPath.empty()
                                          ? (options.output + ".ast.json")
                                          : options.astJsonPath;
            std::string astJsonError;
            if (!aym::writeAstJson(nodes, astJsonPath, astJsonError)) {
                aym::error(astJsonError);
                return 1;
            }
            std::cout << "[aymc] AST JSON generado: " << astJsonPath << std::endl;
        }

        aym::SemanticAnalyzer sem;
        diagnostics.clear();
        sem.setDiagnosticEngine(&diagnostics);
        sem.analyze(nodes);
        if (sem.hasErrors()) {
            diagnostics.printAll(std::cerr);
            if (!emitDiagnosticsIfRequested()) {
                return 1;
            }
            return 1;
        }

        if (options.checkOnly) {
            if (!emitDiagnosticsIfRequested()) {
                return 1;
            }
            std::cout << "[aymc] Validacion completada sin errores." << std::endl;
            return 0;
        }

        std::string runtimeDirString;
        if (backendKind == aym::BackendKind::Native &&
            pipelineMode != aym::CodegenPipelineMode::CompileOnly) {
            fs::path runtimeDir = aym::findRuntimeDirectory();
            if (!fs::exists(runtimeDir)) {
                aym::error("No se encontro el runtime. Busca runtime/runtime.c junto al compilador o instala el paquete completo.");
                return 1;
            }
            runtimeDirString = runtimeDir.string();
        }

        std::string backendError;
        bool ok = aym::runBackendCompile(backendKind,
                                         nodes,
                                         options.output + ".asm",
                                         sem.getGlobals(),
                                         sem.getParamTypes(),
                                         sem.getFunctionReturnTypes(),
                                         sem.getGlobalTypes(),
                                         options.windowsTarget,
                                         options.seedProvided ? options.seed : -1,
                                         runtimeDirString,
                                         options.emitAsm,
                                         pipelineMode,
                                         options.timePipeline,
                                         pipelineMetricsJsonPath,
                                         options.toolTimeoutMs,
                                         backendError);
        if (!ok) {
            if (!backendError.empty()) {
                aym::error(backendError);
            } else if (pipelineMode == aym::CodegenPipelineMode::CompileOnly) {
                aym::error("Fallo la compilacion del backend (ASM/objeto).");
            } else {
                aym::error("Fallo la compilacion del backend (ensamblado/enlace).");
            }
            return 1;
        }

        if (!emitDiagnosticsIfRequested()) {
            return 1;
        }

        return 0;
    } catch (const std::exception &ex) {
        aym::error(ex.what());
        return 1;
    } catch (...) {
        aym::error("Fallo interno no controlado durante compilacion.");
        return 1;
    }
}
