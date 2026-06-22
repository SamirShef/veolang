#include <ast/context.h>
#include <ast/dumper.h>
#include <basic/types/pool.h>
#include <codegen/codegen.h>
#include <diagnostic/engine.h>
#include <driver/cli_options.h>
#include <driver/compiler.h>
#include <filesystem>
#include <hir_analyze/dead_code_eliminator.h>
#include <hir_analyze/return_checker.h>
#include <lexer/lexer.h>
#include <linearizer/linearizer.h>
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
    llvm::Module        *mod,
    llvm::TargetMachine *targetMachine,
    const std::string   &fileName,
    const fs::path      &projectPath) {
    if (EmitIROpt) {
        std::error_code ec;
        std::string     llvmFileName = fileName;
        size_t          dotPos       = llvmFileName.find_last_of ('.');
        if (dotPos != std::string::npos) {
            llvmFileName = llvmFileName.substr (0, dotPos) + ".ll";
        } else {
            llvmFileName += ".ll";
        }
        llvm::raw_fd_ostream os (llvmFileName, ec);
        mod->print (os, nullptr);
        llvm::errs ().changeColor (llvm::raw_fd_ostream::WHITE, true)
            << "LLVM IR was dumped to "
            << fs::path (llvmFileName).lexically_relative (projectPath).string () << '\n'
            << llvm::raw_fd_ostream::RESET;
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
        llvm::errs ().changeColor (llvm::raw_fd_ostream::WHITE, true)
            << "LLVM IR was dumped to "
            << fs::path (asmFileName).lexically_relative (projectPath).string () << '\n'
            << llvm::raw_fd_ostream::RESET;
    }
    if (!EmitFile (mod, targetMachine, fileName, llvm::CodeGenFileType::ObjectFile)) {
        return false;
    }

    return true;
}

bool
LinkObjectFiles (
    const std::string              &target,
    const std::string              &exeFile,
    const std::vector<std::string> &objFiles) {
    std::string objs;
    for (int i = 0; i < objFiles.size (); ++i) {
        objs += objFiles[i];
        if (i < objFiles.size () - 1) {
            objs += ' ';
        }
    }
    return system (("clang -fuse-ld=lld --target=\"" + target + "\" " + objs + " -o "
                    + exeFile)
                       .c_str ())
           == EXIT_SUCCESS;
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

bool
CompileCFile (
    const std::string &target,
    const std::string &cSource,
    const std::string &objFile,
    OptLevel           level) {
    std::string optFlag = "-O0";
    switch (level) {
#define variant(kind)                                                                    \
    case OptLevel::kind: optFlag = "-" #kind; break;
        variant (O0);
        variant (O1);
        variant (O2);
        variant (O3);
        variant (Os);
        variant (Oz);
#undef variant
    }

    std::string cmd = "clang --target=\"" + target + "\" " + optFlag + " -c \"" + cSource
                      + "\" -o \"" + objFile + "\"";

    return system (cmd.c_str ()) == EXIT_SUCCESS;
}

CompilationResult
Compile (
    const fs::path     &projectPath,
    const fs::path     &filePath,
    const fs::path     &objPath,
    symbols::Module    *mod,
    TypePool           &typePool,
    const llvm::Triple &triple) {
    fs::path filePathInBuild
        = projectPath / ("build/targets/" + triple.getTriple () + "/obj")
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

    ast::Context astContext;
    Lexer        lex (diag, mgr, bufferId);
    Parser       parser (diag, lex, typePool, astContext);
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

    hir::Context hirContext;
    hir::Builder builder (hirContext);

    unsigned ptrBitWidth = triple.isArch64Bit () ? 64 : 32;
    if (triple.isArch16Bit ()) {
        ptrBitWidth = 16;
    }
    Sema sema (diag, builder, astContext, mod, typePool, ptrBitWidth);
    sema.Analyze (parseRes);

    HIRLinearizer linearizer (builder);
    linearizer.Linearize ();

    for (auto *func : hirContext.Functions ()) {
        DeadCodeEliminator::RunOnFunction (func);
        ReturnChecker retChecker (diag);
        retChecker.RunOnFunction (func);
    }

    diag.Render ();
    if (diag.HasErrors ()) {
        exit (1);
    }

    InitializeLLVMTargets ();

    std::string         error;
    const llvm::Target *target = llvm::TargetRegistry::lookupTarget (triple, error);

    if (target == nullptr) {
        llvm::errs () << llvm::raw_fd_ostream::RED
                      << "Error looking up target: " << llvm::raw_fd_ostream::RESET
                      << error << '\n';
        return { .Success = false, .ObjPath = "" };
    }

    std::string cpu = "generic";
    std::string features;

    llvm::TargetOptions               opt;
    std::optional<llvm::Reloc::Model> rm = llvm::Reloc::Model::PIC_;

    auto *targetMachine = target->createTargetMachine (triple, cpu, features, opt, rm);
    auto  dataLayout    = targetMachine->createDataLayout ();

    CodeGen codegen (
        mgr,
        mod,
        hirContext.Globals (),
        hirContext.Functions (),
        hirContext.Structs (),
        triple,
        dataLayout);
    auto llvmMod = codegen.Generate ();

    Optimize (*llvmMod, OptimizationLevelOpt);

    if (!EmitObjectFile (llvmMod.get (), targetMachine, objPath.string (), projectPath)) {
        return { .Success = false, .ObjPath = "" };
    }

    return { .Success = !diag.HasErrors (), .ObjPath = objPath };
}
}
