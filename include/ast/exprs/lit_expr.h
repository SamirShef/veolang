#pragma once
#include <ast/expr.h>
#include <lexer/token_kind.h>
#include <string>

namespace veo::ast {

class LiteralExpr : public Expr {
    std::string _val;
    TokenKind   _kind;

public:
    LiteralExpr (std::string val, TokenKind kind, llvm::SMLoc start, llvm::SMLoc end)
        : _val (std::move (val)), _kind (kind), Expr (NodeKind::LitExpr, start, end) {}

    ast_classof (LitExpr);

    const std::string &
    Value () {
        return _val;
    }

    TokenKind
    Kind () const {
        return _kind;
    }
};

}
