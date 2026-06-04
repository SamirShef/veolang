#pragma once
#include <hir_analyze/cfg_bulder.h>

namespace veo {

class DeadCodeEliminator {
public:
    static void
    RunOnFunction (hir::Function *func);
};

}
