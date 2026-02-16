#ifndef AYM_PROJECT_MANIFEST_H
#define AYM_PROJECT_MANIFEST_H

#include <map>
#include <string>
#include <vector>

namespace aym {

struct ManifestError {
    std::string code;
    std::string message;
    size_t line = 0;
    size_t column = 0;
};

struct ProjectManifest {
    std::string name;
    std::string version;
    std::string edition;
    std::map<std::string, std::string> dependencies;
};

struct LockDependency {
    std::string name;
    std::string requirement;
    std::string resolved;
    std::string checksum;
    size_t line = 0;
};

struct ProjectLock {
    int lockVersion = 0;
    std::string projectName;
    std::string projectVersion;
    std::string edition;
    std::string manifestChecksum;
    std::vector<LockDependency> dependencies;
};

bool parseProjectManifestFile(const std::string &path, ProjectManifest &manifest, std::string &error);
bool parseProjectManifestFileDetailed(const std::string &path, ProjectManifest &manifest, ManifestError &error);
bool writeProjectLockFile(const ProjectManifest &manifest, const std::string &path, std::string &error);
bool writeProjectLockFileDetailed(const ProjectManifest &manifest, const std::string &path, ManifestError &error);
bool parseProjectLockFile(const std::string &path, ProjectLock &lock, std::string &error);
bool parseProjectLockFileDetailed(const std::string &path, ProjectLock &lock, ManifestError &error);
bool verifyLockAgainstManifest(const ProjectManifest &manifest, const ProjectLock &lock, std::string &error);
bool verifyLockAgainstManifestDetailed(const ProjectManifest &manifest, const ProjectLock &lock, ManifestError &error);
std::string defaultManifestPath();
std::string defaultLockPathForManifest(const std::string &manifestPath);

} // namespace aym

#endif // AYM_PROJECT_MANIFEST_H
