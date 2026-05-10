#pragma once
#include <ast/exprs/bin_expr.h>
#include <hir/node.h>

namespace veo::hir {

class BinaryExpr : public Node {
    ast::BinOp _op;
    Node      *_lhs;
    Node      *_rhs;

public:
    BinaryExpr (ast::BinOp op, Node *lhs, Node *rhs, llvm::SMLoc start, llvm::SMLoc end)
        : _op (op), _lhs (lhs), _rhs (rhs), Node (NodeKind::BinExpr, start, end) {}

    hir_classof (BinExpr);

    ast::BinOp
    Op () const {
        return _op;
    }

    Node *
    Lhs () const {
        return _lhs;
    }

    Node *
    Rhs () const {
        return _rhs;
    }
};

}
