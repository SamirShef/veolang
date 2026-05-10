#pragma once
#include <ast/expr.h>
#include <string>

namespace veo::ast {

class LiteralExpr : public Expr {
    std::string _val;

public:
    LiteralExpr (std::string val, llvm::SMLoc start, llvm::SMLoc end)
        : _val (std::move (val)), Expr (NodeKind::LitExpr, start, end) {}

    ast_classof (LitExpr);

    std::string
    Value () const {
        return _val;
    }
};

}
