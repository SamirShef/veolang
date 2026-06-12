#include <hir_analyze/return_checker.h>

namespace veo {

using namespace diagnostic;

void
ReturnChecker::RunOnFunction (hir::Function *func) {
    if (func->Body ().empty ()) {
        return;
    }
    if (func->RetType () == nullptr || func->RetType ()->IsNoth ()) {
        return;
    }

    for (auto *bb : func->Body ()) {
        if (bb->Terminator () == nullptr) {
            _diag
                .Report (
                    DiagCode::ENotAllControlPathsRetValue,
                    "not all control paths return a value",
                    Severity::Error)
                .AddSpan (func->Start (), func->End ());
            break;
        }
    }
}

}
