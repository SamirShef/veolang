#pragma once
#include <ast/expr.h>

namespace veo::ast {

class DerefExpr : public Expr {
    Expr *_expr;

public:
    DerefExpr (Expr *expr, llvm::SMLoc start, llvm::SMLoc end)
        : _expr (expr), Expr (NodeKind::DerefExpr, start, end) {}

    ast_classof (DerefExpr);

    Expr *
    GetExpr () const {
        return _expr;
    }
};

}
