#include "codegen_impl.h"
#include <iostream>

namespace aym {

void CodeGenImpl::emitInput(bool asString) {
    if (asString) {
        out << "    lea " << reg1(this->windows) << ", [rel fmt_read_str]\n";
        out << "    lea " << reg2(this->windows) << ", [rel input_buf]\n";
        out << "    xor eax,eax\n";
        out << "    call scanf\n";
        out << "    lea rax, [rel input_buf]\n";
    } else {
        out << "    lea " << reg1(this->windows) << ", [rel fmt_read_int]\n";
        out << "    lea " << reg2(this->windows) << ", [rel input_val]\n";
        out << "    xor eax,eax\n";
        out << "    call scanf\n";
        out << "    mov rax, [rel input_val]\n";
    }
}

bool CodeGenImpl::emit(const std::vector<std::unique_ptr<Node>> &nodes,
                       const std::string &path,
                       const std::unordered_set<std::string> &semGlobals,
                       const std::unordered_map<std::string,std::vector<std::string>> &paramTypesIn,
                       const std::unordered_map<std::string,std::string> &functionReturnTypesIn,
                       const std::unordered_map<std::string,std::string> &globalTypesIn,
                       bool windowsTarget,
                       long seedIn,
                       const std::string &runtimeDirIn,
                       bool keepAsmIn,
                       CodegenPipelineMode modeIn,
                       bool timePipelineIn,
                       const std::string &timePipelineJsonPathIn,
                       long long toolTimeoutMsIn,
                       std::string *errorMessageOut) {
    if (errorMessageOut != nullptr) {
        errorMessageOut->clear();
    }
    this->windows = windowsTarget;
    globals = semGlobals;
    paramTypes = paramTypesIn;
    functionReturnTypes = functionReturnTypesIn;
    globalTypes = globalTypesIn;
    seed = seedIn;
    keepAsm = keepAsmIn;
    pipelineMode = modeIn;
    timePipeline = timePipelineIn;
    timePipelineJsonPath = timePipelineJsonPathIn;
    toolTimeoutMs = toolTimeoutMsIn;
    functions.clear();
    mainStmts.clear();
    classes.clear();
    strings.clear();
    tryTempCounter = 0;

    if (pipelineMode != CodegenPipelineMode::LinkOnly) {
        collectProgramItems(nodes);
        collectClassStrings();

        std::ofstream fout(path);
        if (!fout.is_open()) {
            const std::string openError = "No se pudo generar ASM en '" + path + "'";
            if (errorMessageOut != nullptr) {
                *errorMessageOut = openError;
            } else {
                std::cerr << openError << std::endl;
            }
            return false;
        }
        out.swap(fout);

        emitRuntimePrelude();
        emitMainEntry();
    }
    return assembleAndLinkOutput(path, runtimeDirIn, keepAsmIn, modeIn, errorMessageOut);
}



} // namespace aym
