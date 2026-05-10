#pragma once
#include <ast/exprs/un_expr.h>
#include <hir/node.h>

namespace veo::hir {

class UnaryExpr : public Node {
    ast::UnOp _op;
    Node     *_rhs;

public:
    UnaryExpr (ast::UnOp op, Node *rhs, llvm::SMLoc start, llvm::SMLoc end)
        : _op (op), _rhs (rhs), Node (NodeKind::UnExpr, start, end) {}

    hir_classof (UnExpr);

    ast::UnOp
    Op () const {
        return _op;
    }

    Node *
    Rhs () const {
        return _rhs;
    }
};

}
