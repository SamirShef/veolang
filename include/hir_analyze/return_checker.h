#pragma once
#include <diagnostic/engine.h>
#include <hir/func.h>

namespace veo {

class ReturnChecker {
    diagnostic::DiagnosticEngine &_diag; // NOLINT

public:
    explicit ReturnChecker (diagnostic::DiagnosticEngine &diag) : _diag (diag) {}

    void
    RunOnFunction (hir::Function *func);
};

}
