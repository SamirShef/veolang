#pragma once
#include <ast/expr.h>

namespace veo::ast {

class SizeofExpr : public Expr {
    Expr *_expr;

public:
    SizeofExpr (Expr *expr, llvm::SMLoc start, llvm::SMLoc end)
        : _expr (expr), Expr (NodeKind::SizeofExpr, start, end) {}

    ast_classof (SizeofExpr);

    Expr *
    GetExpr () const {
        return _expr;
    }
};

}
