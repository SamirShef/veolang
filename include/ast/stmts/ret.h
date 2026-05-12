#pragma once
#include <ast/expr.h>
#include <ast/stmt.h>

namespace veo::ast {

class Return : public Stmt {
    Expr *_expr;

public:
    Return (Expr *expr, llvm::SMLoc start, llvm::SMLoc end)
        : _expr (expr), Stmt (NodeKind::Ret, start, end) {}

    Expr *
    RetExpr () const {
        return _expr;
    }
};

}
