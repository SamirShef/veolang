#pragma once
#include <basic/symbols/module.h>
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
    llvm::Module      *mod,
    const std::string &fileName,
    std::string        targetTripleStr,
    const fs::path    &projectPath);

bool
LinkObjectFiles (const std::string &exeFile, const std::vector<std::string> &objFiles);

std::string
GetOutputName (const std::string &inputFile, const llvm::Triple &triple);

void
Optimize (llvm::Module &mod, OptLevel level);

CompilationResult
Compile (
    const fs::path  &projectPath,
    const fs::path  &filePath,
    const fs::path  &objPath,
    symbols::Module *mod);

}
