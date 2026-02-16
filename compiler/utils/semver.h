#ifndef AYM_SEMVER_H
#define AYM_SEMVER_H

#include <string>
#include <vector>

namespace aym {

struct SemverVersion {
    int major = 0;
    int minor = 0;
    int patch = 0;
};

enum class VersionComparatorOp {
    Exact,
    Greater,
    GreaterEqual,
    Less,
    LessEqual,
    Caret,
    Tilde
};

struct VersionComparator {
    VersionComparatorOp op = VersionComparatorOp::Exact;
    SemverVersion version;
};

struct SemverRequirement {
    bool any = false;
    std::vector<VersionComparator> comparators;
};

bool parseSemverVersion(const std::string &text, SemverVersion &version, std::string &error);
bool parseSemverRequirement(const std::string &text, SemverRequirement &requirement, std::string &error);
int compareSemver(const SemverVersion &lhs, const SemverVersion &rhs);
bool semverSatisfies(const SemverVersion &version, const SemverRequirement &requirement);
std::string formatSemverVersion(const SemverVersion &version);
bool inferResolvedVersion(const SemverRequirement &requirement, SemverVersion &version, std::string &error);

} // namespace aym

#endif // AYM_SEMVER_H
