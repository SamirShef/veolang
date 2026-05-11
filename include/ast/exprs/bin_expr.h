#pragma once
#include <ast/expr.h>
#include <cstdint>
#include <lexer/token_kind.h>

namespace veo::ast {

enum class BinOp : uint8_t {
    Plus,
    Minus,
    Mul,
    Div,
    Rem,
    Eq,
    NEq,
    Lt,
    LtEq,
    Gt,
    GtEq,
    LogAnd,
    LogOr,
    Invalid
};

inline BinOp
TokToBinOp (TokenKind kind) {
#define bin(kind) BinOp::kind
#define variant(tok, op)                                                                 \
    case TokenKind::tok: return bin (op);
    switch (kind) {
        variant (Plus, Plus);
        variant (Minus, Minus);
        variant (Star, Mul);
        variant (Slash, Div);
        variant (Percent, Rem);
        variant (EqEq, Eq);
        variant (BangEq, NEq);
        variant (Lt, Lt);
        variant (LtEq, LtEq);
        variant (Gt, Gt);
        variant (GtEq, GtEq);
        variant (LogAnd, LogAnd);
        variant (LogOr, LogOr);
    default: return bin (Invalid);
    }
#undef variant
#undef bin
}

inline const char *
BinOpToString (BinOp op) {
#define variant(kind, res)                                                               \
    case BinOp::kind: return res;
    switch (op) {
        variant (Plus, "+");
        variant (Minus, "-");
        variant (Mul, "*");
        variant (Div, "/");
        variant (Rem, "%");
        variant (Eq, "==");
        variant (NEq, "!=");
        variant (Lt, "<");
        variant (LtEq, "<=");
        variant (Gt, ">");
        variant (GtEq, ">=");
        variant (LogAnd, "&&");
        variant (LogOr, "||");
        variant (Invalid, "<invalid>");
    }
#undef variant
    return ""; // to shut up the fucking warning
}

class BinaryExpr : public Expr {
    BinOp _op;
    Expr *_lhs;
    Expr *_rhs;

public:
    BinaryExpr (BinOp op, Expr *lhs, Expr *rhs, llvm::SMLoc start, llvm::SMLoc end)
        : _op (op), _lhs (lhs), _rhs (rhs), Expr (NodeKind::BinExpr, start, end) {}

    ast_classof (BinExpr);

    BinOp
    Op () const {
        return _op;
    }

    Expr *
    Lhs () const {
        return _lhs;
    }

    Expr *
    Rhs () const {
        return _rhs;
    }
};

}
