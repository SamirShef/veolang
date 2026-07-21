#include <basic/types/integer.h>
#include <sema/sema.h>

namespace veo {

OptValue
Sema::foldBinary (
    ast::BinOp   op,
    const Value &lhs,
    const Value &rhs,
    Type        *resType,
    llvm::SMLoc  start,
    llvm::SMLoc  end) {
    return std::visit (
        [&] (auto &&l) -> OptValue {
            return std::visit (
                [&] (auto &&r) -> OptValue {
                    using LType = std::decay_t<decltype (l)>;
                    using RType = std::decay_t<decltype (r)>;

                    if constexpr (std::is_arithmetic_v<LType>
                                  && std::is_arithmetic_v<RType>) {
                        if ((op == ast::BinOp::Div || op == ast::BinOp::Rem) && r == 0) {
                            _diag
                                .Report (
                                    DiagCode::EDivByZero,
                                    "division by zero",
                                    Severity::Error)
                                .AddSpan (start, end);
                            return std::nullopt;
                        }

                        ValueData resData;
                        switch (op) {
#define variant(kind, op)                                                                \
    case ast::BinOp::kind: resData = l op r; break;
                            variant (Plus, +);
                            variant (Minus, -);
                            variant (Mul, *);
                            variant (Div, /);
                            variant (Eq, ==);
                            variant (NEq, !=);
                            variant (Lt, <);
                            variant (LtEq, <=);
                            variant (Gt, >);
                            variant (GtEq, >=);
                        default: return std::nullopt;
#undef variant
                        }
                        return Value (ValueKind::Const, resData, resType);
                    }
                    return Value (ValueKind::Const, resType);
                },
                rhs.Data);
        },
        lhs.Data);
}

int64_t
Sema::foldBinaryBitwise (ValueData lhs, ValueData rhs, Type *commonType, ast::BinOp op) {
    auto    lVal       = std::get<0> (lhs);
    auto    rVal       = std::get<0> (rhs);
    bool    isUnsigned = commonType->AsInteger ()->IsUnsigned ();
    int64_t res        = 0;
    switch (op) {
        // NOLINTBEGIN(bugprone-macro-parentheses)
#define variant(kind, _op)                                                               \
    case ast::BinOp::kind:                                                               \
        res = isUnsigned ? static_cast<int64_t> (static_cast<uint64_t> (lVal)            \
                                                     _op static_cast<uint64_t> (rVal))   \
                         : static_cast<uint64_t> (lVal)                                  \
                               _op static_cast<uint64_t> (rVal);                         \
        break;
        // NOLINTEND(bugprone-macro-parentheses)

        variant (BitAnd, &);
        variant (BitOr, |);
        variant (BitXor, ^);
    default: {
    }

#undef variant
    }
    return res;
}

OptValue
Sema::foldUnary (
    ast::UnOp op, const Value &rhs, Type *resType, llvm::SMLoc start, llvm::SMLoc end) {
    return std::visit (
        [&] (auto &&r) -> OptValue {
            using T = std::decay_t<decltype (r)>;
            ValueData resData;

            if constexpr (std::is_arithmetic_v<T>) {
                switch (op) {
                case ast::UnOp::Minus: resData = -r; break;
                case ast::UnOp::Not: {
                    if constexpr (std::is_same_v<T, int64_t>) {
                        resData = !r;
                    } else {
                        _diag
                            .Report (
                                DiagCode::ECannotApplyOp,
                                "cannot apply operator '!' with type '"
                                    + typeToString (rhs.Type) + "'",
                                Severity::Error)
                            .AddSpan (start, end);
                        return std::nullopt;
                    }
                    break;
                }
                default: return std::nullopt;
                }
            }

            return Value (ValueKind::Const, resData, resType);
        },
        rhs.Data);
}

}
