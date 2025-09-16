#ifndef AYM_MODULE_RESOLVER_H
#define AYM_MODULE_RESOLVER_H

#include "fs.h"
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

namespace aym {

class Node;

class ModuleResolver {
public:
    ModuleResolver();
    explicit ModuleResolver(const fs::path &entryDir);

    void setEntryDir(const fs::path &dir);
    void addSearchPath(const fs::path &path);
    void clear();

    void resolve(std::vector<std::unique_ptr<Node>> &nodes, const fs::path &currentDir);
    std::vector<std::unique_ptr<Node>> load(const std::string &moduleName,
                                            const fs::path &currentDir,
                                            size_t line,
                                            size_t column);

private:
    std::vector<fs::path> searchPaths;
    std::unordered_set<std::string> searchKeys;
    std::unordered_set<std::string> loadedModules;
    std::unordered_set<std::string> loadingModules;

    void addIfUnique(const fs::path &path);
    static bool isAbsoluteModule(const std::string &moduleName);
    std::string normalize(const std::string &moduleName) const;
    fs::path findModulePath(const std::string &moduleName,
                            const std::string &normalized,
                            const fs::path &currentDir) const;
    std::vector<std::unique_ptr<Node>> parseModule(const fs::path &path,
                                                   const std::string &moduleName);
};

} // namespace aym

#endif // AYM_MODULE_RESOLVER_H
