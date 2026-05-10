#pragma once
#include <filesystem>

namespace fs = std::filesystem;

namespace veo::driver {

struct CompilationResult {
    bool     Success;
    fs::path ObjPath;
};

CompilationResult
Compile (const fs::path &projectPath, const fs::path &filePath);

}
