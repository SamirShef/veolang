#include <algorithm>
#include <cmath>
#include <diagnostic/builder.h>
#include <diagnostic/codes.h>
#include <diagnostic/engine.h>
#include <format>
#include <llvm/Support/raw_ostream.h>

namespace veo::diagnostic {

int
DigitCount (int line) {
    if (line == 0) {
        return 1;
    }
    return static_cast<int> (std::log10 (line)) + 1;
}

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

char
SeverityToPrefix (Severity severity) {
    return (char) toupper (SeverityToString (severity)[0]);
}

llvm::raw_fd_ostream::Colors
SeverityToColor (Severity severity) {
#define func_case(expr, col)                                                             \
    case Severity::expr: return llvm::raw_fd_ostream::col;

    switch (severity) {
        func_case (Error, RED);
        func_case (Warning, YELLOW);
        func_case (Note, WHITE);
        func_case (Help, CYAN);
    }

#undef func_case

    return llvm::raw_fd_ostream::WHITE;
}

int
DiagCodeToIntegerCode (DiagCode code) {
    if (code <= errCodeLast) {
        return static_cast<int> (code);
    }
    if (code <= warnCodeLast) {
        return static_cast<int> (
            static_cast<uint8_t> (code) - static_cast<uint8_t> (warnCodeStart));
    }
    return 0;
}

void
DiagnosticEngine::renderDiag (DiagnosticBuilder &diag) {
    std::ranges::sort (diag.Spans (), [&] (const Annotation &a, const Annotation &b) {
        return _mgr->getLineAndColumn (a.Span.Start).first
               < _mgr->getLineAndColumn (b.Span.Start).first;
    });
    printDiagnosticHeader (diag);
    printDiagnosticBody (diag);
}

void // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
DiagnosticEngine::printDiagnosticHeader (DiagnosticBuilder &diag) {
    llvm::errs ().changeColor (SeverityToColor (diag.GetSeverity ()), true);

    llvm::errs () << SeverityToString (diag.GetSeverity ()) << llvm::raw_fd_ostream::WHITE
                  << '[';
    llvm::errs ().changeColor (SeverityToColor (diag.GetSeverity ()), true);
    std::string errCode = std::format ("{:04}", DiagCodeToIntegerCode (diag.Code ()));
    llvm::errs () << SeverityToPrefix (diag.GetSeverity ()) << errCode
                  << llvm::raw_fd_ostream::WHITE << "]: " << diag.Message () << '\n';
}

void
DiagnosticEngine::printDiagnosticBody (DiagnosticBuilder &diag) {
    int         maxLineWidth = 1;
    std::string lastBufferId;
    for (const auto &span : diag.Spans ()) {
        unsigned           buffer = _mgr->FindBufferContainingLoc (span.Span.Start);
        const std::string &bufferId
            = _mgr->getBufferInfo (buffer).Buffer->getBufferIdentifier ().str ();
        const char *bufferStart = _mgr->getBufferInfo (buffer).Buffer->getBufferStart ();
        const char *bufferEnd   = _mgr->getBufferInfo (buffer).Buffer->getBufferEnd ();
        auto        lineAndCol  = _mgr->getLineAndColumn (span.Span.Start, buffer);
        int         maxLine     = static_cast<int> (
            _mgr->getLineAndColumn (diag.Spans ().back ().Span.Start, buffer).first);
        maxLineWidth = DigitCount (maxLine);

        if (span == diag.Spans ().front ()
            || !lastBufferId.empty () && lastBufferId != bufferId) {
            if (span != diag.Spans ().front ()) {
                llvm::errs () << '\n';
            }
            llvm::errs ().changeColor (llvm::raw_fd_ostream::WHITE);
            llvm::errs () << std::string (maxLineWidth, ' ') << " --> " << bufferId << ':'
                          << lineAndCol.first << ':' << lineAndCol.second << '\n';
            lastBufferId = bufferId;
        }

        if (span == diag.Spans ().front ()) {
            llvm::errs ().changeColor (llvm::raw_fd_ostream::WHITE, true);
            llvm::errs () << std::string (maxLineWidth, ' ') << "  |\n";
        }

        llvm::errs ().changeColor (llvm::raw_fd_ostream::YELLOW, true)
            << std::format (" {:{}} ", lineAndCol.first, maxLineWidth);
        llvm::errs ().changeColor (llvm::raw_fd_ostream::WHITE, true) << "| ";

        const char *lineStart = span.Span.Start.getPointer ();
        const char *lineEnd   = lineStart;
        for (; *(lineStart - 1) != '\n' && lineStart > bufferStart; --lineStart) {
        }
        for (; *lineEnd != '\n' && lineEnd <= bufferEnd; ++lineEnd) {
        }
        llvm::errs () << std::string (lineStart, lineEnd - lineStart) << '\n';

        llvm::errs () << std::string (maxLineWidth, ' ') << "  | ";
        llvm::errs () << std::string (span.Span.Start.getPointer () - lineStart, ' ');
        char underlineSymbol = span.IsPrimary ? '^' : '-';
        llvm::errs ().changeColor (llvm::raw_fd_ostream::RED, true) << std::string (
            span.Span.End.getPointer () - span.Span.Start.getPointer (),
            underlineSymbol);
        llvm::errs ().changeColor (llvm::raw_fd_ostream::WHITE, true);
        if (!span.Label.empty ()) {
            llvm::errs () << ' ' << span.Label;
        }
        llvm::errs () << '\n';

        if (span == diag.Spans ().back ()) {
            llvm::errs () << std::string (maxLineWidth, ' ') << "  |\n"
                          << llvm::raw_fd_ostream::RESET;
        }
    }

    for (const std::string &note : diag.Notes ()) {
        llvm::errs ().changeColor (llvm::raw_fd_ostream::CYAN, true)
            << std::string (maxLineWidth, ' ') << "  = note: ";
        llvm::errs () << llvm::raw_fd_ostream::RESET << note << '\n';
    }
}

}
