#include <ast/dumper.h>
#include <codegen/codegen.h>
#include <diagnostic/engine.h>
#include <driver/cli_options.h>
#include <driver/compiler.h>
#include <filesystem>
#include <lexer/lexer.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/MC/TargetRegistry.h>
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
EmitObjectFile (
    llvm::Module *mod, const std::string &fileName, std::string targetTripleStr) {
    if (targetTripleStr.empty ()) {
        targetTripleStr = llvm::sys::getDefaultTargetTriple ();
    }

    llvm::Triple triple (targetTripleStr);

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

    std::error_code      ec;
    llvm::raw_fd_ostream dest (fileName, ec, llvm::sys::fs::OF_None);

    if (ec) {
        llvm::errs () << llvm::raw_fd_ostream::RED << "Could not open file " << fileName
                      << ": " << ec.message () << '\n'
                      << llvm::raw_fd_ostream::RESET;
        return false;
    }

    llvm::legacy::PassManager pass;
    auto                      fileType = llvm::CodeGenFileType::ObjectFile;

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
LinkObjectFiles (const std::string &exeFile, const std::vector<std::string> &objFiles) {
    std::string objs;
    for (int i = 0; i < objFiles.size (); ++i) {
        objs += objFiles[i];
        if (i < objFiles.size () - 1) {
            objs += ' ';
        }
    }
    // int res = llvm::sys::ExecuteAndWait (
    //     "clang",
    //     { "clang", objs, "-o", exeFile, "-fuse-ld=lld" });
    // return res >= 0;
    return system (("clang " + objs + " -o " + exeFile).c_str ()) == EXIT_SUCCESS;
}

std::string
GetOutputName (const std::string &inputFile, const llvm::Triple &triple) {
    std::string outputExe = llvm::sys::path::stem (inputFile).str ();
    if (triple.isOSWindows ()) {
        outputExe += ".exe";
    }
    return outputExe;
}

CompilationResult
Compile (
    const fs::path  &projectPath,
    const fs::path  &filePath,
    const fs::path  &objPath,
    symbols::Module *mod) {
    fs::path filePathInBuild
        = projectPath / "build"
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

    Lexer       lex (diag, mgr, bufferId);
    Parser      parser (diag, lex);
    ParseResult parseRes = parser.Parse ();

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
            << "\nAST dumped to " << outputPath.string () << '\n'
            << llvm::raw_fd_ostream::RESET;
        dumper.Dump (parseRes);
    }

    hir::Context ctx;
    Sema         sema (diag, ctx, mod);
    sema.Analyze (parseRes);

    diag.Render ();
    if (diag.HasErrors ()) {
        exit (1);
    }

    CodeGen codegen (mod->Name, ctx.Globals (), ctx.Functions ());
    auto    llvmMod = codegen.Generate ();

    InitializeLLVMTargets ();
    std::string  tripleStr = llvm::sys::getDefaultTargetTriple ();
    llvm::Triple triple (tripleStr);

    if (EmitIROpt) {
        std::error_code       ec;
        std::filesystem::path llvmIRPath = objPath;
        llvm::raw_fd_ostream  os (llvmIRPath.replace_extension (".ll").string (), ec);
        if (ec) {
            llvm::errs () << llvm::raw_fd_ostream::RED << ec.message ()
                          << llvm::raw_fd_ostream::RESET;
            exit (1);
        }
        llvmMod->print (os, nullptr);
    }

    if (!EmitObjectFile (llvmMod.get (), objPath.string (), tripleStr)) {
        return { .Success = false, .ObjPath = "" };
    }

    return { .Success = !parseRes.HasErrors, .ObjPath = "" };
}

}
