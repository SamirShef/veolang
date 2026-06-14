#pragma once
#include <filesystem>
#include <string>
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
    fs::path _projectRoot;

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
};

}
