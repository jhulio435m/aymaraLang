#include "process.h"
#include "fs.h"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <fstream>
#include <limits>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <cerrno>
#include <windows.h>
#else
#include <cerrno>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace aym {

namespace {

std::string quoteArgForDisplay(const std::string &value) {
    bool needsQuotes = value.find(' ') != std::string::npos ||
                       value.find('\t') != std::string::npos ||
                       value.find('"') != std::string::npos;
    if (!needsQuotes) {
        return value;
    }
    std::string out = "\"";
    for (char ch : value) {
        if (ch == '"') {
            out += "\\\"";
        } else {
            out.push_back(ch);
        }
    }
    out.push_back('"');
    return out;
}

#ifdef _WIN32
std::string quoteArgForProcess(const std::string &value) {
    if (value.empty()) {
        return "\"\"";
    }
    bool needsQuotes = false;
    for (char ch : value) {
        if (ch == ' ' || ch == '\t' || ch == '"') {
            needsQuotes = true;
            break;
        }
    }
    if (!needsQuotes) {
        return value;
    }

    std::string out;
    out.reserve(value.size() + 2);
    out.push_back('"');
    size_t slashCount = 0;
    for (char ch : value) {
        if (ch == '\\') {
            ++slashCount;
            continue;
        }
        if (ch == '"') {
            out.append(slashCount * 2 + 1, '\\');
            out.push_back('"');
            slashCount = 0;
            continue;
        }
        if (slashCount > 0) {
            out.append(slashCount, '\\');
            slashCount = 0;
        }
        out.push_back(ch);
    }
    if (slashCount > 0) {
        out.append(slashCount * 2, '\\');
    }
    out.push_back('"');
    return out;
}

std::string buildWindowsCommandLine(const std::vector<std::string> &args) {
    std::string commandLine;
    bool first = true;
    for (const auto &arg : args) {
        if (!first) {
            commandLine.push_back(' ');
        }
        first = false;
        commandLine += quoteArgForProcess(arg);
    }
    return commandLine;
}
#endif

unsigned long long currentProcessIdValue() {
#ifdef _WIN32
    return static_cast<unsigned long long>(GetCurrentProcessId());
#else
    return static_cast<unsigned long long>(getpid());
#endif
}

fs::path captureBaseDirectory() {
    std::error_code ec;
    fs::path base = fs::temp_directory_path(ec);
    if (!ec && !base.empty()) {
        return base;
    }
    ec.clear();
    base = fs::current_path(ec);
    if (!ec && !base.empty()) {
        return base;
    }
    return fs::path(".");
}

fs::path createCaptureFilePath(const std::string &prefix, std::string &error) {
    error.clear();
    static std::atomic<unsigned long long> counter{0};
    const fs::path base = captureBaseDirectory();
    for (int attempt = 0; attempt < 64; ++attempt) {
        const auto stamp = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        const unsigned long long serial = counter.fetch_add(1, std::memory_order_relaxed);
        const fs::path candidate = base / (prefix + "_" +
                                           std::to_string(currentProcessIdValue()) + "_" +
                                           std::to_string(stamp) + "_" +
                                           std::to_string(serial) + ".tmp");
        std::ofstream out(candidate, std::ios::binary | std::ios::trunc);
        if (out.is_open()) {
            out.close();
            return candidate;
        }
    }
    error = "no se pudo crear archivo temporal de captura para proceso";
    return fs::path();
}

void cleanupCaptureFile(const fs::path &path) {
    if (path.empty()) {
        return;
    }
    std::error_code ec;
    fs::remove(path, ec);
}

bool readCaptureFile(const fs::path &path,
                     size_t maxBytes,
                     std::string &text,
                     bool &truncated,
                     std::string &error) {
    text.clear();
    truncated = false;
    error.clear();
    if (path.empty()) {
        return true;
    }

    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) {
        error = "no se pudo leer archivo de captura: " + path.string();
        return false;
    }

    std::vector<char> buffer(4096);
    while (in.good()) {
        in.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const std::streamsize n = in.gcount();
        if (n <= 0) {
            break;
        }
        size_t toAppend = 0;
        if (text.size() < maxBytes) {
            const size_t remaining = maxBytes - text.size();
            toAppend = static_cast<size_t>(n) > remaining ? remaining : static_cast<size_t>(n);
            text.append(buffer.data(), toAppend);
        }
        if (static_cast<size_t>(n) > toAppend) {
            truncated = true;
        }
        if (text.size() >= maxBytes) {
            truncated = true;
            break;
        }
    }
    return true;
}

bool finalizeCaptureOutput(const ProcessOptions &options,
                           const fs::path &stdoutPath,
                           const fs::path &stderrPath,
                           ProcessResult &result,
                           std::string &error) {
    error.clear();
    if (!options.captureOutput) {
        return true;
    }
    const size_t maxBytes = options.maxCaptureBytes;

    std::string readError;
    bool okStdout = readCaptureFile(stdoutPath,
                                    maxBytes,
                                    result.stdoutText,
                                    result.stdoutTruncated,
                                    readError);
    if (!okStdout && error.empty()) {
        error = readError;
    }
    readError.clear();
    bool okStderr = readCaptureFile(stderrPath,
                                    maxBytes,
                                    result.stderrText,
                                    result.stderrTruncated,
                                    readError);
    if (!okStderr && error.empty()) {
        error = readError;
    }

    cleanupCaptureFile(stdoutPath);
    cleanupCaptureFile(stderrPath);
    return okStdout && okStderr;
}

} // namespace

std::string formatProcessCommand(const std::vector<std::string> &args) {
    std::string command;
    bool first = true;
    for (const auto &arg : args) {
        if (!first) {
            command.push_back(' ');
        }
        first = false;
        command += quoteArgForDisplay(arg);
    }
    return command;
}

bool runProcessCommand(const std::vector<std::string> &args, ProcessResult &result) {
    ProcessOptions options;
    return runProcessCommand(args, result, options);
}

bool runProcessCommand(const std::vector<std::string> &args,
                       ProcessResult &result,
                       const ProcessOptions &options) {
    result = {};
    result.command = formatProcessCommand(args);

    if (args.empty()) {
        result.exitCode = -1;
        result.error = "comando vacio";
        return false;
    }

    std::string capturePathError;
    const fs::path stdoutCapturePath = options.captureOutput
                                     ? createCaptureFilePath("aym_stdout", capturePathError)
                                     : fs::path();
    if (options.captureOutput && stdoutCapturePath.empty()) {
        result.exitCode = -1;
        result.error = capturePathError.empty()
                     ? "no se pudo preparar captura de stdout"
                     : capturePathError;
        return false;
    }
    const fs::path stderrCapturePath = options.captureOutput
                                     ? createCaptureFilePath("aym_stderr", capturePathError)
                                     : fs::path();
    if (options.captureOutput && stderrCapturePath.empty()) {
        cleanupCaptureFile(stdoutCapturePath);
        result.exitCode = -1;
        result.error = capturePathError.empty()
                     ? "no se pudo preparar captura de stderr"
                     : capturePathError;
        return false;
    }

    auto finalizeCaptureWithMessage = [&](bool processOk) -> bool {
        std::string captureError;
        const bool captured = finalizeCaptureOutput(options,
                                                    stdoutCapturePath,
                                                    stderrCapturePath,
                                                    result,
                                                    captureError);
        if (!captured) {
            if (processOk) {
                result.exitCode = -1;
                result.error = captureError;
                return false;
            }
            if (!captureError.empty()) {
                if (result.error.empty()) {
                    result.error = captureError;
                } else {
                    result.error += " | " + captureError;
                }
            }
        }
        return processOk;
    };

#ifdef _WIN32
    STARTUPINFOA startupInfo;
    PROCESS_INFORMATION processInfo;
    ZeroMemory(&startupInfo, sizeof(startupInfo));
    ZeroMemory(&processInfo, sizeof(processInfo));
    startupInfo.cb = sizeof(startupInfo);

    HANDLE stdoutHandle = INVALID_HANDLE_VALUE;
    HANDLE stderrHandle = INVALID_HANDLE_VALUE;
    bool inheritHandles = FALSE;
    if (options.captureOutput) {
        SECURITY_ATTRIBUTES security;
        security.nLength = sizeof(security);
        security.lpSecurityDescriptor = nullptr;
        security.bInheritHandle = TRUE;

        stdoutHandle = CreateFileA(stdoutCapturePath.string().c_str(),
                                   GENERIC_WRITE,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   &security,
                                   CREATE_ALWAYS,
                                   FILE_ATTRIBUTE_NORMAL,
                                   nullptr);
        if (stdoutHandle == INVALID_HANDLE_VALUE) {
            cleanupCaptureFile(stdoutCapturePath);
            cleanupCaptureFile(stderrCapturePath);
            result.exitCode = -1;
            result.error = std::string("fallo CreateFile stdout capture: code=") +
                           std::to_string(GetLastError());
            return false;
        }
        stderrHandle = CreateFileA(stderrCapturePath.string().c_str(),
                                   GENERIC_WRITE,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   &security,
                                   CREATE_ALWAYS,
                                   FILE_ATTRIBUTE_NORMAL,
                                   nullptr);
        if (stderrHandle == INVALID_HANDLE_VALUE) {
            CloseHandle(stdoutHandle);
            cleanupCaptureFile(stdoutCapturePath);
            cleanupCaptureFile(stderrCapturePath);
            result.exitCode = -1;
            result.error = std::string("fallo CreateFile stderr capture: code=") +
                           std::to_string(GetLastError());
            return false;
        }

        startupInfo.dwFlags |= STARTF_USESTDHANDLES;
        startupInfo.hStdOutput = stdoutHandle;
        startupInfo.hStdError = stderrHandle;
        startupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        inheritHandles = TRUE;
    }

    std::string commandLine = buildWindowsCommandLine(args);
    std::vector<char> mutableCommandLine(commandLine.begin(), commandLine.end());
    mutableCommandLine.push_back('\0');

    const BOOL created = CreateProcessA(nullptr,
                                        mutableCommandLine.data(),
                                        nullptr,
                                        nullptr,
                                        inheritHandles,
                                        0,
                                        nullptr,
                                        nullptr,
                                        &startupInfo,
                                        &processInfo);
    if (stdoutHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(stdoutHandle);
    }
    if (stderrHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(stderrHandle);
    }

    if (!created) {
        cleanupCaptureFile(stdoutCapturePath);
        cleanupCaptureFile(stderrCapturePath);
        result.exitCode = -1;
        result.error = std::string("fallo CreateProcess: code=") + std::to_string(GetLastError());
        return false;
    }

    DWORD waitTimeout = INFINITE;
    if (options.timeoutMs > 0) {
        if (options.timeoutMs > static_cast<long long>((std::numeric_limits<DWORD>::max)())) {
            waitTimeout = (std::numeric_limits<DWORD>::max)();
        } else {
            waitTimeout = static_cast<DWORD>(options.timeoutMs);
        }
    }

    const DWORD waitResult = WaitForSingleObject(processInfo.hProcess, waitTimeout);
    if (waitResult == WAIT_TIMEOUT) {
        TerminateProcess(processInfo.hProcess, 124);
        WaitForSingleObject(processInfo.hProcess, 5000);
        CloseHandle(processInfo.hThread);
        CloseHandle(processInfo.hProcess);
        result.exitCode = 124;
        result.timedOut = true;
        result.error = "proceso excedio timeout de " + std::to_string(options.timeoutMs) + " ms";
        return finalizeCaptureWithMessage(false);
    }
    if (waitResult != WAIT_OBJECT_0) {
        CloseHandle(processInfo.hThread);
        CloseHandle(processInfo.hProcess);
        cleanupCaptureFile(stdoutCapturePath);
        cleanupCaptureFile(stderrCapturePath);
        result.exitCode = -1;
        result.error = std::string("fallo WaitForSingleObject: code=") + std::to_string(GetLastError());
        return false;
    }

    DWORD exitCode = 0;
    if (!GetExitCodeProcess(processInfo.hProcess, &exitCode)) {
        CloseHandle(processInfo.hThread);
        CloseHandle(processInfo.hProcess);
        cleanupCaptureFile(stdoutCapturePath);
        cleanupCaptureFile(stderrCapturePath);
        result.exitCode = -1;
        result.error = std::string("fallo GetExitCodeProcess: code=") + std::to_string(GetLastError());
        return false;
    }
    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);

    result.exitCode = static_cast<int>(exitCode);
    const bool ok = result.exitCode == 0;
    if (!ok) {
        result.error = "proceso finalizo con codigo no-cero";
    }
    return finalizeCaptureWithMessage(ok);
#else
    int stdoutFd = -1;
    int stderrFd = -1;
    if (options.captureOutput) {
        stdoutFd = open(stdoutCapturePath.string().c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
        if (stdoutFd < 0) {
            cleanupCaptureFile(stdoutCapturePath);
            cleanupCaptureFile(stderrCapturePath);
            result.exitCode = -1;
            result.error = std::string("fallo open stdout capture: ") + std::strerror(errno);
            return false;
        }
        stderrFd = open(stderrCapturePath.string().c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
        if (stderrFd < 0) {
            close(stdoutFd);
            cleanupCaptureFile(stdoutCapturePath);
            cleanupCaptureFile(stderrCapturePath);
            result.exitCode = -1;
            result.error = std::string("fallo open stderr capture: ") + std::strerror(errno);
            return false;
        }
    }

    std::vector<char *> argvRaw;
    argvRaw.reserve(args.size() + 1);
    for (const auto &arg : args) {
        argvRaw.push_back(const_cast<char *>(arg.c_str()));
    }
    argvRaw.push_back(nullptr);

    const pid_t pid = fork();
    if (pid < 0) {
        if (stdoutFd >= 0) close(stdoutFd);
        if (stderrFd >= 0) close(stderrFd);
        cleanupCaptureFile(stdoutCapturePath);
        cleanupCaptureFile(stderrCapturePath);
        result.exitCode = -1;
        result.error = std::string("fallo fork: ") + std::strerror(errno);
        return false;
    }
    if (pid == 0) {
        if (options.captureOutput) {
            if (dup2(stdoutFd, STDOUT_FILENO) < 0 || dup2(stderrFd, STDERR_FILENO) < 0) {
                _exit(126);
            }
            close(stdoutFd);
            close(stderrFd);
        }
        execvp(argvRaw.front(), argvRaw.data());
        _exit(127);
    }

    if (stdoutFd >= 0) close(stdoutFd);
    if (stderrFd >= 0) close(stderrFd);

    auto handleProcessStatus = [&](int status) -> bool {
        if (WIFEXITED(status)) {
            result.exitCode = WEXITSTATUS(status);
            if (result.exitCode != 0) {
                result.error = "proceso finalizo con codigo no-cero";
                return false;
            }
            return true;
        }
        if (WIFSIGNALED(status)) {
            result.exitCode = 128 + WTERMSIG(status);
            result.error = std::string("proceso terminado por senal ") + std::to_string(WTERMSIG(status));
            return false;
        }
        result.exitCode = -1;
        result.error = "estado de proceso no reconocido";
        return false;
    };

    if (options.timeoutMs <= 0) {
        int status = 0;
        if (waitpid(pid, &status, 0) < 0) {
            cleanupCaptureFile(stdoutCapturePath);
            cleanupCaptureFile(stderrCapturePath);
            result.exitCode = -1;
            result.error = std::string("fallo waitpid: ") + std::strerror(errno);
            return false;
        }
        const bool ok = handleProcessStatus(status);
        return finalizeCaptureWithMessage(ok);
    }

    const auto started = std::chrono::steady_clock::now();
    while (true) {
        int status = 0;
        const pid_t waitResult = waitpid(pid, &status, WNOHANG);
        if (waitResult == pid) {
            const bool ok = handleProcessStatus(status);
            return finalizeCaptureWithMessage(ok);
        }
        if (waitResult < 0) {
            cleanupCaptureFile(stdoutCapturePath);
            cleanupCaptureFile(stderrCapturePath);
            result.exitCode = -1;
            result.error = std::string("fallo waitpid: ") + std::strerror(errno);
            return false;
        }

        const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - started).count();
        if (elapsedMs >= options.timeoutMs) {
            kill(pid, SIGKILL);
            int statusAfterKill = 0;
            waitpid(pid, &statusAfterKill, 0);
            result.exitCode = 124;
            result.timedOut = true;
            result.error = "proceso excedio timeout de " + std::to_string(options.timeoutMs) + " ms";
            return finalizeCaptureWithMessage(false);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
#endif
}

} // namespace aym
