#include <algorithm>
#include <hir_analyze/dead_code_eliminator.h>
#include <unordered_set>

namespace veo {

void
DeadCodeEliminator::RunOnFunction (hir::Function *func) {
    CFGBuilder::Build (func);

    std::unordered_set<hir::BasicBlock *> reachable;
    std::vector<hir::BasicBlock *>        worklist;

    auto *entry = func->Body ().front ();
    worklist.push_back (entry);
    reachable.insert (entry);

    while (!worklist.empty ()) {
        auto *cur = worklist.back ();
        worklist.pop_back ();

        for (auto *succ : cur->Succs ()) {
            if (!reachable.contains (succ)) {
                reachable.insert (succ);
                worklist.push_back (succ);
            }
        }
    }

    auto &blocks = func->Body ();
    auto  it     = std::remove_if ( // NOLINT
        blocks.begin (),
        blocks.end (),
        [&] (hir::BasicBlock *bb) {
            return !reachable.contains (bb);
        });
    bool mutated = it != blocks.end ();
    blocks.erase (it, blocks.end ());

    if (mutated) {
        CFGBuilder::Build (func);
    }
}

}
