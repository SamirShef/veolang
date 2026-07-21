#pragma once
#include <ast/expr.h>

namespace veo::ast {

class RefExpr : public Expr {
    Expr *_expr;

public:
    RefExpr (Expr *expr, llvm::SMLoc start, llvm::SMLoc end)
        : _expr (expr), Expr (NodeKind::RefExpr, start, end) {}

    ast_classof (RefExpr);

    Expr *
    GetExpr () const {
        return _expr;
    }
};

}
