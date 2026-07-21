#pragma once
#include <ast/expr.h>

namespace veo::ast {

class TernaryExpr : public Expr {
    Expr *_cond;
    Expr *_trueVal;
    Expr *_falseVal;

public:
    TernaryExpr (
        Expr *cond, Expr *trueVal, Expr *falseVal, llvm::SMLoc start, llvm::SMLoc end)
        : _cond (cond),
          _trueVal (trueVal),
          _falseVal (falseVal),
          Expr (NodeKind::TernaryExpr, start, end) {}

    ast_classof (TernaryExpr);

    Expr *
    Cond () const {
        return _cond;
    }

    Expr *
    TrueVal () const {
        return _trueVal;
    }

    Expr *
    FalseVal () const {
        return _falseVal;
    }
};

}
