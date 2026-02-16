#ifndef AYM_PROCESS_H
#define AYM_PROCESS_H

#include <cstddef>
#include <string>
#include <vector>

namespace aym {

struct ProcessOptions {
    long long timeoutMs = 0;
    bool captureOutput = false;
    size_t maxCaptureBytes = 8192;
};

struct ProcessResult {
    int exitCode = 0;
    bool timedOut = false;
    bool stdoutTruncated = false;
    bool stderrTruncated = false;
    std::string command;
    std::string error;
    std::string stdoutText;
    std::string stderrText;
};

std::string formatProcessCommand(const std::vector<std::string> &args);
bool runProcessCommand(const std::vector<std::string> &args, ProcessResult &result);
bool runProcessCommand(const std::vector<std::string> &args,
                       ProcessResult &result,
                       const ProcessOptions &options);

} // namespace aym

#endif // AYM_PROCESS_H
