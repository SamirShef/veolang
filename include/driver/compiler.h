#pragma once
#include <basic/symbols/module.h>
#include <filesystem>
#include <llvm/IR/Module.h>

namespace fs = std::filesystem;

namespace veo::driver {

struct CompilationResult {
    bool     Success;
    fs::path ObjPath;
};

void
InitializeLLVMTargets ();

bool
EmitObjectFile (
    llvm::Module *mod, const std::string &fileName, std::string targetTripleStr);

bool
LinkObjectFiles (const std::string &exeFile, const std::vector<std::string> &objFiles);

std::string
GetOutputName (const std::string &inputFile, const llvm::Triple &triple);

CompilationResult
Compile (
    const fs::path  &projectPath,
    const fs::path  &filePath,
    const fs::path  &objPath,
    symbols::Module *mod);

}
