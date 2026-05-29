#pragma once
#include <ast/expr.h>
#include <basic/types/type.h>

namespace veo::ast {

class CastExpr : public Expr {
    basic::Type *_type;
    Expr        *_expr;

public:
    CastExpr (basic::Type *type, Expr *expr, llvm::SMLoc start, llvm::SMLoc end)
        : _type (type), _expr (expr), Expr (NodeKind::CastExpr, start, end) {}

    ast_classof (CastExpr);

    basic::Type *
    Type () const {
        return _type;
    }

    Expr *
    GetExpr () const {
        return _expr;
    }
};

}
