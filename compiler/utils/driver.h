#ifndef AYM_DRIVER_H
#define AYM_DRIVER_H

#include <string>
#include <vector>
#include "fs.h"

namespace aym {

class ModuleResolver;

struct CompileOptions {
    std::vector<std::string> inputs;
    std::string output;
    std::string backend = "native";
    bool outputProvided = false;
    bool debug = false;
    bool dumpAst = false;
    bool checkOnly = false;
    bool emitAsm = false;
    bool compileOnly = false;
    bool linkOnly = false;
    bool timePipeline = false;
    bool emitTimePipelineJson = false;
    std::string timePipelineJsonPath;
    long long toolTimeoutMs = 0;
    bool emitAstJson = false;
    std::string astJsonPath;
    bool emitDiagnosticsJson = false;
    std::string diagnosticsJsonPath;
    bool checkManifest = false;
    bool emitLock = false;
    bool checkLock = false;
    std::string manifestPath;
    std::string lockPath;
    bool windowsTarget = false;
    long seed = 0;
    bool seedProvided = false;
};

enum class CliParseResult {
    Ok,
    ShowHelp,
    Error
};

CompileOptions makeDefaultCompileOptions();
CliParseResult parseCompileOptions(int argc, char **argv, CompileOptions &options, std::string &errorMsg);
const char *compileHelpText();
void finalizeOutputPath(CompileOptions &options);
bool loadInputSources(const std::vector<std::string> &inputs, std::string &source, std::string &failedPath);
fs::path detectEntryDirectory(const std::vector<std::string> &inputs);
void registerInputSearchPaths(const std::vector<std::string> &inputs, ModuleResolver &resolver);
fs::path findRuntimeDirectory();

} // namespace aym

#endif // AYM_DRIVER_H
