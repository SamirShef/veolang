#pragma once
#include <cstdint>
#include <llvm/Support/CommandLine.h>

namespace veo::driver {

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
static llvm::cl::OptionCategory Category ("Veo Compiler Options");

static llvm::cl::SubCommand NewSub ("new", "Create a new project");

static llvm::cl::opt<std::string> NewName (
        llvm::cl::Positional,
        llvm::cl::desc ("<name>"),
        llvm::cl::Required,
        llvm::cl::sub (NewSub));

static llvm::cl::SubCommand
        InitSub ("init", "Initialize a project in current directory");
static llvm::cl::SubCommand
        BuildSub ("build", "Build the current project");
static llvm::cl::SubCommand
        RunSub ("run", "Build the current project and run it");
static llvm::cl::SubCommand
        TestSub ("test", "Executing functions which marked as [test]");
static llvm::cl::SubCommand CheckSub (
        "check", "Checking code without dumping build artefacts");
static llvm::cl::SubCommand CleanSub ("clean", "Clean build directory");
static llvm::cl::SubCommand
        FetchSub ("fetch", "Update the module registry");

enum class OptLevel : uint8_t { O0, O1, O2, O3 };

static llvm::cl::opt<OptLevel> OptimizationLevel (
        llvm::cl::desc ("Optimization level:"),
        llvm::cl::values (
                clEnumValN (OptLevel::O0, "O0", "No optimization"),
                clEnumValN (OptLevel::O1, "O1", "Basic optimization"),
                clEnumValN (OptLevel::O2, "O2", "Default optimization"),
                clEnumValN (
                        OptLevel::O3, "O3", "Aggressive optimization")),
        llvm::cl::init (OptLevel::O0),
        llvm::cl::cat (Category));

static llvm::cl::opt<std::string> TargetTriple (
        "target",
        llvm::cl::desc ("Specify target triple"),
        llvm::cl::value_desc ("triple"),
        llvm::cl::init (""),
        llvm::cl::Prefix,
        llvm::cl::cat (Category));

static llvm::cl::opt<bool> ForceRebuild (
        "force-rebuild",
        llvm::cl::desc ("Rebuild project without checking of cache"),
        llvm::cl::cat (Category));

static llvm::cl::opt<bool>
        EmitIR ("emit-ir",
                llvm::cl::desc ("Emits LLVM IR to .ll file "),
                llvm::cl::cat (Category));

static llvm::cl::opt<bool> DumpMod (
        "dump-mod",
        llvm::cl::desc (
                "Dumps symbol table even module to .veomodtxt file"),
        llvm::cl::cat (Category));

static llvm::cl::opt<std::string> Explain (
        "explain",
        llvm::cl::desc ("Explain diagnostic code"),
        llvm::cl::value_desc ("code"),
        llvm::cl::init (""),
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

}
