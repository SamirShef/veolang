#include "basic/value.h"
#include "lexer/token_kind.h"

#include <basic/types/all.h>
#include <cstdint>
#include <diagnostic/codes.h>
#include <sema/sema.h>

namespace veo {

void
Sema::analyzeStmt (Stmt *stmt) {
    switch (stmt->Kind ()) {
    case NodeKind::VarDef: analyzeVarDef (llvm::cast<VarDef> (stmt)); break;
    case NodeKind::FuncDef: analyzeFuncDef (llvm::cast<FuncDef> (stmt)); break;
    default: break;
    }
}

void
Sema::analyzeVarDef (VarDef *vd) {
    if (auto var = getVariable (vd->Name ().Val); var.has_value ()) {
        _diag
            .Report (
                DiagCode::ERedefinition,
                "variable '" + vd->Name ().Val + "' is already defined",
                Severity::Error)
            .AddSpan (
                var->Name.Start,
                var->Name.End,
                "previous definition was here",
                false)
            .AddSpan (vd->Name ().Start, vd->Name ().End, "redefined here");
        return;
    }
    bool  isGlobal = _vars.size () == 1;
    Type *type     = vd->Type ();
    resolveType (&vd->Type ());
    auto val = analyzeExpr (vd->Init (), vd->Type ());
    if (type == nullptr) {
        type = val.Val->Type;
    }
    auto var = Variable (
        vd->Name (),
        type,
        vd->IsConst (),
        isGlobal ? _vars.top ().Vars.size () : _localsCount++,
        val.Val);
    _vars.top ().Vars.emplace (vd->Name ().Val, var);
    if (isGlobal) {
        _mod->Vars.emplace (vd->Name ().Val, var);
    }
    _builder.CreateVariable (
        vd->Name (),
        vd->Type (),
        val.Node,
        vd->IsConst (),
        isGlobal,
        vd->Start (),
        vd->End ());
}

void
Sema::analyzeFuncDef (FuncDef *fd) {}

Sema::SemanticResult
Sema::analyzeExpr (Expr *expr, Type *expectedType) {
    if (expr == nullptr) {
        return {};
    }
    switch (expr->Kind ()) {
    case NodeKind::LitExpr:
        return analyzeLiteralExpr (llvm::cast<LiteralExpr> (expr), expectedType);
    case NodeKind::BinExpr:
        return analyzeBinaryExpr (llvm::cast<BinaryExpr> (expr), expectedType);
    case NodeKind::UnExpr:
        return analyzeUnaryExpr (llvm::cast<UnaryExpr> (expr), expectedType);
    case NodeKind::VarExpr:
        return analyzeVarExpr (llvm::cast<VarExpr> (expr), expectedType);
    default: return {};
    }
}

// NOLINTBEGIN(readability-function-cognitive-complexity)
Sema::SemanticResult
Sema::analyzeLiteralExpr (LiteralExpr *le, Type *expectedType) {
    const std::string &val = le->Value ();
    switch (le->Kind ()) {
#define int_lit(kind, bitWidth, isUnsigned, min, max)                                    \
    case TokenKind::kind##Lit: {                                                         \
        int64_t ival  = std::stoll (val);                                                \
        auto    value = Value (                                                          \
            ValueKind::Const,                                                            \
            ValueData (ival),                                                            \
            new IntegerType (bitWidth, isUnsigned));                                     \
        auto res = SemanticResult (                                                      \
            value,                                                                       \
            _builder.CreateLiteral (value, le->Start (), le->End ()));                   \
        if (ival < (min) || ival > (max)) {                                              \
            _diag                                                                        \
                .Report (                                                                \
                    DiagCode::ELitOutOfRange,                                            \
                    "literal out of range for '" + res.Val->Type->ToString () + "'",     \
                    Severity::Error)                                                     \
                .AddSpan (                                                               \
                    le->Start (),                                                        \
                    le->End (),                                                          \
                    "value must be in range [" + std::to_string (min) + ", "             \
                        + std::to_string (max) + "]");                                   \
        }                                                                                \
        if (expectedType == nullptr) {                                                   \
            return res;                                                                  \
        }                                                                                \
        res = implicitlyCast (res, &expectedType, le->Start (), le->End ());             \
        return res;                                                                      \
    }

#define float_lit(kind, floatingKind)                                                    \
    case TokenKind::kind##Lit: {                                                         \
        double fval  = std::stold (val);                                                 \
        auto   value = Value (                                                           \
            ValueKind::Const,                                                            \
            ValueData (fval),                                                            \
            new FloatingType (FloatingKind::floatingKind));                              \
        auto res = SemanticResult (                                                      \
            value,                                                                       \
            _builder.CreateLiteral (value, le->Start (), le->End ()));                   \
        if (expectedType == nullptr) {                                                   \
            return res;                                                                  \
        }                                                                                \
        res = implicitlyCast (res, &expectedType, le->Start (), le->End ());             \
        return res;                                                                      \
    }

        int_lit (U8, 8, true, 0, UINT8_MAX);
        int_lit (U16, 16, true, 0, UINT16_MAX);
        int_lit (U32, 32, true, 0, UINT32_MAX);
        int_lit (U64, 64, true, 0, UINT64_MAX);

        int_lit (I8, 8, false, INT8_MIN, INT8_MAX);
        int_lit (I16, 16, false, INT16_MIN, INT16_MAX);
        int_lit (I32, 32, false, INT32_MIN, INT32_MAX);
        int_lit (I64, 64, false, INT64_MIN, INT64_MAX);

        float_lit (F32, Float);
        float_lit (F64, Double);
    case TokenKind::IntLit: {
        if (expectedType == nullptr) {
            // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
            expectedType = new IntegerType (32);
        }
        if (!expectedType->IsInteger ()) {
            _diag
                .Report (
                    DiagCode::ECannotImplCast,
                    "cannot implicitly cast 'i32' to '" + expectedType->ToString () + "'",
                    Severity::Error)
                .AddSpan (le->Start (), le->End ());
            return {};
        }
        int64_t ival  = std::stoll (val);
        auto    value = Value (ValueKind::Const, ValueData (ival), expectedType);
        if (!canFit (value, expectedType)) {
            const auto *it  = expectedType->AsInteger ();
            int64_t     min = it->IsUnsigned ()
                                  ? 0
                                  : -static_cast<int64_t> (1ULL << (it->BitWidth () - 1));
            auto max = it->IsUnsigned ()
                           ? static_cast<int64_t> (1ULL << it->BitWidth ()) - 1
                           : static_cast<int64_t> (1ULL << (it->BitWidth () - 1)) - 1;
            _diag
                .Report (
                    DiagCode::ELitOutOfRange,
                    "literal out of range for '" + value.Type->ToString () + "'",
                    Severity::Error)
                .AddSpan (
                    le->Start (),
                    le->End (),
                    "value must be in range [" + std::to_string (min) + ", "
                        + std::to_string (max) + "]");
            return {};
        }
        auto res = SemanticResult (
            value,
            _builder.CreateLiteral (value, le->Start (), le->End ()));
        res = implicitlyCast (res, &expectedType, le->Start (), le->End ());
        return res;
    }
    case TokenKind::BoolLit: {
        bool bval  = (val == "true");
        auto value = Value (ValueKind::Const, ValueData (bval), new BoolType ());
        auto res   = SemanticResult (
            value,
            _builder.CreateLiteral (value, le->Start (), le->End ()));
        if (expectedType == nullptr) {
            return res;
        }
        res = implicitlyCast (res, &expectedType, le->Start (), le->End ());
        return res;
    }
    case TokenKind::CharLit: {
        int64_t cval  = val.size () == 1 ? val[0] : 0;
        auto    value = Value (ValueKind::Const, ValueData (cval), new CharType ());
        auto    res   = SemanticResult (
            value,
            _builder.CreateLiteral (value, le->Start (), le->End ()));
        if (expectedType == nullptr) {
            return res;
        }
        res = implicitlyCast (res, &expectedType, le->Start (), le->End ());
        return res;
    }
    default: return {};
#undef float_lit
#undef int_lit
    }
}
// NOLINTEND(readability-function-cognitive-complexity)

Sema::SemanticResult
Sema::analyzeBinaryExpr (BinaryExpr *be, Type *expectedType) {
    auto lhs = analyzeExpr (be->Lhs (), nullptr);
    auto rhs = analyzeExpr (be->Rhs (), nullptr);
    resolveType (&lhs.Val->Type);
    resolveType (&rhs.Val->Type);
    BinOp op         = be->Op ();
    Type *commonType = nullptr;
    Type *resType    = nullptr;

    if (op >= BinOp::LogAnd && op <= BinOp::LogOr) {
        commonType = new BoolType (); // NOLINT(cppcoreguidelines-owning-memory)
        resType    = commonType;
    } else if (op >= BinOp::Eq && op <= BinOp::GtEq) {
        commonType
            = getCommonType (lhs.Val->Type, rhs.Val->Type, be->Start (), be->End ());
        resType = new BoolType (); // NOLINT(cppcoreguidelines-owning-memory)
    } else {
        commonType
            = getCommonType (lhs.Val->Type, rhs.Val->Type, be->Start (), be->End ());
        resType = commonType;
    }

    if (commonType != nullptr) {
        lhs = implicitlyCast (lhs, &commonType, be->Lhs ()->Start (), be->Lhs ()->End ());
        rhs = implicitlyCast (rhs, &commonType, be->Rhs ()->Start (), be->Rhs ()->End ());
    }

    if (!lhs.Val.has_value () || !rhs.Val.has_value ()) {
        return {};
    }
    if (lhs.Val->Kind == ValueKind::Const && rhs.Val->Kind == ValueKind::Const) {
        auto foldedValue
            = foldBinary (op, *lhs.Val, *rhs.Val, resType, be->Start (), be->End ());
        if (expectedType != nullptr) {
            implicitlyCast (
                { foldedValue, nullptr },
                &expectedType,
                be->Start (),
                be->End ());
        }
        if (foldedValue.has_value ()) {
            return {
                foldedValue,
                _builder.CreateLiteral (foldedValue.value (), be->Start (), be->End ())
            };
        }
    }

    auto *node = _builder.CreateBinary (
        op,
        commonType,
        lhs.Node,
        rhs.Node,
        be->Start (),
        be->End ());

    auto res = SemanticResult (Value (ValueKind::Unknown, resType), node);

    if (expectedType != nullptr) {
        res = implicitlyCast (res, &expectedType, be->Start (), be->End ());
    }

    return res;
}

Sema::SemanticResult
Sema::analyzeUnaryExpr (UnaryExpr *ue, Type *expectedType) {
    auto rhs = analyzeExpr (ue->Rhs (), nullptr);
    if (!rhs.Val.has_value ()) {
        return {};
    }
    resolveType (&rhs.Val->Type);

    UnOp  op      = ue->Op ();
    Type *resType = nullptr;

    switch (op) {
    case UnOp::Minus:
        if (!rhs.Val->Type->IsInteger () && !rhs.Val->Type->IsFloating ()) {
            _diag
                .Report (
                    DiagCode::ECannotApplyOp,
                    "unary operator cannot be applied to type '"
                        + rhs.Val->Type->ToString () + "'",
                    Severity::Error)
                .AddSpan (ue->Start (), ue->End ());
            return {};
        }
        resType = rhs.Val->Type;
        break;

    case UnOp::Not:
        if (!rhs.Val->Type->IsBool ()) {
            Type *boolType = new BoolType (); // NOLINT(cppcoreguidelines-owning-memory)
            rhs            = implicitlyCast (
                rhs,
                &boolType,
                ue->Rhs ()->Start (),
                ue->Rhs ()->End ());
            if (!rhs.Val.has_value ()) {
                return {};
            }
            resType = boolType;
        } else {
            resType = rhs.Val->Type;
        }
        break;

    default: return {};
    }

    if (rhs.Val->Kind == ValueKind::Const) {
        auto foldedValue = foldUnary (op, *rhs.Val, resType, ue->Start (), ue->End ());
        if (expectedType != nullptr) {
            implicitlyCast (
                { foldedValue, nullptr },
                &expectedType,
                ue->Start (),
                ue->End ());
        }
        if (foldedValue.has_value ()) {
            return {
                foldedValue,
                _builder.CreateLiteral (foldedValue.value (), ue->Start (), ue->End ())
            };
        }
    }

    auto *node = _builder.CreateUnary (op, rhs.Node, ue->Start (), ue->End ());
    auto  res  = SemanticResult (Value (ValueKind::Unknown, resType), node);

    if (expectedType != nullptr) {
        res = implicitlyCast (res, &expectedType, ue->Start (), ue->End ());
    }

    return res;
}

// NOLINTBEGIN(readability-convert-member-functions-to-static)
Sema::SemanticResult
Sema::analyzeVarExpr (VarExpr *ve, Type *expectedType) {
    std::optional<Variable> var = getVariable (ve->Name ().Val);
    if (!var.has_value ()) {
        _diag
            .Report (
                DiagCode::EUndefined,
                "variable '" + ve->Name ().Val + "' is not defined",
                Severity::Error)
            .AddSpan (ve->Start (), ve->End ());
        return {};
    }
    auto value = Value (
        var->IsConst ? ValueKind::Const : ValueKind::Unknown,
        var->Val->Data,
        var->Type);
    hir::Node *node = nullptr;
    if (var->IsConst) {
        node = _builder.CreateLiteral (value, ve->Start (), ve->End ());
    } else {
        node = _builder.CreateLoadVar (var->Index, var->Type, ve->Start (), ve->End ());
    }
    auto res = SemanticResult (value, node);
    if (expectedType != nullptr) {
        res = implicitlyCast (res, &expectedType, ve->Start (), ve->End ());
    }
    return res;
}

Type *
Sema::resolveType (Type **type) {
    if (*type == nullptr) {
        return *type;
    }
    // TODO: implement resolving logic (after structs)
    return *type;
}
// NOLINTEND(readability-convert-member-functions-to-static)

std::optional<Variable>
Sema::getVariable (const std::string &name) {
    auto vars = _vars;
    while (!vars.empty ()) {
        if (auto it = vars.top ().Vars.find (name); it != vars.top ().Vars.end ()) {
            return it->second;
        }
        vars.pop ();
    }
    return std::nullopt;
}

// NOLINTBEGIN(readability-function-cognitive-complexity)
Type *
Sema::getCommonType (Type *lhs, Type *rhs, llvm::SMLoc start, llvm::SMLoc end) {
    if (*lhs == *rhs) {
        return lhs;
    }
    if (lhs->IsInteger () && rhs->IsInteger ()) {
        const auto *lhsT = lhs->AsInteger ();
        const auto *rhsT = rhs->AsInteger ();
        if (lhsT->IsUnsigned () && rhsT->IsUnsigned ()
            || !lhsT->IsUnsigned () && !rhsT->IsUnsigned ()) {
            if (lhsT->BitWidth () > rhsT->BitWidth ()) {
                return lhs;
            }
            return rhs;
        }
        if (lhsT->IsUnsigned ()) {
            if (lhsT->BitWidth () >= rhsT->BitWidth ()) {
                return lhs;
            }
            return rhs;
        }
        if (rhsT->IsUnsigned ()) {
            if (rhsT->BitWidth () >= lhsT->BitWidth ()) {
                return rhs;
            }
            return lhs;
        }
    }
    if (lhs->IsFloating () && rhs->IsFloating ()) {
        const auto *lhsT = lhs->AsFloating ();
        const auto *rhsT = rhs->AsFloating ();
        if (lhsT->IsDouble ()) {
            return lhs;
        }
        return rhs;
    }
    if (lhs->IsFloating () && rhs->IsInteger ()) {
        const auto *lhsT = lhs->AsFloating ();
        const auto *rhsT = rhs->AsInteger ();
        if (lhsT->IsFloat () && rhsT->BitWidth () >= 32
            || lhsT->IsDouble () && rhsT->BitWidth () >= 64) {
            _diag
                .Report (
                    DiagCode::WLossPrecision,
                    "casting types can lead to loss of precision",
                    Severity::Warning)
                .AddSpan (start, end);
        }
        return lhs;
    }
    if (lhs->IsInteger () && rhs->IsFloating ()) {
        const auto *lhsT = lhs->AsInteger ();
        const auto *rhsT = rhs->AsFloating ();
        if (rhsT->IsFloat () && lhsT->BitWidth () >= 32
            || rhsT->IsDouble () && lhsT->BitWidth () >= 64) {
            _diag
                .Report (
                    DiagCode::WLossPrecision,
                    "casting types can lead to loss of precision",
                    Severity::Warning)
                .AddSpan (start, end);
        }
        return rhs;
    }
    _diag
        .Report (
            DiagCode::ECannotFindCommonType,
            "cannot find common type",
            Severity::Error)
        .AddSpan (start, end)
        /*.AddHelp ("сonsider using an explicit cast")*/;
    return nullptr;
}
// NOLINTEND(readability-function-cognitive-complexity)

OptValue
Sema::foldBinary (
    BinOp        op,
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

                    if ((op == BinOp::Div || op == BinOp::Rem) && r == 0) {
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
                    case BinOp::Plus: resData = l + r; break;
                    case BinOp::Minus: resData = l - r; break;
                    case BinOp::Mul: resData = l * r; break;
                    case BinOp::Div: resData = l / r; break;
                    case BinOp::Eq: resData = static_cast<int64_t> (l == r); break;
                    case BinOp::NEq: resData = static_cast<int64_t> (l != r); break;
                    case BinOp::Lt: resData = static_cast<int64_t> (l < r); break;
                    case BinOp::LtEq: resData = static_cast<int64_t> (l <= r); break;
                    case BinOp::Gt: resData = static_cast<int64_t> (l > r); break;
                    case BinOp::GtEq: resData = static_cast<int64_t> (l >= r); break;
                    default: return std::nullopt;
                    }
                    return Value (ValueKind::Const, resData, resType);
                },
                rhs.Data);
        },
        lhs.Data);
}

OptValue
Sema::foldUnary (
    UnOp op, const Value &rhs, Type *resType, llvm::SMLoc start, llvm::SMLoc end) {
    return std::visit (
        [&] (auto &&r) -> OptValue {
            using T = std::decay_t<decltype (r)>;
            ValueData resData;

            switch (op) {
            case UnOp::Minus: resData = -r; break;
            case UnOp::Not: {
                if constexpr (std::is_same_v<T, int64_t>) {
                    resData = static_cast<int64_t> (!r);
                } else {
                    _diag
                        .Report (
                            DiagCode::ECannotApplyOp,
                            "cannot apply operator '!' with type '"
                                + rhs.Type->ToString () + "'",
                            Severity::Error)
                        .AddSpan (start, end);
                    return std::nullopt;
                }
                break;
            }
            default: return std::nullopt;
            }

            return Value (ValueKind::Const, resData, resType);
        },
        rhs.Data);
}

Sema::SemanticResult
Sema::implicitlyCast (
    SemanticResult val, Type **expectedType, llvm::SMLoc start, llvm::SMLoc end) {
    if (!val.Val.has_value ()) {
        return {};
    }
    resolveType (&val.Val->Type);
    resolveType (expectedType);

    Type *src = val.Val->Type;
    Type *dst = *expectedType;

    if (*src == *dst) {
        return val;
    }

    _diag
        .Report (
            DiagCode::ECannotImplCast,
            "cannot implicitly cast '" + src->ToString () + "' to '" + dst->ToString ()
                + "'",
            Severity::Error)
        .AddSpan (start, end);

    return {};
}

bool
Sema::canFit (Value &val, const Type *targetType) {
    return std::visit (
        [&] (auto &&arg) -> bool {
            using T = std::decay_t<decltype (arg)>;

            if (const auto *it = llvm::dyn_cast<IntegerType> (targetType)) {
                unsigned bits       = it->BitWidth ();
                bool     isUnsigned = it->IsUnsigned ();

                if (isUnsigned) {
                    if (arg < 0) {
                        return false;
                    }
                    auto     uval = static_cast<uint64_t> (arg);
                    uint64_t umax = (1ULL << bits) - 1;
                    return uval <= umax;
                }
                auto min = -static_cast<int64_t> ((1ULL << (bits - 1)));
                auto max = static_cast<int64_t> ((1ULL << (bits - 1)) - 1);
                return arg >= min && arg <= max;
            }

            return false;
        },
        val.Data);
}

}
