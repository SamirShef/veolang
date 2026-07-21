#pragma once
#include <hir/node.h>

namespace veo::hir {

class ExprStmt : public Node {
    Node *_expr;

public:
    ExprStmt (Node *expr, llvm::SMLoc start, llvm::SMLoc end)
        : _expr (expr), Node (NodeKind::ExprStmt, start, end) {}

    hir_classof (ExprStmt);

    Node *
    Expr () const {
        return _expr;
    }
};

}
