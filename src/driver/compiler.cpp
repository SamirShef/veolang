#include <ast/context.h>
#include <ast/dumper.h>
#include <basic/types/pool.h>
#include <codegen/codegen.h>
#include <diagnostic/engine.h>
#include <driver/cli_options.h>
#include <driver/compiler.h>
#include <filesystem>
#include <lexer/lexer.h>
#include <lld/Common/Driver.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Passes/OptimizationLevel.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/CodeGen.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/Program.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>
#include <parser/parser.h>
#include <sema/sema.h>

namespace veo::driver {

void
InitializeLLVMTargets () {
    llvm::InitializeAllTargetInfos ();
    llvm::InitializeAllTargets ();
    llvm::InitializeAllTargetMCs ();
    llvm::InitializeAllAsmParsers ();
    llvm::InitializeAllAsmPrinters ();
}

bool
EmitFile (
    llvm::Module         *mod,
    llvm::TargetMachine  *targetMachine,
    const std::string    &fileName,
    llvm::CodeGenFileType fileType) {
    std::error_code      ec;
    llvm::raw_fd_ostream dest (fileName, ec, llvm::sys::fs::OF_None);

    if (ec) {
        llvm::errs () << llvm::raw_fd_ostream::RED << "Could not open file " << fileName
                      << ": " << ec.message () << '\n'
                      << llvm::raw_fd_ostream::RESET;
        return false;
    }

    llvm::legacy::PassManager pass;
    if (targetMachine->addPassesToEmitFile (pass, dest, nullptr, fileType)) {
        llvm::errs () << llvm::raw_fd_ostream::RED
                      << "TargetMachine can't emit a file of this type\n"
                      << llvm::raw_fd_ostream::RESET;
        return false;
    }

    pass.run (*mod);
    dest.flush ();

    return true;
}

bool
EmitObjectFile (
    llvm::Module *mod, const std::string &fileName, const llvm::Triple &triple) {
    mod->setTargetTriple (triple);

    std::string         error;
    const llvm::Target *target = llvm::TargetRegistry::lookupTarget (triple, error);

    if (target == nullptr) {
        llvm::errs () << llvm::raw_fd_ostream::RED
                      << "Error looking up target: " << llvm::raw_fd_ostream::RESET
                      << error << '\n';
        return false;
    }

    std::string cpu = "generic";
    std::string features;

    llvm::TargetOptions               opt;
    std::optional<llvm::Reloc::Model> rm = llvm::Reloc::Model::PIC_;

    auto *targetMachine = target->createTargetMachine (triple, cpu, features, opt, rm);

    mod->setDataLayout (targetMachine->createDataLayout ());

    if (!EmitFile (mod, targetMachine, fileName, llvm::CodeGenFileType::ObjectFile)) {
        return false;
    }
    if (EmitAsmOpt) {
        std::string asmFileName = fileName;
        size_t      dotPos      = asmFileName.find_last_of ('.');
        if (dotPos != std::string::npos) {
            asmFileName = asmFileName.substr (0, dotPos) + ".s";
        } else {
            asmFileName += ".s";
        }

        if (!EmitFile (
                mod,
                targetMachine,
                asmFileName,
                llvm::CodeGenFileType::AssemblyFile)) {
            return false;
        }
    }

    return true;
}

LinkerFlavor
SelectLinkerFlavor (const std::string &targetTriple) {
    llvm::Triple triple (targetTriple);

    if (triple.isOSWindows ()) {
        if (triple.getEnvironment () == llvm::Triple::GNU) {
            return LinkerFlavor::MinGW;
        }
        return LinkerFlavor::Coff;
    }

    if (triple.isMacOSX () || triple.isOSDarwin ()) {
        return LinkerFlavor::MachO;
    }

    if (triple.isOSLinux () || triple.isOSFreeBSD ()) {
        return LinkerFlavor::Elf;
    }

    return LinkerFlavor::Unknown;
}

bool
LinkObjectFiles (
    const std::string              &target,
    const std::string              &exeFile,
    const std::vector<std::string> &objFiles) {
    LinkerFlavor flavor = SelectLinkerFlavor (target);
    if (flavor == LinkerFlavor::Unknown) {
        llvm::errs ().changeColor (llvm::raw_fd_ostream::RED, true)
            << "Error: Unsupported target triple for linking: "
            << llvm::raw_fd_ostream::RESET << target << '\n';
        return false;
    }
    std::vector<const char *> args;
    args.push_back ("veoc-linker");

    for (const auto &obj : objFiles) {
        args.push_back (obj.c_str ());
    }

    args.push_back ("-o");
    args.push_back (exeFile.c_str ());

    llvm::raw_ostream &diagOs = llvm::errs ();
    llvm::raw_ostream &infoOs = llvm::outs ();

    switch (flavor) {
#define variant(kind, name)                                                              \
    case LinkerFlavor::kind: return lld::name::link (args, diagOs, infoOs, false, false);
        variant (Elf, elf);
        variant (MinGW, mingw);
        variant (Coff, coff);
        variant (MachO, macho);
    default: return false;
#undef variant
    }
}

std::string
GetOutputName (const std::string &inputFile, const llvm::Triple &triple) {
    std::string outputExe = llvm::sys::path::stem (inputFile).str ();
    if (triple.isOSWindows ()) {
        outputExe += ".exe";
    }
    return outputExe;
}

void
Optimize (llvm::Module &mod, OptLevel level) {
    if (level == OptLevel::O0) {
        return;
    }

    llvm::LoopAnalysisManager     lam;
    llvm::FunctionAnalysisManager fam;
    llvm::CGSCCAnalysisManager    cgam;
    llvm::ModuleAnalysisManager   mam;

    llvm::PassBuilder pb;

    pb.registerModuleAnalyses (mam);
    pb.registerCGSCCAnalyses (cgam);
    pb.registerFunctionAnalyses (fam);
    pb.registerLoopAnalyses (lam);
    pb.crossRegisterProxies (lam, fam, cgam, mam);

    llvm::ModulePassManager mpm;
    llvm::OptimizationLevel llvmLevel;
    switch (level) {
#define variant(kind)                                                                    \
    case OptLevel::kind: llvmLevel = llvm::OptimizationLevel::kind; break;
        variant (O0);
        variant (O1);
        variant (O2);
        variant (O3);
        variant (Os);
        variant (Oz);
#undef variant
    }

    mpm = pb.buildPerModuleDefaultPipeline (llvmLevel);

    mpm.run (mod, mam);
}

CompilationResult
Compile (
    const fs::path     &projectPath,
    const fs::path     &filePath,
    const fs::path     &objPath,
    symbols::Module    *mod,
    const llvm::Triple &triple) {
    fs::path filePathInBuild
        = projectPath / "build" / "obj"
          / fs::absolute (filePath).lexically_relative (projectPath).lexically_normal ();
    fs::create_directories (filePathInBuild.parent_path ());
    llvm::SourceMgr              mgr;
    diagnostic::DiagnosticEngine diag (mgr);
    auto bufferOrErr = llvm::MemoryBuffer::getFile (filePath.string ());
    if (std::error_code ec = bufferOrErr.getError ()) {
        llvm::errs ()
            << llvm::raw_fd_ostream::RED
            << filePath.lexically_relative (projectPath).lexically_normal ().string ()
            << ": " << ec.message () << '\n'
            << llvm::raw_fd_ostream::RESET;
        exit (1);
    }
    unsigned bufferId = mgr.AddNewSourceBuffer (std::move (*bufferOrErr), llvm::SMLoc ());

    TypePool     pool;
    ast::Context context;
    Lexer        lex (diag, mgr, bufferId);
    Parser       parser (diag, lex, pool, context);
    ParseResult  parseRes = parser.Parse ();
    if (diag.HasErrors ()) {
        parseRes.HasErrors = true;
    }

    if (DumpASTOpt == DumpASTInto::Terminal) {
        ast::Dumper dumper (llvm::errs ());
        llvm::errs ().changeColor (llvm::raw_fd_ostream::WHITE, true)
            << "\nAST Dump:\n"
            << llvm::raw_fd_ostream::RESET;
        dumper.Dump (parseRes);
    } else if (DumpASTOpt == DumpASTInto::File) {
        std::error_code      ec;
        fs::path             outputPath = filePathInBuild.replace_extension (".txt");
        llvm::raw_fd_ostream output (outputPath.string (), ec);
        ast::Dumper          dumper (output);
        llvm::errs ().changeColor (llvm::raw_fd_ostream::WHITE, true)
            << "AST was dumped to "
            << outputPath.lexically_relative (projectPath).string () << '\n'
            << llvm::raw_fd_ostream::RESET;
        dumper.Dump (parseRes);
    }

    hir::Context ctx;
    Sema         sema (diag, ctx, mod, pool);
    sema.Analyze (parseRes);

    diag.Render ();
    if (diag.HasErrors ()) {
        exit (1);
    }

    CodeGen codegen (mod->Name, ctx.Globals (), ctx.Functions (), ctx.Structs ());
    auto    llvmMod = codegen.Generate ();

    InitializeLLVMTargets ();

    Optimize (*llvmMod, OptimizationLevelOpt);

    if (!EmitObjectFile (llvmMod.get (), objPath.string (), triple)) {
        return { .Success = false, .ObjPath = "" };
    }
    if (EmitIROpt) {
        std::error_code      ec;
        fs::path             llvmIRPath = objPath;
        llvm::raw_fd_ostream os (llvmIRPath.replace_extension (".ll").string (), ec);
        if (ec) {
            llvm::errs () << llvm::raw_fd_ostream::RED << ec.message ()
                          << llvm::raw_fd_ostream::RESET;
            exit (1);
        }
        llvmMod->print (os, nullptr);
        llvm::errs ().changeColor (llvm::raw_fd_ostream::WHITE, true)
            << "LLVM IR was dumped to "
            << llvmIRPath.lexically_relative (projectPath).string () << '\n'
            << llvm::raw_fd_ostream::RESET;
    }
    if (EmitAsmOpt) {
        std::string asmFileName = objPath.string ();
        size_t      dotPos      = asmFileName.find_last_of ('.');
        if (dotPos != std::string::npos) {
            asmFileName = asmFileName.substr (0, dotPos) + ".s";
        } else {
            asmFileName += ".s";
        }

        llvm::errs ().changeColor (llvm::raw_fd_ostream::WHITE, true)
            << "LLVM IR was dumped to "
            << fs::path (asmFileName).lexically_relative (projectPath).string () << '\n'
            << llvm::raw_fd_ostream::RESET;
    }

    return { .Success = !parseRes.HasErrors, .ObjPath = "" };
}

}
