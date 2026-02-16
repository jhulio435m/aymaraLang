#include "semver.h"

#include <cctype>
#include <cstdint>
#include <limits>

namespace aym {

namespace {

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

bool parseIntPart(const std::string &part, int &out) {
    if (part.empty()) {
        return false;
    }
    int64_t value = 0;
    for (char ch : part) {
        if (!std::isdigit(static_cast<unsigned char>(ch))) {
            return false;
        }
        value = (value * 10) + (ch - '0');
        if (value > std::numeric_limits<int>::max()) {
            return false;
        }
    }
    out = static_cast<int>(value);
    return true;
}

bool splitSemver(const std::string &value, int &major, int &minor, int &patch) {
    const size_t firstDot = value.find('.');
    if (firstDot == std::string::npos) {
        return false;
    }
    const size_t secondDot = value.find('.', firstDot + 1);
    if (secondDot == std::string::npos) {
        return false;
    }
    if (value.find('.', secondDot + 1) != std::string::npos) {
        return false;
    }

    const std::string majorPart = value.substr(0, firstDot);
    const std::string minorPart = value.substr(firstDot + 1, secondDot - firstDot - 1);
    const std::string patchPart = value.substr(secondDot + 1);

    return parseIntPart(majorPart, major) &&
           parseIntPart(minorPart, minor) &&
           parseIntPart(patchPart, patch);
}

int saturatingIncrement(int value) {
    if (value >= std::numeric_limits<int>::max()) {
        return std::numeric_limits<int>::max();
    }
    return value + 1;
}

SemverVersion bumpPatch(const SemverVersion &version) {
    SemverVersion bumped = version;
    bumped.patch = saturatingIncrement(bumped.patch);
    return bumped;
}

SemverVersion upperBoundForCaret(const SemverVersion &base) {
    if (base.major > 0) {
        return {saturatingIncrement(base.major), 0, 0};
    }
    if (base.minor > 0) {
        return {0, saturatingIncrement(base.minor), 0};
    }
    return {0, 0, saturatingIncrement(base.patch)};
}

SemverVersion upperBoundForTilde(const SemverVersion &base) {
    return {base.major, saturatingIncrement(base.minor), 0};
}

bool satisfiesComparator(const SemverVersion &version, const VersionComparator &comparator) {
    const int cmp = compareSemver(version, comparator.version);
    switch (comparator.op) {
        case VersionComparatorOp::Exact:
            return cmp == 0;
        case VersionComparatorOp::Greater:
            return cmp > 0;
        case VersionComparatorOp::GreaterEqual:
            return cmp >= 0;
        case VersionComparatorOp::Less:
            return cmp < 0;
        case VersionComparatorOp::LessEqual:
            return cmp <= 0;
        case VersionComparatorOp::Caret: {
            if (cmp < 0) {
                return false;
            }
            const SemverVersion upper = upperBoundForCaret(comparator.version);
            return compareSemver(version, upper) < 0;
        }
        case VersionComparatorOp::Tilde: {
            if (cmp < 0) {
                return false;
            }
            const SemverVersion upper = upperBoundForTilde(comparator.version);
            return compareSemver(version, upper) < 0;
        }
    }
    return false;
}

} // namespace

bool parseSemverVersion(const std::string &text, SemverVersion &version, std::string &error) {
    error.clear();
    version = {};

    const std::string value = trim(text);
    if (value.empty()) {
        error = "version semantica vacia";
        return false;
    }

    int major = 0;
    int minor = 0;
    int patch = 0;
    if (!splitSemver(value, major, minor, patch)) {
        error = "version semantica invalida (se esperaba MAJOR.MINOR.PATCH)";
        return false;
    }

    version.major = major;
    version.minor = minor;
    version.patch = patch;
    return true;
}

bool parseSemverRequirement(const std::string &text, SemverRequirement &requirement, std::string &error) {
    error.clear();
    requirement = {};

    const std::string value = trim(text);
    if (value.empty()) {
        error = "requirement vacio";
        return false;
    }
    if (value == "*") {
        requirement.any = true;
        return true;
    }

    size_t start = 0;
    while (start <= value.size()) {
        size_t comma = value.find(',', start);
        std::string token = value.substr(start, comma == std::string::npos ? std::string::npos : comma - start);
        token = trim(token);
        if (token.empty()) {
            error = "requirement invalido: segmento vacio";
            return false;
        }

        VersionComparator comparator;
        std::string versionText;
        if (token.rfind(">=", 0) == 0) {
            comparator.op = VersionComparatorOp::GreaterEqual;
            versionText = trim(token.substr(2));
        } else if (token.rfind("<=", 0) == 0) {
            comparator.op = VersionComparatorOp::LessEqual;
            versionText = trim(token.substr(2));
        } else if (token.rfind(">", 0) == 0) {
            comparator.op = VersionComparatorOp::Greater;
            versionText = trim(token.substr(1));
        } else if (token.rfind("<", 0) == 0) {
            comparator.op = VersionComparatorOp::Less;
            versionText = trim(token.substr(1));
        } else if (token.rfind("^", 0) == 0) {
            comparator.op = VersionComparatorOp::Caret;
            versionText = trim(token.substr(1));
        } else if (token.rfind("~", 0) == 0) {
            comparator.op = VersionComparatorOp::Tilde;
            versionText = trim(token.substr(1));
        } else if (token.rfind("=", 0) == 0) {
            comparator.op = VersionComparatorOp::Exact;
            versionText = trim(token.substr(1));
        } else {
            comparator.op = VersionComparatorOp::Exact;
            versionText = token;
        }

        std::string versionError;
        if (!parseSemverVersion(versionText, comparator.version, versionError)) {
            error = "requirement invalido: " + versionError;
            return false;
        }
        requirement.comparators.push_back(comparator);

        if (comma == std::string::npos) {
            break;
        }
        start = comma + 1;
    }

    return true;
}

int compareSemver(const SemverVersion &lhs, const SemverVersion &rhs) {
    if (lhs.major != rhs.major) {
        return lhs.major < rhs.major ? -1 : 1;
    }
    if (lhs.minor != rhs.minor) {
        return lhs.minor < rhs.minor ? -1 : 1;
    }
    if (lhs.patch != rhs.patch) {
        return lhs.patch < rhs.patch ? -1 : 1;
    }
    return 0;
}

bool semverSatisfies(const SemverVersion &version, const SemverRequirement &requirement) {
    if (requirement.any) {
        return true;
    }
    for (const auto &comparator : requirement.comparators) {
        if (!satisfiesComparator(version, comparator)) {
            return false;
        }
    }
    return true;
}

std::string formatSemverVersion(const SemverVersion &version) {
    return std::to_string(version.major) + "." +
           std::to_string(version.minor) + "." +
           std::to_string(version.patch);
}

bool inferResolvedVersion(const SemverRequirement &requirement, SemverVersion &version, std::string &error) {
    error.clear();
    version = {};

    if (requirement.any) {
        version = {0, 0, 0};
        return true;
    }

    bool hasCandidate = false;
    bool hasExact = false;
    SemverVersion candidate = {0, 0, 0};

    for (const auto &comparator : requirement.comparators) {
        if (comparator.op != VersionComparatorOp::Exact) {
            continue;
        }
        if (!hasExact) {
            candidate = comparator.version;
            hasCandidate = true;
            hasExact = true;
            continue;
        }
        if (compareSemver(candidate, comparator.version) != 0) {
            error = "requirement invalido: comparadores exactos conflictivos";
            return false;
        }
    }

    if (!hasExact) {
        for (const auto &comparator : requirement.comparators) {
            SemverVersion lowerBound;
            bool contributes = false;
            if (comparator.op == VersionComparatorOp::Caret || comparator.op == VersionComparatorOp::Tilde ||
                comparator.op == VersionComparatorOp::GreaterEqual) {
                lowerBound = comparator.version;
                contributes = true;
            } else if (comparator.op == VersionComparatorOp::Greater) {
                lowerBound = bumpPatch(comparator.version);
                contributes = true;
            }

            if (!contributes) {
                continue;
            }
            if (!hasCandidate || compareSemver(lowerBound, candidate) > 0) {
                candidate = lowerBound;
                hasCandidate = true;
            }
        }
    }

    if (!hasCandidate) {
        candidate = {0, 0, 0};
    }

    if (!semverSatisfies(candidate, requirement)) {
        error = "no se pudo inferir version resuelta que cumpla el requirement";
        return false;
    }

    version = candidate;
    return true;
}

} // namespace aym
