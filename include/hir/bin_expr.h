#pragma once
#include <ast/exprs/bin_expr.h>
#include <basic/types/type.h>
#include <hir/node.h>

namespace veo::hir {

class BinaryExpr : public Node {
    ast::BinOp   _op;
    basic::Type *_commonType;
    Node        *_lhs;
    Node        *_rhs;

public:
    BinaryExpr (
        ast::BinOp   op,
        basic::Type *commonType,
        Node        *lhs,
        Node        *rhs,
        llvm::SMLoc  start,
        llvm::SMLoc  end)
        : _op (op),
          _commonType (commonType),
          _lhs (lhs),
          _rhs (rhs),
          Node (NodeKind::BinExpr, start, end) {}

    hir_classof (BinExpr);

    ast::BinOp
    Op () const {
        return _op;
    }

    basic::Type *
    CommonType () const {
        return _commonType;
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
