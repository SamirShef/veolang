#include <algorithm>
#include <cmath>
#include <diagnostic/builder.h>
#include <diagnostic/codes.h>
#include <diagnostic/engine.h>
#include <format>
#include <llvm/Support/raw_ostream.h>

namespace veo::diagnostic {

int
GetDigitCount (int line) {
    if (line == 0) {
        return 1;
    }
    return static_cast<int> (std::log10 (line)) + 1;
}

std::string
SeverityToString (Severity severity) {
#define case(expr, res)                                                   \
    case Severity::expr: return res;

    switch (severity) {
        case (Error, "error");
        case (Warning, "warning");
        case (Note, "note");
        case (Help, "help");
    }

#undef case

    return "error";
}

char
SeverityToPrefix (Severity severity) {
    return (char) toupper(SeverityToString(severity)[0]);
}

llvm::raw_fd_ostream::Colors
SeverityToColor(Severity severity) {
#define case(expr, col)                                                   \
        case Severity::expr: return llvm::raw_fd_ostream::col;

    switch (severity) {
        case (Error, RED);
        case (Warning, YELLOW);
        case (Note, WHITE);
        case (Help, CYAN);
    }

#undef case

    return llvm::raw_fd_ostream::WHITE;
}

int
DiagCodeToIntegerCode(DiagCode code) {
    if (code <= errCodeLast) {
        return static_cast<int>(code);
    }
    if (code <= warnCodeLast) {
        return static_cast<int>(static_cast<uint8_t>(code) - static_cast<uint8_t>(warnCodeStart));
    }
    return 0;
}

void
DiagnosticEngine::renderDiag (DiagnosticBuilder &diag) {
    std::ranges::sort(diag.GetSpans(), [&](const Annotation &a, const Annotation &b) {
        return _mgr->getLineAndColumn(a.Span.Start).first < _mgr->getLineAndColumn(b.Span.Start).first;
    });
    printDiagnosticHeader(diag);
    printDiagnosticBody(diag);
}

void
DiagnosticEngine::printDiagnosticHeader(DiagnosticBuilder &diag) {
    llvm::errs().changeColor (SeverityToColor (diag.GetSeverity ()), true);

    llvm::errs() << SeverityToString (diag.GetSeverity ()) << llvm::raw_fd_ostream::WHITE << '[';
    llvm::errs().changeColor (SeverityToColor (diag.GetSeverity()), true);
    std::string errCode = std::format ("{:04}", DiagCodeToIntegerCode(diag.GetCode()));
    llvm::errs() << SeverityToPrefix (diag.GetSeverity ()) << errCode << llvm::raw_fd_ostream::WHITE << "]: " << diag.GetMessage () << '\n';
}

// clang-format off
void
DiagnosticEngine::printDiagnosticBody (DiagnosticBuilder &diag) {
    int maxLineWidth = 1;
    std::string lastBufferId;
    for (const auto &span : diag.GetSpans()) {
        unsigned buffer = _mgr->FindBufferContainingLoc (span.Span.Start);
        const std::string &bufferId
            = _mgr->getBufferInfo(buffer).Buffer->getBufferIdentifier ().str ();
        auto lineAndCol = _mgr->getLineAndColumn (span.Span.Start, buffer);
        int maxLine = static_cast<int>(_mgr->getLineAndColumn(diag.GetSpans().back().Span.Start, buffer).first);
        maxLineWidth = GetDigitCount(maxLine);

        if (span == diag.GetSpans().front() || !lastBufferId.empty() && lastBufferId != bufferId) {
            if (span != diag.GetSpans().front()) {
                llvm::errs() << '\n';
            }
            llvm::errs().changeColor(llvm::raw_fd_ostream::WHITE);
            llvm::errs() << std::string(maxLineWidth, ' ') << " --> " << bufferId << ':' << lineAndCol.first << ':' << lineAndCol.second << '\n';
            lastBufferId = bufferId;
        }

        if (span == diag.GetSpans().front()) {
            llvm::errs().changeColor(llvm::raw_fd_ostream::WHITE, true);
            llvm::errs() << std::string(maxLineWidth, ' ') << "  |\n";
        }

        llvm::errs ().changeColor(llvm::raw_fd_ostream::YELLOW, true) << std::format(" {:{}} ", lineAndCol.first, maxLineWidth);
        llvm::errs().changeColor(llvm::raw_fd_ostream::WHITE, true) << "| ";

        const char *lineStart = span.Span.Start.getPointer ();
        const char *lineEnd = lineStart;
        for (; *(lineStart - 1) != '\n'; --lineStart) {}
        for (; *lineEnd != '\n'; ++lineEnd) {}
        llvm::errs () << std::string (lineStart, lineEnd - lineStart) << '\n';

        llvm::errs() << std::string(maxLineWidth, ' ') << "  | ";
        llvm::errs() << std::string(span.Span.Start.getPointer() - lineStart, ' ');
        char underlineSymbol = span.IsPrimary ? '^' :
        '-';
        llvm::errs ().changeColor(llvm::raw_fd_ostream::RED, true) << std::string (span.Span.End.getPointer () - span.Span.Start.getPointer () + 1, underlineSymbol);
        llvm::errs().changeColor(llvm::raw_fd_ostream::WHITE, true);
        if (!span.Label.empty()) {
            llvm::errs() << ' ' << span.Label;
        }
        llvm::errs() << '\n';

        if (span == diag.GetSpans().back()) {
            llvm::errs () << std::string(maxLineWidth, ' ') << "  |\n" << llvm::raw_fd_ostream::RESET;
        }
    }

    for (const std::string &note : diag.GetNotes()) {
        llvm::errs().changeColor(llvm::raw_fd_ostream::CYAN, true) << std::string(maxLineWidth, ' ') << "  = note: ";
        llvm::errs() << llvm::raw_fd_ostream::RESET << note << '\n';
    }
}
        // clang-format on
}
