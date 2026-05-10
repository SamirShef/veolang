#include "driver/cli_options.h"

#include <ast/dumper.h>
#include <diagnostic/engine.h>
#include <driver/compiler.h>
#include <filesystem>
#include <lexer/lexer.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_ostream.h>
#include <parser/parser.h>

namespace veo::driver {

CompilationResult
Compile (const fs::path &projectPath, const fs::path &filePath) {
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
    diag.Render ();

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

    return { .Success = !parseRes.HasErrors, .ObjPath = "" };
}

}
