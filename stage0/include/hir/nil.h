#pragma once
#include <hir/node.h>

namespace veo::hir {

class NilExpr : public Node {
public:
    NilExpr (llvm::SMLoc start, llvm::SMLoc end) : Node (NodeKind::NilExpr, start, end) {}

    hir_classof (NilExpr);
};

}
