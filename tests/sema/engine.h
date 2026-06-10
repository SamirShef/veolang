#pragma once
#include "commands_parser.h"

#include <ast/context.h>
#include <basic/types/pool.h>
#include <diagnostic/engine.h>
#include <filesystem>
#include <hir/builder.h>
#include <hir_analyze/dead_code_eliminator.h>
#include <hir_analyze/return_checker.h>
#include <lexer/lexer.h>
#include <linearizer/linearizer.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/TargetParser/Triple.h>
#include <parser/parser.h>
#include <sema/sema.h>

using namespace veo;

std::string
SeverityToString (Severity severity) {
#define func_case(expr, res)                                                             \
    case Severity::expr: return res;

    switch (severity) {
        func_case (Error, "error");
        func_case (Warning, "warning");
        func_case (Note, "note");
        func_case (Help, "help");
    }

#undef func_case

    return "error";
}

class TestEngine {
public:
    static bool
    Test (const std::filesystem::path &path) {
        llvm::SourceMgr              mgr;
        diagnostic::DiagnosticEngine diag (mgr);
        auto bufferOrErr = llvm::MemoryBuffer::getFile (path.string ());
        if (std::error_code ec = bufferOrErr.getError ()) {
            llvm::errs () << llvm::raw_fd_ostream::RED << path.string () << ": "
                          << ec.message () << '\n'
                          << llvm::raw_fd_ostream::RESET;
            exit (1);
        }
        unsigned bufferId
            = mgr.AddNewSourceBuffer (std::move (*bufferOrErr), llvm::SMLoc ());

        CommandsParser commandsParser (mgr, bufferId);
        auto           commands = commandsParser.Parse ();
        std::ranges::sort (commands, [&] (const Command &a, const Command &b) {
            return a.Line < b.Line;
        });

        basic::TypePool pool;
        ast::Context    astContext;
        Lexer           lex (diag, mgr, bufferId);
        Parser          parser (diag, lex, pool, astContext);
        ParseResult     parseRes = parser.Parse ();
        if (diag.HasErrors ()) {
            parseRes.HasErrors = true;
        }

        hir::Context hirContext;
        hir::Builder builder (hirContext);

        llvm::Triple triple (llvm::sys::getDefaultTargetTriple ());
        unsigned     ptrBitWidth = triple.isArch64Bit () ? 64 : 32;
        if (triple.isArch16Bit ()) {
            ptrBitWidth = 16;
        }
        auto *mod = new symbols::Module (path.stem ().string ());
        Sema  sema (diag, builder, astContext, mod, pool, ptrBitWidth);
        sema.Analyze (parseRes);

        HIRLinearizer linearizer (builder);
        linearizer.Linearize ();

        for (auto *func : hirContext.Functions ()) {
            DeadCodeEliminator::RunOnFunction (func);
            ReturnChecker retChecker (diag);
            retChecker.RunOnFunction (func);
        }

        diag.SortDiagSpans ();

        auto getBuilderLine = [&] (diagnostic::DiagnosticBuilder &b) -> unsigned {
            for (const auto &span : b.Spans ()) {
                if (span.IsPrimary) {
                    return mgr.FindLineNumber (span.Span.Start, bufferId);
                }
            }
            if (!b.Spans ().empty ()) {
                return mgr.FindLineNumber (b.Spans ()[0].Span.Start, bufferId);
            }
            return 0;
        };
        std::ranges::sort (
            diag.Builders (),
            [&] (diagnostic::DiagnosticBuilder &a, diagnostic::DiagnosticBuilder &b) {
                return getBuilderLine (a) < getBuilderLine (b);
            });

        if (!commands.empty () && commands[0].Kind == CommandKind::NoErrors) {
            if (diag.HasErrors ()) {
                llvm::errs () << "Expected success running, but got errors\n";
                delete mod; // NOLINT
                return false;
            }
            delete mod; // NOLINT
            return true;
        }
        if (commands.size () != diag.Builders ().size ()) {
            llvm::errs () << "Count of expected errors (" << commands.size ()
                          << ") is not equal to count of real errors ("
                          << diag.Builders ().size () << ")\n";
            delete mod; // NOLINT
            return false;
        }
        auto &builders = diag.Builders ();
        bool  ok       = true;
        for (size_t i = 0; i < commands.size (); ++i) {
            const auto &command = commands[i];
            auto       &diag    = builders[i];
            if (!validateDiag (command, diag, mgr, bufferId)) {
                ok = false;
            }
        }

        delete mod; // NOLINT
        return ok;
    }

private:
#define variant(kind)                                                                    \
    case CommandKind::kind: {                                                            \
        if (severity != Severity::kind) {                                                \
            ok = false;                                                                  \
            llvm::errs () << "Expected " << SeverityToString (Severity::kind)            \
                          << ", but got " << SeverityToString (severity) << '\n';        \
        }                                                                                \
        if (command.Line != line) {                                                      \
            ok = false;                                                                  \
            llvm::errs () << "Expected at " << command.Line << ", but got at " << line   \
                          << '\n';                                                       \
        }                                                                                \
        if (command.Val != diag.Message ()) {                                            \
            ok = false;                                                                  \
            llvm::errs () << "Expected \"" << command.Val << "\", but got \""            \
                          << diag.Message () << "\"\n";                                  \
        }                                                                                \
        break;                                                                           \
    }

    static bool
    validateDiag (
        const Command         &command,
        DiagnosticBuilder     &diag,
        const llvm::SourceMgr &mgr,
        unsigned               bufferId) {
        bool        ok       = true;
        auto        severity = diag.GetSeverity ();
        const auto &span     = diag.Spans ()[0];
        auto        line     = calculateLine (mgr, diag, bufferId, command.Line);
        switch (command.Kind) {
            variant (Error);
            variant (Warning);
            variant (Note);
        default: {
        }
        }

        return ok;
    }

    static unsigned
    calculateLine (
        const llvm::SourceMgr &mgr,
        DiagnosticBuilder     &diag,
        unsigned               bufferId,
        unsigned               commandLine) {
        unsigned line = 0;
        for (const auto &span : diag.Spans ()) {
            if (span.IsPrimary) {
                unsigned currentSpanLine = mgr.FindLineNumber (span.Span.Start, bufferId);
                if (currentSpanLine == commandLine) {
                    line = currentSpanLine;
                    break;
                }
                if (line == 0) {
                    line = currentSpanLine;
                }
            }
        }

        if (line == 0 && !diag.Spans ().empty ()) {
            line = mgr.FindLineNumber (diag.Spans ()[0].Span.Start, bufferId);
        }
        return line;
    }

#undef variant
};
