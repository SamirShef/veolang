#pragma once
#include <ast/expr.h>
#include <ast/stmt.h>

namespace veo::ast {

class ExprStmt : public Stmt {
    Expr *_expr;

public:
    explicit ExprStmt (Expr *expr)
        : _expr (expr), Stmt (NodeKind::ExprStmt, expr->Start (), expr->End ()) {}

    ast_classof (ExprStmt);

    Expr *
    GetExpr () const {
        return _expr;
    }
};

}
