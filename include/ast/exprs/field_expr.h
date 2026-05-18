#pragma once
#include <ast/expr.h>
#include <basic/name.h>

namespace veo::ast {

class FieldExpr : public Expr {
    Expr          *_base;
    basic::NameObj _name;

public:
    FieldExpr (Expr *base, basic::NameObj name, llvm::SMLoc start, llvm::SMLoc end)
        : _base (base),
          _name (std::move (name)),
          Expr (NodeKind::FieldExpr, start, end) {}

    ast_classof (FieldExpr);

    Expr *
    Base () const {
        return _base;
    }

    basic::NameObj
    Name () const {
        return _name;
    }
};

}
