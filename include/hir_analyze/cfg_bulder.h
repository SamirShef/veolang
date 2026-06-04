#pragma once
#include <hir/func.h>

namespace veo {

class CFGBuilder {
public:
    static void
    Build (hir::Function *func);

private:
    static void
    link (hir::BasicBlock *from, hir::BasicBlock *to);
};

}
