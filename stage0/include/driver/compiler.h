#pragma once
#include <basic/symbols/module.h>
#include <basic/types/pool.h>
#include <driver/cli_options.h>
#include <filesystem>
#include <llvm/IR/Module.h>
#include <llvm/Target/TargetMachine.h>

namespace fs = std::filesystem;

namespace veo::driver {

struct CompilationResult {
    bool     Success;
    fs::path ObjPath;
};

void
InitializeLLVMTargets ();

bool
EmitFile (
    llvm::Module         *mod,
    llvm::TargetMachine  *targetMachine,
    const std::string    &fileName,
    llvm::CodeGenFileType fileType);

bool
EmitObjectFile (
    llvm::Module        *mod,
    llvm::TargetMachine *targetMachine,
    const std::string   &fileName,
    const fs::path      &projectPath);

bool
LinkObjectFiles (
    const std::string              &target,
    const std::string              &exeFile,
    const std::vector<std::string> &objFiles);

std::string
GetOutputName (const std::string &inputFile, const llvm::Triple &triple);

void
Optimize (llvm::Module &mod, OptLevel level);

bool
CompileCFile (
    const std::string &target,
    const std::string &cSource,
    const std::string &objFile,
    OptLevel           level);

CompilationResult
Compile (
    const fs::path     &projectPath,
    const fs::path     &filePath,
    const fs::path     &objPath,
    symbols::Module    *mod,
    basic::TypePool    &typePool,
    const llvm::Triple &triple);

}
