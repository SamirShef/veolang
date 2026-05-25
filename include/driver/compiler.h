#pragma once
#include <basic/symbols/module.h>
#include <driver/cli_options.h>
#include <filesystem>
#include <llvm/IR/Module.h>
#include <llvm/Target/TargetMachine.h>

// NOLINTBEGIN
namespace lld {

namespace elf {

bool
link (
    llvm::ArrayRef<const char *> args,
    llvm::raw_ostream           &stdoutOS,
    llvm::raw_ostream           &stderrOS,
    bool                         exitEarly,
    bool                         disableOutput);

}

namespace mingw {

bool
link (
    llvm::ArrayRef<const char *> args,
    llvm::raw_ostream           &stdoutOS,
    llvm::raw_ostream           &stderrOS,
    bool                         exitEarly,
    bool                         disableOutput);

}

namespace coff {

bool
link (
    llvm::ArrayRef<const char *> args,
    llvm::raw_ostream           &stdoutOS,
    llvm::raw_ostream           &stderrOS,
    bool                         exitEarly,
    bool                         disableOutput);

}

namespace macho {

bool
link (
    llvm::ArrayRef<const char *> args,
    llvm::raw_ostream           &stdoutOS,
    llvm::raw_ostream           &stderrOS,
    bool                         exitEarly,
    bool                         disableOutput);

}

}
// NOLINTEND

namespace fs = std::filesystem;

namespace veo::driver {

enum class LinkerFlavor : uint8_t { Elf, MinGW, Coff, MachO, Unknown };

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
    llvm::Module *mod, const std::string &fileName, const llvm::Triple &triple);

LinkerFlavor
SelectLinkerFlavor (const std::string &targetTriple);

bool
LinkObjectFiles (
    const std::string              &target,
    const std::string              &exeFile,
    const std::vector<std::string> &objFiles);

std::string
GetOutputName (const std::string &inputFile, const llvm::Triple &triple);

void
Optimize (llvm::Module &mod, OptLevel level);

CompilationResult
Compile (
    const fs::path     &projectPath,
    const fs::path     &filePath,
    const fs::path     &objPath,
    symbols::Module    *mod,
    const llvm::Triple &triple);

}
