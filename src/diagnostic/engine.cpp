#include <diagnostic/builder.h>
#include <diagnostic/codes.h>
#include <diagnostic/engine.h>
#include <format>
#include <llvm/Support/raw_ostream.h>

namespace veo::diagnostic {

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
DiagnosticEngine::printDiagnosticHeader(DiagnosticBuilder &diag) {
    unsigned buffer = _mgr->FindBufferContainingLoc (
            diag.GetSpans ()[0].Span.Start);
    const auto        &bufferInfo = _mgr->getBufferInfo (buffer);
    const std::string &bufferId
            = bufferInfo.Buffer->getBufferIdentifier ().str ();

    llvm::errs().changeColor (SeverityToColor (diag.GetSeverity ()), true);

    llvm::errs() << SeverityToString (diag.GetSeverity ()) << llvm::raw_fd_ostream::WHITE << '[';
    llvm::errs().changeColor (SeverityToColor (diag.GetSeverity()), true);
    std::string errCode = std::format ("{:04}", DiagCodeToIntegerCode(diag.GetCode()));
    llvm::errs() << SeverityToPrefix (diag.GetSeverity ()) << errCode << llvm::raw_fd_ostream::WHITE << "]: " << diag.GetMessage () << '\n';


    auto lineAndCol = _mgr->getLineAndColumn (diag.GetSpans()[0].Span.Start, buffer);
    llvm::errs() << "  --> " << bufferId << ':' << lineAndCol.first << ':' << lineAndCol.second << '\n';
}

void
DiagnosticEngine::renderDiag (DiagnosticBuilder &diag) {
    printDiagnosticHeader(diag);
}

}
