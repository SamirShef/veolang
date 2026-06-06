#include <hir/branch.h>
#include <hir_analyze/cfg_bulder.h>

namespace veo {

void
CFGBuilder::Build (hir::Function *func) {
    if (func->IsDeclaration ()) {
        return;
    }

    for (auto *bb : func->Body ()) {
        bb->ClearLinks ();
    }

    for (auto *bb : func->Body ()) {
        auto *term = bb->Terminator ();
        if (term == nullptr) {
            continue;
        }

        if (term->Kind () == hir::NodeKind::Branch) {
            auto *br = llvm::cast<hir::Branch> (term);
            if (br->Then () != nullptr) {
                link (bb, br->Then ());
            }
            if (br->Else () != nullptr) {
                link (bb, br->Else ());
            }
        }
    }
}

void
CFGBuilder::link (hir::BasicBlock *from, hir::BasicBlock *to) {
    if (from == nullptr || to == nullptr) {
        return;
    }
    from->AddSuccessor (to);
    to->AddPredecessor (from);
}

}
