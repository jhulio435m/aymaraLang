#ifndef AYM_DIAGNOSTIC_H
#define AYM_DIAGNOSTIC_H

#include <ostream>
#include <string>
#include <vector>

namespace aym {

enum class DiagnosticSeverity {
    Error,
    Warning
};

struct Diagnostic {
    DiagnosticSeverity severity = DiagnosticSeverity::Error;
    std::string code;
    std::string message;
    std::string suggestion;
    size_t line = 0;
    size_t column = 0;
};

class DiagnosticEngine {
public:
    void report(const Diagnostic &diag);
    void error(const std::string &code, const std::string &message,
               size_t line = 0, size_t column = 0,
               const std::string &suggestion = "");
    void warning(const std::string &code, const std::string &message,
                 size_t line = 0, size_t column = 0,
                 const std::string &suggestion = "");
    void clear();
    bool hasErrors() const;
    bool empty() const { return diagnostics.empty(); }
    const std::vector<Diagnostic> &all() const { return diagnostics; }
    void printAll(std::ostream &out) const;
    bool writeJsonFile(const std::string &path, std::string &error) const;
    static std::string format(const Diagnostic &diag);

private:
    std::vector<Diagnostic> diagnostics;
};

} // namespace aym

#endif // AYM_DIAGNOSTIC_H
