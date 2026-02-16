#include "diagnostic.h"
#include "diagnostic_catalog.h"
#include "fs.h"

#include <fstream>

namespace aym {

namespace {
const char *severityName(DiagnosticSeverity severity) {
    switch (severity) {
        case DiagnosticSeverity::Error:
            return "error";
        case DiagnosticSeverity::Warning:
            return "warning";
        default:
            return "unknown";
    }
}

std::string escapeJson(const std::string &value) {
    std::string out;
    out.reserve(value.size());
    for (unsigned char ch : value) {
        switch (ch) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (ch < 0x20) {
                    const char *digits = "0123456789ABCDEF";
                    out += "\\u00";
                    out += digits[(ch >> 4) & 0xF];
                    out += digits[ch & 0xF];
                } else {
                    out.push_back(static_cast<char>(ch));
                }
                break;
        }
    }
    return out;
}

std::string quote(const std::string &value) {
    return "\"" + escapeJson(value) + "\"";
}
} // namespace

void DiagnosticEngine::report(const Diagnostic &diag) {
    diagnostics.push_back(diag);
}

void DiagnosticEngine::error(const std::string &code, const std::string &message,
                             size_t line, size_t column,
                             const std::string &suggestion) {
    std::string resolvedSuggestion = suggestion;
    if (resolvedSuggestion.empty()) {
        resolvedSuggestion = diagnosticSuggestionForCode(code);
    }
    report({DiagnosticSeverity::Error, code, message, resolvedSuggestion, line, column});
}

void DiagnosticEngine::warning(const std::string &code, const std::string &message,
                               size_t line, size_t column,
                               const std::string &suggestion) {
    std::string resolvedSuggestion = suggestion;
    if (resolvedSuggestion.empty()) {
        resolvedSuggestion = diagnosticSuggestionForCode(code);
    }
    report({DiagnosticSeverity::Warning, code, message, resolvedSuggestion, line, column});
}

void DiagnosticEngine::clear() {
    diagnostics.clear();
}

bool DiagnosticEngine::hasErrors() const {
    for (const auto &diag : diagnostics) {
        if (diag.severity == DiagnosticSeverity::Error) {
            return true;
        }
    }
    return false;
}

void DiagnosticEngine::printAll(std::ostream &out) const {
    for (const auto &diag : diagnostics) {
        out << format(diag) << "\n";
    }
}

bool DiagnosticEngine::writeJsonFile(const std::string &path, std::string &error) const {
    error.clear();
    if (path.empty()) {
        error = "Ruta invalida para --diagnostics-json";
        return false;
    }

    fs::path outPath(path);
    if (outPath.has_parent_path()) {
        std::error_code ec;
        fs::create_directories(outPath.parent_path(), ec);
        if (ec) {
            error = "No se pudo crear directorio para diagnostics JSON: " + outPath.parent_path().string();
            return false;
        }
    }

    std::ofstream out(path);
    if (!out.is_open()) {
        error = "No se pudo escribir diagnostics JSON en: " + path;
        return false;
    }

    out << "{\n";
    out << "  \"diagnostics\": [\n";
    for (size_t i = 0; i < diagnostics.size(); ++i) {
        const auto &diag = diagnostics[i];
        out << "    {\n";
        out << "      \"severity\": " << quote(severityName(diag.severity)) << ",\n";
        out << "      \"code\": " << quote(diag.code) << ",\n";
        out << "      \"message\": " << quote(diag.message) << ",\n";
        out << "      \"suggestion\": " << quote(diag.suggestion) << ",\n";
        out << "      \"line\": " << diag.line << ",\n";
        out << "      \"column\": " << diag.column << "\n";
        out << "    }";
        if (i + 1 < diagnostics.size()) {
            out << ",";
        }
        out << "\n";
    }
    out << "  ]\n";
    out << "}\n";
    return true;
}

std::string DiagnosticEngine::format(const Diagnostic &diag) {
    std::string out = "[aymc][" + std::string(severityName(diag.severity)) + "]";
    if (!diag.code.empty()) {
        out += "[" + diag.code + "]";
    }
    if (diag.line > 0) {
        out += " linea " + std::to_string(diag.line);
        if (diag.column > 0) {
            out += ", columna " + std::to_string(diag.column);
        }
    }
    out += ": " + diag.message;
    if (!diag.suggestion.empty()) {
        out += "\n  sugerencia: " + diag.suggestion;
    }
    return out;
}

} // namespace aym
