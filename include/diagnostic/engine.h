#pragma once
#include <diagnostic/builder.h>
#include <diagnostic/codes.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_ostream.h>

namespace veo::diagnostic {

class DiagnosticEngine {
    llvm::SourceMgr               *_mgr;
    std::vector<DiagnosticBuilder> _builders;
    bool                           _hasErrs{};

public:
    explicit DiagnosticEngine (llvm::SourceMgr &mgr) : _mgr (&mgr) {}

    DiagnosticBuilder &
    Report (DiagCode code, std::string message, Severity severity) {
        _builders.emplace_back (code, std::move (message), severity);
        if (severity == Severity::Error) {
            _hasErrs = true;
        }
        return _builders.back ();
    }

    bool
    HasErrors () const {
        return _hasErrs;
    }

    void
    Render () {
        int i = 0;
        for (DiagnosticBuilder &diag : _builders) {
            renderDiag (diag);
            if (i != 0) {
                llvm::errs () << '\n';
            }
            ++i;
        }
    }

private:
    void
    renderDiag (DiagnosticBuilder &diag);

    void
    printDiagnosticHeader (DiagnosticBuilder &diag);

    void
    printDiagnosticBody (DiagnosticBuilder &diag);
};

}
