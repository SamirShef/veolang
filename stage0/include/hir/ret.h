#pragma once
#include <hir/node.h>

namespace veo::hir {

class Return : public Node {
    Node *_expr;

public:
    Return (Node *expr, llvm::SMLoc start, llvm::SMLoc end)
        : _expr (expr), Node (NodeKind::Ret, start, end) {}

    hir_classof (Ret);

    Node *
    Expr () const {
        return _expr;
    }
};

}
