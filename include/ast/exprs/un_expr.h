#pragma once
#include <ast/expr.h>
#include <cstdint>
#include <lexer/token_kind.h>

namespace veo::ast {

enum class UnOp : uint8_t { Minus, Not, Invalid };

inline UnOp
TokToUnOp (TokenKind kind) {
#define un(kind) UnOp::kind
#define variant(tok, op)                                                                 \
    case TokenKind::tok: return un (op);
    switch (kind) {
        variant (Minus, Minus);
        variant (Bang, Not);
    default: return un (Invalid);
    }
#undef variant
#undef un
}

inline const char *
UnOpToString (UnOp op) {
#define variant(kind, res)                                                               \
    case UnOp::kind: return res;
    switch (op) {
        variant (Minus, "-");
        variant (Not, "!");
        variant (Invalid, "<invalid>");
    }
#undef variant
    return ""; // to shut up the fucking warning
}

class UnaryExpr : public Expr {
    UnOp  _op;
    Expr *_rhs;

public:
    UnaryExpr (UnOp op, Expr *rhs, llvm::SMLoc start, llvm::SMLoc end)
        : _op (op), _rhs (rhs), Expr (NodeKind::UnExpr, start, end) {}

    ast_classof (UnExpr);

    UnOp
    Op () const {
        return _op;
    }

    Expr *
    Rhs () const {
        return _rhs;
    }
};

}
