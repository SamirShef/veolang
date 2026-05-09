#pragma once
#include <cstdint>
#include <lexer/token_kind.h>

namespace veo {

enum class Precedence : uint8_t {
    Unary,
    Logical,
    Comparation,
    Equality,
    BiwiseAnd,
    BiwiseXor,
    BiwiseOr,
    Sum,
    Product,
};

inline int
GetTokPrecedence (TokenKind kind) {
#define tok(kind) case TokenKind::kind:
#define prec(kind) return (int) Precedence::kind;
    switch (kind) {
        tok (LogAnd) tok (LogOr) prec (Logical);
        tok (Gt) tok (GtEq) tok (Lt) tok (LtEq) prec (Comparation);
        tok (EqEq) tok (BangEq) prec (Equality);
        tok (BitAnd) prec (BiwiseAnd);
        tok (Carret) prec (BiwiseXor);
        tok (BitOr) prec (BiwiseOr);
        tok (Plus) tok (Minus) prec (Sum);
        tok (Star) tok (Slash) tok (Percent) prec (Product);
    default: prec (Unary);
    }
#undef prec
#undef tok
}

}
