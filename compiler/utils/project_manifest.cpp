#include "project_manifest.h"
#include "fs.h"
#include "semver.h"

#include <cctype>
#include <cstdint>
#include <exception>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <unordered_map>

namespace aym {

namespace {

enum class ManifestSection {
    None,
    Package,
    Dependencies
};

std::string trim(const std::string &value) {
    size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start]))) {
        ++start;
    }
    size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }
    return value.substr(start, end - start);
}

std::string stripInlineComment(const std::string &line) {
    bool inString = false;
    bool escaped = false;
    for (size_t i = 0; i < line.size(); ++i) {
        const char ch = line[i];
        if (escaped) {
            escaped = false;
            continue;
        }
        if (ch == '\\') {
            escaped = true;
            continue;
        }
        if (ch == '"') {
            inString = !inString;
            continue;
        }
        if (ch == '#' && !inString) {
            return line.substr(0, i);
        }
    }
    return line;
}

bool parseQuotedValue(const std::string &value, std::string &parsed, std::string &error) {
    const std::string trimmed = trim(value);
    if (trimmed.size() < 2 || trimmed.front() != '"' || trimmed.back() != '"') {
        error = "el valor debe ser string entre comillas";
        return false;
    }
    parsed.clear();
    parsed.reserve(trimmed.size() - 2);
    for (size_t i = 1; i + 1 < trimmed.size(); ++i) {
        const char ch = trimmed[i];
        if (ch == '\\' && i + 1 < trimmed.size() - 1) {
            const char next = trimmed[++i];
            switch (next) {
                case '"':
                    parsed.push_back('"');
                    break;
                case '\\':
                    parsed.push_back('\\');
                    break;
                case 'n':
                    parsed.push_back('\n');
                    break;
                case 't':
                    parsed.push_back('\t');
                    break;
                default:
                    parsed.push_back(next);
                    break;
            }
        } else {
            parsed.push_back(ch);
        }
    }
    return true;
}

bool parseKeyValue(const std::string &line, std::string &key, std::string &value, std::string &error) {
    const size_t eq = line.find('=');
    if (eq == std::string::npos) {
        error = "se esperaba formato clave = \"valor\"";
        return false;
    }
    key = trim(line.substr(0, eq));
    value = trim(line.substr(eq + 1));
    if (key.empty()) {
        error = "la clave no puede estar vacia";
        return false;
    }
    if (value.empty()) {
        error = "el valor no puede estar vacio";
        return false;
    }
    return true;
}

uint64_t fnv1a64(const std::string &value) {
    uint64_t hash = 14695981039346656037ull;
    for (unsigned char ch : value) {
        hash ^= ch;
        hash *= 1099511628211ull;
    }
    return hash;
}

std::string hex64(uint64_t value) {
    std::ostringstream out;
    out << std::hex << std::nouppercase << std::setw(16) << std::setfill('0') << value;
    return out.str();
}

std::string escapeLockString(const std::string &value) {
    std::string out;
    out.reserve(value.size());
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

bool parseIntegerValue(const std::string &value, int &parsed, std::string &error) {
    const std::string trimmed = trim(value);
    if (trimmed.empty()) {
        error = "el valor entero no puede estar vacio";
        return false;
    }
    for (char ch : trimmed) {
        if (!std::isdigit(static_cast<unsigned char>(ch))) {
            error = "el valor debe ser entero positivo";
            return false;
        }
    }
    try {
        parsed = std::stoi(trimmed);
    } catch (const std::exception &) {
        error = "valor entero fuera de rango";
        return false;
    }
    return true;
}

std::string dependencyChecksum(const std::string &name, const std::string &resolved) {
    return "fnv1a64:" + hex64(fnv1a64(name + "@" + resolved));
}

std::string normalizeEdition(const std::string &edition) {
    return edition.empty() ? "2026" : edition;
}

std::string manifestChecksum(const ProjectManifest &manifest) {
    std::string payload;
    payload.reserve(128 + manifest.dependencies.size() * 32);
    payload += "name=" + manifest.name + "\n";
    payload += "version=" + manifest.version + "\n";
    payload += "edition=" + normalizeEdition(manifest.edition) + "\n";
    for (const auto &dep : manifest.dependencies) {
        payload += "dep=" + dep.first + "=>" + dep.second + "\n";
    }
    return "fnv1a64:" + hex64(fnv1a64(payload));
}

bool isValidFNV1A64Checksum(const std::string &value) {
    const std::string prefix = "fnv1a64:";
    if (value.rfind(prefix, 0) != 0) {
        return false;
    }
    if (value.size() != prefix.size() + 16) {
        return false;
    }
    for (size_t i = prefix.size(); i < value.size(); ++i) {
        const char ch = value[i];
        const bool hex = (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f');
        if (!hex) {
            return false;
        }
    }
    return true;
}

} // namespace

std::string defaultManifestPath() {
    return "aym.toml";
}

std::string defaultLockPathForManifest(const std::string &manifestPath) {
    fs::path path = manifestPath.empty() ? fs::path(defaultManifestPath()) : fs::path(manifestPath);
    if (path.has_parent_path()) {
        return (path.parent_path() / "aym.lock").string();
    }
    return "aym.lock";
}

bool parseProjectManifestFileDetailed(const std::string &path, ProjectManifest &manifest, ManifestError &error) {
    error = {};
    manifest = {};

    std::ifstream in(path);
    if (!in.is_open()) {
        error.code = "AYM5001";
        error.message = "no se pudo abrir manifest: " + path;
        return false;
    }

    ManifestSection section = ManifestSection::None;
    size_t packageVersionLine = 0;
    std::unordered_map<std::string, size_t> dependencyLines;
    std::string rawLine;
    size_t lineNumber = 0;
    while (std::getline(in, rawLine)) {
        ++lineNumber;
        const std::string noComment = stripInlineComment(rawLine);
        const std::string line = trim(noComment);
        if (line.empty()) {
            continue;
        }

        if (line.front() == '[' && line.back() == ']') {
            const std::string sectionName = trim(line.substr(1, line.size() - 2));
            if (sectionName == "package") {
                section = ManifestSection::Package;
            } else if (sectionName == "dependencies") {
                section = ManifestSection::Dependencies;
            } else {
                error.code = "AYM5002";
                error.line = lineNumber;
                error.message = "seccion no soportada [" + sectionName + "]";
                return false;
            }
            continue;
        }

        std::string key;
        std::string valueRaw;
        std::string kvError;
        if (!parseKeyValue(line, key, valueRaw, kvError)) {
            error.code = "AYM5002";
            error.line = lineNumber;
            error.message = kvError;
            return false;
        }

        std::string value;
        std::string valueError;
        if (!parseQuotedValue(valueRaw, value, valueError)) {
            error.code = "AYM5002";
            error.line = lineNumber;
            error.message = valueError;
            return false;
        }

        if (section == ManifestSection::Package) {
            if (key == "name") {
                manifest.name = value;
            } else if (key == "version") {
                manifest.version = value;
                packageVersionLine = lineNumber;
            } else if (key == "edition") {
                manifest.edition = value;
            } else {
                error.code = "AYM5002";
                error.line = lineNumber;
                error.message = "clave no soportada en [package]: " + key;
                return false;
            }
        } else if (section == ManifestSection::Dependencies) {
            manifest.dependencies[key] = value;
            dependencyLines[key] = lineNumber;
        } else {
            error.code = "AYM5002";
            error.line = lineNumber;
            error.message = "clave fuera de seccion [package] o [dependencies]";
            return false;
        }
    }

    if (manifest.name.empty()) {
        error.code = "AYM5003";
        error.message = "manifest invalido: falta [package].name";
        return false;
    }
    if (manifest.version.empty()) {
        error.code = "AYM5003";
        error.message = "manifest invalido: falta [package].version";
        return false;
    }

    SemverVersion projectVersion;
    std::string versionError;
    if (!parseSemverVersion(manifest.version, projectVersion, versionError)) {
        error.code = "AYM5008";
        error.line = packageVersionLine;
        error.message = "version invalida en [package].version: " + versionError;
        return false;
    }

    for (const auto &dep : manifest.dependencies) {
        SemverRequirement requirement;
        std::string requirementError;
        if (!parseSemverRequirement(dep.second, requirement, requirementError)) {
            error.code = "AYM5009";
            auto it = dependencyLines.find(dep.first);
            if (it != dependencyLines.end()) {
                error.line = it->second;
            }
            error.message = "requirement invalido para dependencia '" + dep.first + "': " + requirementError;
            return false;
        }
    }

    if (manifest.edition.empty()) {
        manifest.edition = "2026";
    }
    return true;
}

bool parseProjectManifestFile(const std::string &path, ProjectManifest &manifest, std::string &error) {
    error.clear();
    ManifestError detailedError;
    if (parseProjectManifestFileDetailed(path, manifest, detailedError)) {
        return true;
    }
    if (detailedError.line > 0) {
        error = "linea " + std::to_string(detailedError.line) + ": " + detailedError.message;
    } else {
        error = detailedError.message;
    }
    return false;
}

bool writeProjectLockFileDetailed(const ProjectManifest &manifest, const std::string &path, ManifestError &error) {
    error = {};
    if (path.empty()) {
        error.code = "AYM5004";
        error.message = "ruta invalida para lockfile";
        return false;
    }

    SemverVersion projectVersion;
    std::string versionError;
    if (!parseSemverVersion(manifest.version, projectVersion, versionError)) {
        error.code = "AYM5008";
        error.message = "version invalida en [package].version: " + versionError;
        return false;
    }

    fs::path lockPath(path);
    if (lockPath.has_parent_path()) {
        std::error_code ec;
        fs::create_directories(lockPath.parent_path(), ec);
        if (ec) {
            error.code = "AYM5004";
            error.message = "no se pudo crear directorio para lockfile: " + lockPath.parent_path().string();
            return false;
        }
    }

    std::ofstream out(path);
    if (!out.is_open()) {
        error.code = "AYM5004";
        error.message = "no se pudo escribir lockfile en: " + path;
        return false;
    }

    const std::string normalizedEdition = normalizeEdition(manifest.edition);
    const std::string lockManifestChecksum = manifestChecksum(manifest);

    out << "lock_version = 1\n";
    out << "project_name = \"" << escapeLockString(manifest.name) << "\"\n";
    out << "project_version = \"" << escapeLockString(manifest.version) << "\"\n";
    out << "edition = \"" << escapeLockString(normalizedEdition) << "\"\n";
    out << "manifest_checksum = \"" << lockManifestChecksum << "\"\n";

    for (const auto &dep : manifest.dependencies) {
        SemverRequirement requirement;
        std::string requirementError;
        if (!parseSemverRequirement(dep.second, requirement, requirementError)) {
            error.code = "AYM5009";
            error.message = "requirement invalido para dependencia '" + dep.first + "': " + requirementError;
            return false;
        }

        SemverVersion resolvedVersion;
        std::string resolveError;
        if (!inferResolvedVersion(requirement, resolvedVersion, resolveError)) {
            error.code = "AYM5010";
            error.message = "no se pudo resolver version para dependencia '" + dep.first + "': " + resolveError;
            return false;
        }

        const std::string resolved = formatSemverVersion(resolvedVersion);
        const std::string checksum = dependencyChecksum(dep.first, resolved);
        out << "\n[[dependency]]\n";
        out << "name = \"" << escapeLockString(dep.first) << "\"\n";
        out << "requirement = \"" << escapeLockString(dep.second) << "\"\n";
        out << "resolved = \"" << escapeLockString(resolved) << "\"\n";
        out << "checksum = \"" << checksum << "\"\n";
    }

    return true;
}

bool writeProjectLockFile(const ProjectManifest &manifest, const std::string &path, std::string &error) {
    error.clear();
    ManifestError detailedError;
    if (writeProjectLockFileDetailed(manifest, path, detailedError)) {
        return true;
    }
    error = detailedError.message;
    return false;
}

bool parseProjectLockFileDetailed(const std::string &path, ProjectLock &lock, ManifestError &error) {
    error = {};
    lock = {};

    std::ifstream in(path);
    if (!in.is_open()) {
        error.code = "AYM5005";
        error.message = "no se pudo abrir lockfile: " + path;
        return false;
    }

    bool inDependency = false;
    LockDependency currentDep;
    size_t projectVersionLine = 0;
    size_t manifestChecksumLine = 0;
    std::string rawLine;
    size_t lineNumber = 0;
    while (std::getline(in, rawLine)) {
        ++lineNumber;
        const std::string noComment = stripInlineComment(rawLine);
        const std::string line = trim(noComment);
        if (line.empty()) {
            continue;
        }

        if (line == "[[dependency]]") {
            if (inDependency) {
                if (currentDep.name.empty() || currentDep.requirement.empty() ||
                    currentDep.resolved.empty() || currentDep.checksum.empty()) {
                    error.code = "AYM5006";
                    error.line = currentDep.line;
                    error.message = "bloque [[dependency]] incompleto";
                    return false;
                }
                lock.dependencies.push_back(currentDep);
            }
            inDependency = true;
            currentDep = {};
            currentDep.line = lineNumber;
            continue;
        }

        std::string key;
        std::string valueRaw;
        std::string kvError;
        if (!parseKeyValue(line, key, valueRaw, kvError)) {
            error.code = "AYM5006";
            error.line = lineNumber;
            error.message = kvError;
            return false;
        }

        if (inDependency) {
            std::string value;
            std::string valueError;
            if (!parseQuotedValue(valueRaw, value, valueError)) {
                error.code = "AYM5006";
                error.line = lineNumber;
                error.message = valueError;
                return false;
            }
            if (key == "name") {
                currentDep.name = value;
            } else if (key == "requirement") {
                currentDep.requirement = value;
            } else if (key == "resolved") {
                currentDep.resolved = value;
            } else if (key == "checksum") {
                currentDep.checksum = value;
            } else {
                error.code = "AYM5006";
                error.line = lineNumber;
                error.message = "clave no soportada en [[dependency]]: " + key;
                return false;
            }
            continue;
        }

        if (key == "lock_version") {
            int parsed = 0;
            std::string intError;
            if (!parseIntegerValue(valueRaw, parsed, intError)) {
                error.code = "AYM5006";
                error.line = lineNumber;
                error.message = intError;
                return false;
            }
            lock.lockVersion = parsed;
            continue;
        }

        std::string value;
        std::string valueError;
        if (!parseQuotedValue(valueRaw, value, valueError)) {
            error.code = "AYM5006";
            error.line = lineNumber;
            error.message = valueError;
            return false;
        }
        if (key == "project_name") {
            lock.projectName = value;
        } else if (key == "project_version") {
            lock.projectVersion = value;
            projectVersionLine = lineNumber;
        } else if (key == "edition") {
            lock.edition = value;
        } else if (key == "manifest_checksum") {
            lock.manifestChecksum = value;
            manifestChecksumLine = lineNumber;
        } else {
            error.code = "AYM5006";
            error.line = lineNumber;
            error.message = "clave no soportada en cabecera lock: " + key;
            return false;
        }
    }

    if (inDependency) {
        if (currentDep.name.empty() || currentDep.requirement.empty() ||
            currentDep.resolved.empty() || currentDep.checksum.empty()) {
            error.code = "AYM5006";
            error.line = currentDep.line;
            error.message = "bloque [[dependency]] incompleto";
            return false;
        }
        lock.dependencies.push_back(currentDep);
    }

    if (lock.lockVersion != 1) {
        error.code = "AYM5006";
        error.message = "lock_version no soportado (se esperaba 1)";
        return false;
    }
    if (lock.projectName.empty() || lock.projectVersion.empty()) {
        error.code = "AYM5006";
        error.message = "lockfile incompleto: faltan project_name o project_version";
        return false;
    }
    if (lock.manifestChecksum.empty()) {
        error.code = "AYM5006";
        error.message = "lockfile incompleto: falta manifest_checksum";
        return false;
    }
    if (!isValidFNV1A64Checksum(lock.manifestChecksum)) {
        error.code = "AYM5006";
        error.line = manifestChecksumLine;
        error.message = "manifest_checksum invalido: se esperaba formato fnv1a64:<16hex>";
        return false;
    }

    SemverVersion parsedProjectVersion;
    std::string projectVersionError;
    if (!parseSemverVersion(lock.projectVersion, parsedProjectVersion, projectVersionError)) {
        error.code = "AYM5008";
        error.line = projectVersionLine;
        error.message = "project_version invalido en lockfile: " + projectVersionError;
        return false;
    }

    std::unordered_map<std::string, bool> depNames;
    for (const auto &dep : lock.dependencies) {
        if (depNames.find(dep.name) != depNames.end()) {
            error.code = "AYM5006";
            error.line = dep.line;
            error.message = "dependencia duplicada en lockfile: " + dep.name;
            return false;
        }
        depNames[dep.name] = true;

        SemverRequirement requirement;
        std::string requirementError;
        if (!parseSemverRequirement(dep.requirement, requirement, requirementError)) {
            error.code = "AYM5009";
            error.line = dep.line;
            error.message = "requirement invalido para dependencia '" + dep.name + "': " + requirementError;
            return false;
        }

        SemverVersion resolvedVersion;
        std::string resolvedError;
        if (!parseSemverVersion(dep.resolved, resolvedVersion, resolvedError)) {
            error.code = "AYM5010";
            error.line = dep.line;
            error.message = "version resuelta invalida para dependencia '" + dep.name + "': " + resolvedError;
            return false;
        }
        if (!semverSatisfies(resolvedVersion, requirement)) {
            error.code = "AYM5010";
            error.line = dep.line;
            error.message = "version resuelta incompatible con requirement para dependencia: " + dep.name;
            return false;
        }

        if (!isValidFNV1A64Checksum(dep.checksum)) {
            error.code = "AYM5006";
            error.line = dep.line;
            error.message = "checksum invalido para dependencia '" + dep.name +
                            "': se esperaba formato fnv1a64:<16hex>";
            return false;
        }

        const std::string expectedChecksum = dependencyChecksum(dep.name, dep.resolved);
        if (dep.checksum != expectedChecksum) {
            error.code = "AYM5007";
            error.line = dep.line;
            error.message = "checksum inconsistente para dependencia: " + dep.name;
            return false;
        }
    }

    return true;
}

bool parseProjectLockFile(const std::string &path, ProjectLock &lock, std::string &error) {
    error.clear();
    ManifestError detailedError;
    if (parseProjectLockFileDetailed(path, lock, detailedError)) {
        return true;
    }
    if (detailedError.line > 0) {
        error = "linea " + std::to_string(detailedError.line) + ": " + detailedError.message;
    } else {
        error = detailedError.message;
    }
    return false;
}

bool verifyLockAgainstManifestDetailed(const ProjectManifest &manifest, const ProjectLock &lock, ManifestError &error) {
    error = {};

    SemverVersion manifestVersion;
    std::string manifestVersionError;
    if (!parseSemverVersion(manifest.version, manifestVersion, manifestVersionError)) {
        error.code = "AYM5008";
        error.message = "version invalida en [package].version: " + manifestVersionError;
        return false;
    }

    SemverVersion lockVersion;
    std::string lockVersionError;
    if (!parseSemverVersion(lock.projectVersion, lockVersion, lockVersionError)) {
        error.code = "AYM5008";
        error.message = "project_version invalido en lockfile: " + lockVersionError;
        return false;
    }

    if (lock.projectName != manifest.name) {
        error.code = "AYM5007";
        error.message = "project_name del lockfile no coincide con [package].name";
        return false;
    }
    if (compareSemver(lockVersion, manifestVersion) != 0) {
        error.code = "AYM5007";
        error.message = "project_version del lockfile no coincide con [package].version";
        return false;
    }
    if (!lock.edition.empty() && lock.edition != normalizeEdition(manifest.edition)) {
        error.code = "AYM5007";
        error.message = "edition del lockfile no coincide con [package].edition";
        return false;
    }

    const std::string expectedManifestChecksum = manifestChecksum(manifest);
    if (lock.manifestChecksum != expectedManifestChecksum) {
        error.code = "AYM5007";
        error.message = "manifest_checksum inconsistente entre manifest y lockfile";
        return false;
    }

    std::unordered_map<std::string, const LockDependency *> lockDeps;
    for (const auto &dep : lock.dependencies) {
        if (lockDeps.find(dep.name) != lockDeps.end()) {
            error.code = "AYM5006";
            error.line = dep.line;
            error.message = "dependencia duplicada en lockfile: " + dep.name;
            return false;
        }
        lockDeps[dep.name] = &dep;
    }

    for (const auto &dep : manifest.dependencies) {
        const auto it = lockDeps.find(dep.first);
        if (it == lockDeps.end()) {
            error.code = "AYM5007";
            error.message = "falta dependencia en lockfile: " + dep.first;
            return false;
        }
        const LockDependency &lockDep = *it->second;
        if (lockDep.requirement != dep.second) {
            error.code = "AYM5007";
            error.line = lockDep.line;
            error.message = "requirement inconsistente para dependencia: " + dep.first;
            return false;
        }

        SemverRequirement manifestRequirement;
        std::string manifestRequirementError;
        if (!parseSemverRequirement(dep.second, manifestRequirement, manifestRequirementError)) {
            error.code = "AYM5009";
            error.message = "requirement invalido para dependencia '" + dep.first + "': " + manifestRequirementError;
            return false;
        }

        SemverRequirement lockRequirement;
        std::string lockRequirementError;
        if (!parseSemverRequirement(lockDep.requirement, lockRequirement, lockRequirementError)) {
            error.code = "AYM5009";
            error.line = lockDep.line;
            error.message = "requirement invalido en lockfile para dependencia '" + dep.first + "': " + lockRequirementError;
            return false;
        }

        SemverVersion resolvedVersion;
        std::string resolvedError;
        if (!parseSemverVersion(lockDep.resolved, resolvedVersion, resolvedError)) {
            error.code = "AYM5010";
            error.line = lockDep.line;
            error.message = "version resuelta invalida para dependencia '" + dep.first + "': " + resolvedError;
            return false;
        }
        if (!semverSatisfies(resolvedVersion, manifestRequirement)) {
            error.code = "AYM5010";
            error.line = lockDep.line;
            error.message = "version resuelta no satisface el requirement del manifest para dependencia: " + dep.first;
            return false;
        }
        if (!semverSatisfies(resolvedVersion, lockRequirement)) {
            error.code = "AYM5010";
            error.line = lockDep.line;
            error.message = "version resuelta no satisface el requirement del lockfile para dependencia: " + dep.first;
            return false;
        }

        const std::string expectedChecksum = dependencyChecksum(lockDep.name, lockDep.resolved);
        if (lockDep.checksum != expectedChecksum) {
            error.code = "AYM5007";
            error.line = lockDep.line;
            error.message = "checksum inconsistente para dependencia: " + dep.first;
            return false;
        }
    }

    for (const auto &dep : lock.dependencies) {
        if (manifest.dependencies.find(dep.name) == manifest.dependencies.end()) {
            error.code = "AYM5007";
            error.line = dep.line;
            error.message = "dependencia extra en lockfile no declarada en manifest: " + dep.name;
            return false;
        }
    }

    return true;
}

bool verifyLockAgainstManifest(const ProjectManifest &manifest, const ProjectLock &lock, std::string &error) {
    error.clear();
    ManifestError detailedError;
    if (verifyLockAgainstManifestDetailed(manifest, lock, detailedError)) {
        return true;
    }
    if (detailedError.line > 0) {
        error = "linea " + std::to_string(detailedError.line) + ": " + detailedError.message;
    } else {
        error = detailedError.message;
    }
    return false;
}

} // namespace aym
