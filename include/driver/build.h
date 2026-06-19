#pragma once
#include <basic/symbols/module.h>
#include <driver/file.h>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

namespace veo::driver {

struct Manifest {
    std::string           ProjectName;
    fs::path              ManifestPath;
    fs::path              EntryPointPath;
    std::vector<fs::path> CSources;
};

class BuildDriver {
    fs::path                              _projectRoot;
    std::vector<std::string>              _compilationQueue;
    std::unordered_map<std::string, File> _graph;

public:
    explicit BuildDriver (fs::path projectRoot)
        : _projectRoot (std::move (projectRoot)) {}

    static fs::path
    GetProjectRoot (fs::path curPath);

    void
    Build ();

private:
    static Manifest
    parseManifest (const fs::path &path);

    fs::path
    resolveImportPathToFilePath (const std::string &importPath);

    void
    scanDeps (const std::string &importPath);

    void
    sortDeps (const std::string &importPath);

    static std::vector<std::string>
    resolveDeps (const std::string &absolutePath);
};

}
