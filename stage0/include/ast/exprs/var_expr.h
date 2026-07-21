#pragma once
#include <ast/expr.h>
#include <basic/name.h>

namespace veo::ast {

class VarExpr : public Expr {
    basic::NameObj _name;

public:
    VarExpr (basic::NameObj name, llvm::SMLoc start, llvm::SMLoc end)
        : _name (std::move (name)), Expr (NodeKind::VarExpr, start, end) {}

    ast_classof (VarExpr);

    basic::NameObj
    Name () const {
        return _name;
    }
};

}
