#pragma once
#include <hir/node.h>

namespace veo::hir {

class RefExpr : public Node {
    Node *_expr;

public:
    RefExpr (Node *expr, llvm::SMLoc start, llvm::SMLoc end)
        : _expr (expr), Node (NodeKind::RefExpr, start, end) {}

    hir_classof (RefExpr);

    Node *
    Expr () const {
        return _expr;
    }
};

}
