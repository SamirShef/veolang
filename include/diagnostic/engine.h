#pragma once
#include <diagnostic/builder.h>
#include <diagnostic/codes.h>
#include <llvm/Support/SourceMgr.h>

namespace veo::diagnostic {

class DiagnosticEngine {
    llvm::SourceMgr               *_mgr;
    std::vector<DiagnosticBuilder> _builders;
    bool                           _hasErrs{};

public:
    explicit DiagnosticEngine (llvm::SourceMgr &mgr) : _mgr (&mgr) {
    }

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
        for (DiagnosticBuilder &diag : _builders) {
            renderDiag (diag);
        }
    }

private:
    void
    printDiagnosticHeader (DiagnosticBuilder &diag);

    void
    renderDiag (DiagnosticBuilder &diag);
};

}
