#pragma once
#include <ast/expr.h>
#include <lexer/token_kind.h>

namespace veo::ast {

enum class AsgnOp : uint8_t { Eq, PlusEq, MinusEq, MulEq, DivEq, RemEq, Invalid };

inline AsgnOp
TokToAsgnOp (TokenKind kind) {
#define variant(tok, asgn)                                                               \
    case TokenKind::tok: return AsgnOp::asgn;
    switch (kind) {
        variant (Eq, Eq);
        variant (PlusEq, PlusEq);
        variant (MinusEq, MinusEq);
        variant (StarEq, MulEq);
        variant (SlashEq, DivEq);
        variant (PercentEq, RemEq);
    default: return AsgnOp::Invalid;
    }
#undef variant
}

inline const char *
AsgnOpToString (AsgnOp op) {
#define variant(kind, res)                                                               \
    case AsgnOp::kind: return res;
    switch (op) {
        variant (Eq, "=");
        variant (PlusEq, "+=");
        variant (MinusEq, "-=");
        variant (MulEq, "*=");
        variant (DivEq, "/=");
        variant (RemEq, "%=");
        variant (Invalid, "<invalid>");
    }
#undef variant
    return ""; // to shut up the fucking warning
}

class AsgnExpr : public Expr {
    AsgnOp _op;
    Expr  *_ptr;
    Expr  *_expr;

public:
    AsgnExpr (AsgnOp op, Expr *ptr, Expr *expr, llvm::SMLoc start, llvm::SMLoc end)
        : _op (op), _ptr (ptr), _expr (expr), Expr (NodeKind::AsgnExpr, start, end) {}

    ast_classof (AsgnExpr);

    AsgnOp
    Op () const {
        return _op;
    }

    Expr *
    Ptr () const {
        return _ptr;
    }

    Expr *
    Init () const {
        return _expr;
    }
};

}
