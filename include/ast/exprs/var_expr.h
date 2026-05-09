#pragma once
#include <ast/expr.h>
#include <string>

namespace veo::ast {

class VarExpr : public Expr {
    std::string _name;

public:
    VarExpr (std::string name, llvm::SMLoc start, llvm::SMLoc end)
        : _name (std::move (name)), Expr (NodeKind::VarExpr, start, end) {}

    const std::string &
    Name () const {
        return _name;
    }
};

}
