#pragma once
#include "ast/context.h"
#include "get_file_path.h"

#include <ast/dumper.h>
#include <basic/types/pool.h>
#include <diagnostic/engine.h>
#include <iostream>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_ostream.h>
#include <parser/parser.h>

namespace veo::testing {

struct ParserContext {
    llvm::SourceMgr              Mgr;
    diagnostic::DiagnosticEngine Diag;
    basic::TypePool              Pool;
    ast::Context                 ASTContext;

    ParserContext () : Diag (Mgr) {}

    ParseResult
    ParseFile (const std::string &fileName) {
        auto path        = GetFilePath (fileName);
        auto bufferOrErr = llvm::MemoryBuffer::getFile (path.string ());
        if (std::error_code ec = bufferOrErr.getError ()) {
            std::cerr << "Could not open file: " << ec.message () << '\n';
            return { .Nodes = nullptr, .Count = 0, .HasErrors = true };
        }
        unsigned bufferId
            = Mgr.AddNewSourceBuffer (std::move (*bufferOrErr), llvm::SMLoc ());

        Lexer  lex (Diag, Mgr, bufferId);
        Parser parser (Diag, lex, Pool, ASTContext);
        auto   res = parser.Parse ();
        if (Diag.HasErrors ()) {
            res.HasErrors = true;
        }
        return res;
    }

    static std::string
    GetSnaphot (ParseResult res) {
        std::string              buffer;
        llvm::raw_string_ostream os (buffer);
        ast::Dumper              dumper (os);
        dumper.Dump (res);
        return os.str ();
    }
};

}
