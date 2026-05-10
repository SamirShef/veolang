#pragma once
#include <cstdint>
#include <filesystem>
#include <llvm/IR/Module.h>
#include <llvm/Support/CommandLine.h>

namespace fs = std::filesystem;

namespace veo::driver {

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
inline llvm::cl::OptionCategory Category ("Veo Compiler Options");

inline llvm::cl::SubCommand NewSub ("new", "Create a new project");

inline llvm::cl::opt<std::string> NewNameOpt (
    llvm::cl::Positional,
    llvm::cl::desc ("<name>"),
    llvm::cl::Required,
    llvm::cl::sub (NewSub));

inline llvm::cl::SubCommand InitSub ("init", "Initialize a project in current directory");
inline llvm::cl::SubCommand BuildSub ("build", "Build the current project");
inline llvm::cl::SubCommand RunSub ("run", "Build the current project and run it");
inline llvm::cl::SubCommand
    TestSub ("test", "Executing functions which marked as [test]");
inline llvm::cl::SubCommand
    CheckSub ("check", "Checking code without dumping build artefacts");
inline llvm::cl::SubCommand CleanSub ("clean", "Clean build directory");
inline llvm::cl::SubCommand FetchSub ("fetch", "Update the module registry");

enum class OptLevel : uint8_t { O0, O1, O2, O3 };

inline llvm::cl::opt<OptLevel> OptimizationLevelOpt (
    llvm::cl::desc ("Optimization level:"),
    llvm::cl::values (
        clEnumValN (OptLevel::O0, "O0", "No optimization"),
        clEnumValN (OptLevel::O1, "O1", "Basic optimization"),
        clEnumValN (OptLevel::O2, "O2", "Default optimization"),
        clEnumValN (OptLevel::O3, "O3", "Aggressive optimization")),
    llvm::cl::init (OptLevel::O0),
    llvm::cl::cat (Category));

inline llvm::cl::opt<std::string> TargetTripleOpt (
    "target",
    llvm::cl::desc ("Specify target triple"),
    llvm::cl::value_desc ("triple"),
    llvm::cl::init (""),
    llvm::cl::Prefix,
    llvm::cl::cat (Category));

inline llvm::cl::opt<bool> ForceRebuildOpt (
    "force-rebuild",
    llvm::cl::desc ("Rebuild project without checking of cache"),
    llvm::cl::cat (Category));

inline llvm::cl::opt<bool> EmitIROpt (
    "emit-ir", llvm::cl::desc ("Emits LLVM IR to .ll file "), llvm::cl::cat (Category));

inline llvm::cl::opt<bool> DumpSymOpt (
    "dump-sym",
    llvm::cl::desc ("Dumps symbol table even module to .veomodtxt file"),
    llvm::cl::cat (Category));

inline llvm::cl::opt<std::string> ExplainOpt (
    "explain",
    llvm::cl::desc ("Explain diagnostic code"),
    llvm::cl::value_desc ("code"),
    llvm::cl::init (""),
    llvm::cl::cat (Category));

enum class DumpASTInto : uint8_t { None, File, Terminal };

inline llvm::cl::opt<DumpASTInto> DumpASTOpt (
    "dump-ast",
    llvm::cl::desc ("Dumps AST even module to .txt file"),
    llvm::cl::values (
        clEnumValN (DumpASTInto::None, "none", "Does not dump AST"),
        clEnumValN (DumpASTInto::File, "file", "Dump AST to .txt file"),
        clEnumValN (DumpASTInto::Terminal, "term", "Dump AST to stderr")),
    llvm::cl::init (DumpASTInto::None),
    llvm::cl::cat (Category));

// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

inline int
ParseArguments (int argc, char **argv) {
    llvm::cl::HideUnrelatedOptions (Category);
    llvm::cl::ParseCommandLineOptions (argc, argv, "Bloop Compiler\n");

    if (argc < 2) {
        llvm::cl::PrintHelpMessage ();
        return 1;
    }
    return 0;
}

void
ExecuteArguments ();

void
InitNewPackage (const std::string &name);

void
CreateNewPackage (const std::string &name);

void
BuildPackage ();

void
RunPackage ();

void
TestPackage ();

void
CheckPackage ();

void
FetchRegistry ();

void
EmitIR (llvm::Module *mod, const fs::path &output);

void
Explain ();

inline void
PrintOptUnimplementedError (const std::string &optName) {
    llvm::errs () << llvm::raw_fd_ostream::RED << "Option '" << optName
                  << "' is unimplemented in current compiler version\n"
                  << llvm::raw_fd_ostream::RESET;
}

}
