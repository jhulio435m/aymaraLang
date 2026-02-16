#ifndef AYM_PROJECT_TOOL_H
#define AYM_PROJECT_TOOL_H

#include "fs.h"
#include "project_manifest.h"

#include <string>
#include <vector>

namespace aym {

struct ProjectWorkspace {
    fs::path rootDir;
    fs::path manifestPath;
    fs::path lockPath;
    fs::path sourcePath;
    fs::path buildDir;
    fs::path outputBasePath;
    ProjectManifest manifest;
};

struct DependencyCacheLayout {
    fs::path repoRoot;
    fs::path cacheRoot;
    std::vector<fs::path> searchPaths;
};

struct DependencyStoreEntry {
    std::string name;
    std::string resolved;
    bool repoModules = false;
    bool cacheModules = false;
};

struct DependencyStoreReport {
    DependencyCacheLayout layout;
    std::vector<DependencyStoreEntry> entries;
};

bool locateManifestUpwards(const fs::path &start, fs::path &manifestPath);
bool loadProjectWorkspace(const fs::path &manifestHint, ProjectWorkspace &workspace, std::string &error);
bool createProjectScaffold(const fs::path &projectDir, const std::string &packageName, std::string &error);
bool upsertProjectDependency(const fs::path &manifestPath,
                             const std::string &dependency,
                             const std::string &requirement,
                             std::string &error);
bool resolveProjectDependencyLayout(const ProjectWorkspace &workspace,
                                    DependencyCacheLayout &layout,
                                    std::string &error);
bool inspectProjectDependencyStore(const ProjectWorkspace &workspace,
                                   DependencyStoreReport &report,
                                   std::string &error);
bool prepareProjectDependencyCache(const ProjectWorkspace &workspace,
                                   DependencyCacheLayout &layout,
                                   std::string &error);
bool cleanProjectDependencyCache(const ProjectWorkspace &workspace,
                                 bool cleanRepo,
                                 bool cleanCache,
                                 std::string &error);
bool gatherProjectTestFiles(const fs::path &projectRoot, std::vector<fs::path> &testFiles, std::string &error);

} // namespace aym

#endif // AYM_PROJECT_TOOL_H
