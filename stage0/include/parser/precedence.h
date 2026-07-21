#pragma once
#include <cstdint>
#include <lexer/token_kind.h>

namespace veo {

enum class Precedence : uint8_t {
    Lowest,
    Assignment,
    Ternary,
    LogicalOr,
    LogicalAnd,
    BiwiseOr,
    BitwiseXor,
    BitwiseAnd,
    Equality,
    Comparison,
    Sum,
    Product,
    Unary,
    Member,
};

inline int
GetTokPrecedence (TokenKind kind) {
#define tok(kind) case TokenKind::kind:
#define prec(kind) return (int) Precedence::kind;
    switch (kind) {
        tok (Dot) prec (Member);
        tok (Eq) tok (PlusEq) tok (MinusEq) tok (StarEq) tok (SlashEq) tok (PercentEq)
            tok (AndEq) tok (OrEq) tok (CarretEq) prec (Assignment);
        tok (Question) prec (Ternary);
        tok (LogAnd) prec (LogicalAnd);
        tok (LogOr) prec (LogicalOr);
        tok (Gt) tok (GtEq) tok (Lt) tok (LtEq) prec (Comparison);
        tok (EqEq) tok (BangEq) prec (Equality);
        tok (BitAnd) prec (BitwiseAnd);
        tok (Carret) prec (BitwiseXor);
        tok (BitOr) prec (BiwiseOr);
        tok (Plus) tok (Minus) prec (Sum);
        tok (Star) tok (Slash) tok (Percent) prec (Product);
    default: prec (Lowest);
    }
#undef prec
#undef tok
}

}
