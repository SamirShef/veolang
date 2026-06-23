#pragma once
#include <ast/exprs/un_expr.h>
#include <basic/types/type.h>
#include <hir/node.h>

namespace veo::hir {

class UnaryExpr : public Node {
    ast::UnOp    _op;
    Node        *_rhs;
    basic::Type *_commonType;

public:
    UnaryExpr (
        ast::UnOp    op,
        basic::Type *commonType,
        Node        *rhs,
        llvm::SMLoc  start,
        llvm::SMLoc  end)
        : _op (op),
          _commonType (commonType),
          _rhs (rhs),
          Node (NodeKind::UnExpr, start, end) {}

    hir_classof (UnExpr);

    ast::UnOp
    Op () const {
        return _op;
    }

    basic::Type *
    CommonType () const {
        return _commonType;
    }

    Node *
    Rhs () const {
        return _rhs;
    }
};

}
