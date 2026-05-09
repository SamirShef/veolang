#pragma once
#include <ast/expr.h>
#include <string_view>

namespace veo::ast {

class LiteralExpr : public Expr {
    std::string_view _val;

public:
    LiteralExpr (std::string_view val, llvm::SMLoc start, llvm::SMLoc end)
        : _val (val), Expr (NodeKind::LitExpr, start, end) {}

    std::string_view
    Value () const {
        return _val;
    }
};

}
