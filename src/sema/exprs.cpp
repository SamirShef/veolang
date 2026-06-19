#include "ast/stmts/func_def.h"

#include <basic/types/all.h>
#include <codegen/mangler.h>
#include <sema/sema.h>

namespace veo {

using namespace ast;
using namespace symbols;

// NOLINTBEGIN(readability-identifier-naming)
#define UINT64_MIN ((uint64_t) 0)
#define UINT32_MIN ((uint32_t) 0)
#define UINT16_MIN ((uint64_t) 0)
// NOLINTEND(readability-identifier-naming)

#define size_type_range(prefix, maxOrMin)                                                \
    (_ptrBitWidth == 64                                                                  \
         ? prefix##64_##maxOrMin                                                         \
         : (_ptrBitWidth == 32 ? prefix##32_##maxOrMin : prefix##16_##maxOrMin))

static std::optional<long long>
SafeStoll (const std::string &str, int base = 10) {
    char *endptr = nullptr;
    errno        = 0;

    auto result = std::strtoll (str.c_str (), &endptr, base);

    if (endptr == str.c_str ()) {
        return std::nullopt;
    }

    if (errno == ERANGE) {
        return std::nullopt;
    }

    return result;
}

static BinOp
AsgnOpToBinOp (AsgnOp op) {
#define variant(kind, op)                                                                \
    case AsgnOp::kind: return BinOp::op;
    switch (op) {
        variant (PlusEq, Plus);
        variant (MinusEq, Minus);
        variant (MulEq, Mul);
        variant (DivEq, Div);
        variant (RemEq, Rem);
        variant (AndEq, BitAnd);
        variant (OrEq, BitOr);
        variant (XorEq, BitXor);
    default: return BinOp::Invalid;
    }
#undef variant
}

Sema::SemanticResult
Sema::analyzeExpr (Expr *expr, Type *expectedType) {
#define variant(kind, func, type)                                                        \
    case NodeKind::kind: return func (llvm::cast<type> (expr), expectedType);
    if (expr == nullptr) {
        return {};
    }
    switch (expr->Kind ()) {
        variant (LitExpr, analyzeLiteralExpr, LiteralExpr);
        variant (BinExpr, analyzeBinaryExpr, BinaryExpr);
        variant (UnExpr, analyzeUnaryExpr, UnaryExpr);
        variant (VarExpr, analyzeVarExpr, VarExpr);
        variant (FuncCall, analyzeFuncCall, FuncCall);
        variant (AsgnExpr, analyzeAsgnExpr, AsgnExpr);
        variant (FieldExpr, analyzeFieldExpr, FieldExpr);
        variant (StructInstance, analyzeStructInstance, StructInstance);
        variant (MethodCall, analyzeMethodCall, MethodCall);
        variant (TernaryExpr, analyzeTernaryExpr, TernaryExpr);
        variant (CastExpr, analyzeCastExpr, CastExpr);
        variant (RefExpr, analyzeRefExpr, RefExpr);
        variant (DerefExpr, analyzeDerefExpr, DerefExpr);
        variant (NilExpr, analyzeNilExpr, NilExpr);
        variant (TypeExpr, analyzeTypeExpr, TypeExpr);
    default: return {};
    }
#undef variant
}

#define int_lit(kind, typeName, bitWidth, isUnsigned, min, max)                          \
    case TokenKind::kind##Lit: {                                                         \
        int base = 10;                                                                   \
        if (val.length () > 2) {                                                         \
            switch (tolower (val[1])) {                                                  \
            case 'x': base = 16; break;                                                  \
            case 'o': base = 8; break;                                                   \
            case 'b': base = 2; break;                                                   \
            default: base = 10; break;                                                   \
            }                                                                            \
            if (base != 10) {                                                            \
                val.erase (0, 2);                                                        \
            }                                                                            \
        }                                                                                \
        auto ival = SafeStoll (val, base);                                               \
        if (!ival.has_value ()) {                                                        \
            _diag                                                                        \
                .Report (                                                                \
                    DiagCode::EIntLitTooLarge,                                           \
                    "integer literal is too large for type '" #typeName "'",             \
                    Severity::Error)                                                     \
                .AddSpan (                                                               \
                    le->Start (),                                                        \
                    le->End (),                                                          \
                    "value overflows " + std::to_string (bitWidth) + "-bit integer")     \
                .AddNote (                                                               \
                    "the maximum supported literal value is " + std::to_string (max)     \
                    + " (max " #typeName ")");                                           \
            return {};                                                                   \
        }                                                                                \
        auto *type  = TokenKind::kind##Lit == TokenKind::USizeLit                        \
                              || TokenKind::kind##Lit == TokenKind::ISizeLit             \
                          ? createType<SizeType> (isUnsigned)                            \
                          : createType<IntegerType> (bitWidth, isUnsigned);              \
        auto  value = Value (ValueKind::Const, ValueData (ival.value ()), type);         \
        auto  res   = SemanticResult (                                                   \
            value,                                                                       \
            _builder.CreateLiteral (value, le->Start (), le->End ()));                   \
        res = implicitlyCast (res, &expectedType, le->Start (), le->End ());             \
        return res;                                                                      \
    }

#define float_lit(kind, floatingKind)                                                    \
    case TokenKind::kind##Lit: {                                                         \
        double fval  = std::stold (val);                                                 \
        auto   value = Value (                                                           \
            ValueKind::Const,                                                            \
            ValueData (fval),                                                            \
            createType<FloatingType> (FloatingKind::floatingKind));                      \
        auto res = SemanticResult (                                                      \
            value,                                                                       \
            _builder.CreateLiteral (value, le->Start (), le->End ()));                   \
        res = implicitlyCast (res, &expectedType, le->Start (), le->End ());             \
        return res;                                                                      \
    }

Sema::SemanticResult
Sema::analyzeLiteralExpr (LiteralExpr *le, Type *expectedType) {
    std::string val = le->Value ();
    switch (le->Kind ()) {
        int_lit (U8, u8, 8, true, 0, UINT8_MAX);
        int_lit (U16, u16, 16, true, 0, UINT16_MAX);
        int_lit (U32, u32, 32, true, 0, UINT32_MAX);
        int_lit (U64, u64, 64, true, 0, UINT64_MAX);
        int_lit (
            USize,
            usize,
            _ptrBitWidth,
            true,
            size_type_range (UINT, MIN),
            size_type_range (UINT, MAX));

        int_lit (I8, i8, 8, false, INT8_MIN, INT8_MAX);
        int_lit (I16, i16, 16, false, INT16_MIN, INT16_MAX);
        int_lit (I32, i32, 32, false, INT32_MIN, INT32_MAX);
        int_lit (I64, i64, 64, false, INT64_MIN, INT64_MAX);
        int_lit (
            ISize,
            isize,
            _ptrBitWidth,
            false,
            size_type_range (INT, MIN),
            size_type_range (INT, MAX));

        float_lit (F32, Float);
        float_lit (F64, Double);
    case TokenKind::IntLit: {
        if (expectedType == nullptr) {
            expectedType = createType<IntegerType> (
                32); // NOLINT(cppcoreguidelines-avoid-magic-numbers)
        }
        expectedType = expectedType->CanonicalType ();
        if (!expectedType->IsNumber ()) {
            _diag
                .Report (
                    DiagCode::ECannotCast,
                    "cannot implicitly cast 'i32' to '" + typeToString (expectedType)
                        + "'",
                    Severity::Error)
                .AddSpan (le->Start (), le->End ());
            return {};
        }
        int base = 10;
        if (val.length () > 2) {
            switch (tolower (val[1])) {
            case 'x': base = 16; break;
            case 'o': base = 8; break;
            case 'b': base = 2; break;
            default: base = 10; break;
            }
            if (base != 10) {
                val.erase (0, 2);
            }
        }
        auto ival = SafeStoll (val, base);
        if (!ival.has_value ()) {
            _diag
                .Report (
                    DiagCode::EIntLitTooLarge,
                    "integer literal is too large to fit in any integer type",
                    Severity::Error)
                .AddSpan (le->Start (), le->End (), "value overflows 64-bit integer")
                .AddNote (
                    "the maximum supported literal value is " + std::to_string (~0ULL)
                    + " (max u64)");
            return {};
        }
        auto value = Value (
            ValueKind::Const,
            expectedType->IsIntOrSize ()
                ? ValueData (ival.value ())
                : ValueData (static_cast<double> (ival.value ())),
            expectedType);
        if (expectedType->IsIntOrSize () && !canFit (value, expectedType)) {
            bool     isUnsigned = expectedType->IsInteger ()
                                      ? expectedType->AsInteger ()->IsUnsigned ()
                                      : expectedType->AsSize ()->IsUnsigned ();
            unsigned bits       = expectedType->IsInteger ()
                                      ? expectedType->AsInteger ()->BitWidth ()
                                      : 32;
            int64_t  min = isUnsigned ? 0 : -static_cast<int64_t> (1ULL << (bits - 1));
            uint64_t max = 0;
            if (isUnsigned) {
                max = bits == 64 ? ~0ULL : static_cast<uint64_t> (1ULL << bits) - 1;
            } else {
                max = static_cast<uint64_t> (1ULL << (bits - 1)) - 1;
            }
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
        auto value = Value (ValueKind::Const, ValueData (bval), createType<BoolType> ());
        auto res   = SemanticResult (
            value,
            _builder.CreateLiteral (value, le->Start (), le->End ()));
        res = implicitlyCast (res, &expectedType, le->Start (), le->End ());
        return res;
    }
    case TokenKind::CharLit: {
        int64_t cval = val.size () == 1 ? val[0] : 0;
        auto value = Value (ValueKind::Const, ValueData (cval), createType<CharType> ());
        auto res   = SemanticResult (
            value,
            _builder.CreateLiteral (value, le->Start (), le->End ()));
        res = implicitlyCast (res, &expectedType, le->Start (), le->End ());
        return res;
    }
    default: return {};
    }
}

#undef float_lit
#undef int_lit

Sema::SemanticResult
Sema::analyzeBinaryExpr (BinaryExpr *be, Type *expectedType) {
    auto [lhs, rhs] = analyzeBinaryExprOperands (be);
    if (!lhs.Val.has_value () || !rhs.Val.has_value ()) {
        return {};
    }
    resolveType (&lhs.Val->Type);
    resolveType (&rhs.Val->Type);
    BinOp op         = be->Op ();
    Type *commonType = nullptr;
    Type *resType    = nullptr;

    if (op >= BinOp::LogAnd && op <= BinOp::LogOr) {
        commonType = createType<BoolType> ();
        resType    = commonType;
    } else if (op >= BinOp::Eq && op <= BinOp::GtEq) {
        commonType
            = getCommonType (lhs.Val->Type, rhs.Val->Type, be->Start (), be->End ());
        resType = createType<BoolType> ();
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
    if (op >= BinOp::BitAnd && op <= BinOp::BitOr) {
        if (!lhs.Val->Type->IsInteger () || !rhs.Val->Type->IsInteger ()) {
            _diag
                .Report (
                    DiagCode::ECannotApplyOp,
                    "binary operator '" + std::string (BinOpToString (op))
                        + "' cannot be applied to non-integer types '"
                        + typeToString (lhs.Val->Type) + "' and '"
                        + typeToString (rhs.Val->Type) + "'",
                    Severity::Error)
                .AddSpan (be->Start (), be->End ());
            return {};
        }
    }
    if (lhs.Val->Kind == ValueKind::Const && rhs.Val->Kind == ValueKind::Const) {
        auto foldedValue
            = foldBinary (op, *lhs.Val, *rhs.Val, resType, be->Start (), be->End ());
        auto res = SemanticResult (foldedValue, nullptr);
        if (expectedType != nullptr) {
            res = implicitlyCast (res, &expectedType, be->Start (), be->End ());
        }
        if (op >= BinOp::BitAnd && op <= BinOp::BitOr) {
            int64_t result
                = foldBinaryBitwise (lhs.Val->Data, rhs.Val->Data, commonType, op);
            res.Val = Value (ValueKind::Const, ValueData (result), commonType);
        }
        if (res.Val.has_value ()) {
            return {
                res.Val,
                _builder.CreateLiteral (res.Val.value (), be->Start (), be->End ())
            };
        }
    }

    auto *node = _builder.CreateBinary (
        op,
        commonType,
        resType,
        lhs.Node,
        rhs.Node,
        be->Start (),
        be->End ());

    auto res = SemanticResult (Value (ValueKind::Unknown, resType), node);
    res      = implicitlyCast (res, &expectedType, be->Start (), be->End ());
    return res;
}

std::pair<Sema::SemanticResult, Sema::SemanticResult>
Sema::analyzeBinaryExprOperands (BinaryExpr *be) {
    SemanticResult lhs;
    SemanticResult rhs;
    if (be->Lhs ()->Kind () == NodeKind::NilExpr
        && be->Rhs ()->Kind () != NodeKind::NilExpr) {
        rhs = analyzeExpr (be->Rhs (), nullptr);
        if (rhs.Val.has_value ()) {
            resolveType (&rhs.Val->Type);
            lhs = analyzeExpr (be->Lhs (), rhs.Val->Type);
        }
    } else if (be->Rhs ()->Kind () == NodeKind::NilExpr
               && be->Lhs ()->Kind () != NodeKind::NilExpr) {
        lhs = analyzeExpr (be->Lhs (), nullptr);
        if (lhs.Val.has_value ()) {
            resolveType (&lhs.Val->Type);
            rhs = analyzeExpr (be->Rhs (), lhs.Val->Type);
        }
    } else {
        lhs = analyzeExpr (be->Lhs (), nullptr);
        rhs = analyzeExpr (be->Rhs (), nullptr);
    }
    return { lhs, rhs };
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
                    "cannot apply operator '" + std::string (UnOpToString (op))
                        + "' with type '" + typeToString (rhs.Val->Type) + "'",
                    Severity::Error)
                .AddSpan (ue->Start (), ue->End ());
            return {};
        }
        resType = rhs.Val->Type;
        break;

    case UnOp::Not:
        if (!rhs.Val->Type->IsBool ()) {
            _diag
                .Report (
                    DiagCode::ECannotApplyOp,
                    "cannot apply operator '" + std::string (UnOpToString (op))
                        + "' with type '" + typeToString (rhs.Val->Type) + "'",
                    Severity::Error)
                .AddSpan (ue->Start (), ue->End ());
            return {};
        } else {
            resType = rhs.Val->Type;
        }
        break;
    case UnOp::Inverse:
        if (!rhs.Val->Type->IsInteger ()) {
            _diag
                .Report (
                    DiagCode::ECannotApplyOp,
                    "unary operator '" + std::string (UnOpToString (op))
                        + "' cannot be applied to non-integer type '"
                        + typeToString (rhs.Val->Type) + "'",
                    Severity::Error)
                .AddSpan (ue->Start (), ue->End ());
            return {};
        }
        resType = rhs.Val->Type;
        break;

    default: return {};
    }

    if (rhs.Val->Kind == ValueKind::Const) {
        auto foldedValue = foldUnary (op, *rhs.Val, resType, ue->Start (), ue->End ());
        auto res         = SemanticResult (foldedValue, nullptr);
        if (op == UnOp::Inverse) {
            auto iVal       = std::get<0> (rhs.Val->Data);
            bool isUnsigned = resType->AsInteger ()->IsUnsigned ();
            res.Val         = Value (
                ValueKind::Const,
                ValueData (
                    isUnsigned ? (~static_cast<int64_t> (iVal)) // NOLINT
                               : ~iVal), // NOLINT(hicpp-signed-bitwise)
                resType);
        }
        if (expectedType != nullptr) {
            res = implicitlyCast (res, &expectedType, ue->Start (), ue->End ());
        }
        if (res.Val.has_value ()) {
            return {
                res.Val,
                _builder.CreateLiteral (res.Val.value (), ue->Start (), ue->End ())
            };
        }
    }

    auto *node = _builder.CreateUnary (op, resType, rhs.Node, ue->Start (), ue->End ());
    auto  res  = SemanticResult (Value (ValueKind::Unknown, resType), node);
    res        = implicitlyCast (res, &expectedType, ue->Start (), ue->End ());
    return res;
}

// NOLINTBEGIN(readability-convert-member-functions-to-static)
Sema::SemanticResult
Sema::analyzeVarExpr (VarExpr *ve, Type *expectedType) {
    auto *var = getVariable (ve->Name ().Val);
    if (var == nullptr) {
        if (auto *alias = lookupLocalType (ve->Name ().Val)) {
            return { Value (ValueKind::Type, createType<AliasType> (ve->Name (), alias)),
                     nullptr };
        }
        if (auto *s = getStruct (ve->Name ().Val)) {
            return { Value (ValueKind::Type, createType<StructType> (s)), nullptr };
        }
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
        var->IsConst ? var->Val->Data : ValueData (),
        var->Type);
    hir::Node *node = nullptr;
    if (var->IsConst && !value.Type->IsStruct ()) {
        node = _builder.CreateLiteral (value, ve->Start (), ve->End ());
    } else {
        node = _builder.CreateLoadVar (
            var->HIR,
            var->Type,
            var->IsGlobal,
            ve->Start (),
            ve->End ());
    }
    auto res = SemanticResult (value, node);
    res      = implicitlyCast (res, &expectedType, ve->Start (), ve->End ());
    return res;
}

Sema::SemanticResult
Sema::analyzeFuncCall (FuncCall *fc, Type *expectedType) {
    auto it = _mod->Funcs.find (fc->Name ().Val);
    if (it == _mod->Funcs.end ()) {
        _diag
            .Report (
                DiagCode::ECannotFindFunction,
                "cannot find function '" + fc->Name ().Val + "' in this scope",
                Severity::Error)
            .AddSpan (fc->Name ().Start, fc->Name ().End, "not found in this scope");
        return {};
    }
    auto *candidates = &it->second;

    std::vector<Type *>         argTypes;
    std::vector<SemanticResult> argResults;
    for (auto &a : fc->Args ()) {
        auto argRes = analyzeExpr (a, nullptr);
        if (!argRes.Val.has_value ()) {
            continue;
        }
        argResults.emplace_back (argRes);
        argTypes.emplace_back (argRes.Val->Type);
    }

    auto *func = resolveBestOverload (
        candidates,
        fc->GenericParams (),
        argTypes,
        fc->Start (),
        fc->End ());
    if (func == nullptr) {
        return {};
    }

    ast::FuncDef *funcDef = nullptr;
    if (auto it = _genericFuncs.find (func); it != _genericFuncs.end ()) {
        funcDef = it->second;
    }
    auto *finalFunc = func;

    if (funcDef != nullptr && funcDef->IsGeneric ()) {
        std::unordered_map<std::string, Type *> substMap;
        const auto                             &genericParams = funcDef->GenericParams ();

        if (!fc->GenericParams ().empty ()) {
            for (size_t i = 0; i < genericParams.size (); ++i) {
                substMap[genericParams[i].Name.Val] = fc->GenericParams ()[i];
            }
        } else {
            std::unordered_map<std::string, Type *> inferredMap;
            for (size_t i = 0; i < argTypes.size (); ++i) {
                deduceGenericTypes (func->Args[i].Type, argTypes[i], inferredMap);
            }
            substMap = std::move (inferredMap);
        }

        finalFunc = generateGenericFunc (func, funcDef, substMap);
        if (finalFunc == nullptr) {
            return {};
        }
    }

    std::vector<hir::Node *> hirArgs;
    hirArgs.reserve (argResults.size ());
    for (size_t i = 0; i < argResults.size (); ++i) {
        auto res = analyzeExpr (fc->Args ()[i], finalFunc->Args[i].Type);
        hirArgs.emplace_back (res.Node);
    }

    auto res = SemanticResult (
        Value (ValueKind::Unknown, finalFunc->RetType),
        _builder.CreateCall (
            _funcs.at (finalFunc),
            std::move (hirArgs),
            fc->Start (),
            fc->End ()));
    res = implicitlyCast (res, &expectedType, fc->Start (), fc->End ());
    return res;
}

symbols::Function *
Sema::generateGenericFunc (
    symbols::Function                             *func,
    ast::FuncDef                                  *fd,
    const std::unordered_map<std::string, Type *> &substMap) {
    auto       *oldBB = _builder.InsertBlock ();
    const auto &name  = fd->Name ().Val;

    auto specFunc     = std::make_unique<symbols::Function> ();
    specFunc->Name    = basic::NameObj (name, fd->Name ().Start, fd->Name ().End);
    specFunc->RetType = substituteGenericTypes (fd->RetType (), substMap);

    for (const auto &arg : fd->Args ()) {
        Type *concreteArgType = substituteGenericTypes (arg.Type, substMap);
        specFunc->Args.emplace_back (arg.Name, concreteArgType);
    }

    auto *funcPtr = specFunc.get ();

    _mod->Funcs[name].Candidates.push_back (std::move (specFunc));

    std::vector<hir::VarDef *> hirArgs;
    hirArgs.reserve (funcPtr->Args.size ());
    for (const auto &arg : funcPtr->Args) {
        hirArgs.emplace_back (_builder.CreateVariable (
            arg.Name,
            arg.Type,
            nullptr,
            false,
            false,
            arg.Name.Start,
            arg.Name.End,
            nullptr,
            hir::MangleKind::Veo,
            false,
            false));
    }

    auto *funcNode = _builder.CreateFunction (
        funcPtr->Name,
        funcPtr->RetType,
        std::move (hirArgs),
        fd->Start (),
        fd->End (),
        funcPtr,
        hir::MangleKind::Veo);
    _funcs.emplace (funcPtr, funcNode);
    auto *entry = _builder.CreateBasicBlock (funcNode, "entry");
    _builder.SetInsertionPoint (entry);

    _funcRetTypes.push (funcPtr->RetType);
    _vars.emplace ();

    for (size_t i = 0; i < funcPtr->Args.size (); ++i) {
        _vars.top ().Vars.emplace (
            funcPtr->Args[i].Name.Val,
            symbols::Variable (
                funcPtr->Args[i].Name,
                funcPtr->Args[i].Type,
                false,
                false,
                nullptr,
                hir::MangleKind::Veo,
                std::nullopt,
                funcNode->Args ()[i]));
    }

    for (auto *stmt : fd->Body ()) {
        analyzeStmt (stmt);
    }

    _vars.pop ();
    _funcRetTypes.pop ();

    _builder.SetInsertionPoint (oldBB);
    return funcPtr;
}

Sema::SemanticResult
Sema::analyzeAsgnExpr (AsgnExpr *ae, Type *expectedType) {
    if (ae->Ptr () == nullptr) {
        return {};
    }
    if (ae->Ptr ()->Kind () != NodeKind::VarExpr
        && ae->Ptr ()->Kind () != NodeKind::FieldExpr
        && ae->Ptr ()->Kind () != NodeKind::DerefExpr) {
        _diag
            .Report (
                DiagCode::ELHSIsNotAssignable,
                "left-hand side of assignment is not assignable",
                Severity::Error)
            .AddSpan (ae->Ptr ()->Start (), ae->Ptr ()->End ());
        return {};
    }

    auto ptr = analyzeExpr (ae->Ptr (), nullptr);
    if (!ptr.Val.has_value ()) {
        return {};
    }
    auto expr = analyzeExpr (ae->Init (), ptr.Val->Type);
    if (!expr.Val.has_value ()) {
        return {};
    }
    if (!canApplyAsgnOp (ae->Op (), ptr.Val->Type)) {
        _diag
            .Report (
                DiagCode::ECannotApplyOp,
                "cannot apply operator '" + std::string (AsgnOpToString (ae->Op ()))
                    + "' with type '" + typeToString (ptr.Val->Type) + "'",
                Severity::Error)
            .AddSpan (ae->Ptr ()->Start (), ae->Ptr ()->End ());
        return {};
    }
    if (ae->Ptr ()->Kind () == NodeKind::VarExpr) {
        expr = analyzeAsgnVar (ae, expr, ptr);
    } else if (ae->Ptr ()->Kind () == NodeKind::FieldExpr) {
        expr = analyzeAsgnField (ae, expr, ptr);
    } else { // pointer
        expr = analyzeAsgnPtr (ae, expr, ptr);
    }
    expr = implicitlyCast (expr, &expectedType, ae->Start (), ae->End ());
    if (!expr.Val.has_value ()) {
        return {};
    }
    auto *node = _builder.CreateStore (
        ptr.Node,
        expr.Node,
        expr.Val->Type,
        ae->Start (),
        ae->End ());
    return { expr.Val, node };
}

Sema::SemanticResult
Sema::analyzeAsgnVar (
    AsgnExpr *ae, Sema::SemanticResult &expr, const Sema::SemanticResult &ptr) {
    auto *var = getVariable (llvm::cast<VarExpr> (ae->Ptr ())->Name ().Val);
    if (var->IsConst) {
        _diag
            .Report (
                DiagCode::ECannotModifyConst,
                "cannot assign to a constant variable '" + var->Name.Val + "'",
                Severity::Error)
            .AddSpan (ae->Ptr ()->Start (), ae->Ptr ()->End ());
        return {};
    }
    auto op = AsgnOpToBinOp (ae->Op ());
    if (ae->Op () != AsgnOp::Eq) {
        expr.Node = _builder.CreateBinary (
            op,
            expr.Val->Type,
            expr.Val->Type,
            ptr.Node,
            expr.Node,
            ae->Init ()->Start (),
            ae->Init ()->End ());
    }
    expr.Val = Value (ValueKind::Unknown, expr.Val->Type);
    return expr;
}

Sema::SemanticResult
Sema::analyzeAsgnField (
    AsgnExpr *ae, Sema::SemanticResult &expr, const Sema::SemanticResult &ptr) {
    auto *fieldExpr = llvm::cast<FieldExpr> (ae->Ptr ());
    auto  base      = analyzeExpr (fieldExpr->Base (), nullptr);
    if (!base.Val.has_value ()) {
        return {};
    }
    auto *targetType = base.Val->Type->CanonicalType ();
    while (targetType->IsPointer ()) {
        targetType = targetType->AsPointer ()->Base ()->CanonicalType ();
        base.Node  = _builder.CreateDereference (
            base.Node,
            targetType,
            fieldExpr->Start (),
            fieldExpr->End ());
    }
    if (!targetType->IsStruct ()) {
        _diag
            .Report (
                DiagCode::ECannotAccessFromNonStruct,
                "attempted to access a field on a non-structure type '"
                    + typeToString (targetType) + "'",
                Severity::Error)
            .AddSpan (fieldExpr->Name ().Start, fieldExpr->Name ().End);
        return {};
    }
    auto *sym = targetType->AsStruct ()->BaseSymbol ();
    if (!sym->IsComplete) {
        _diag
            .Report (
                DiagCode::EIncompleteType,
                "type '" + typeToString (targetType) + "' is incomplete",
                Severity::Error)
            .AddSpan (fieldExpr->Start (), fieldExpr->End ());
        return {};
    }
    bool baseIsStatic     = base.Val->Kind == ValueKind::Type;
    bool baseIsThis       = !baseIsStatic && _insideMethod.has_value ()
                            && *_insideMethod->second == *targetType;
    bool canAccessPrivate = baseIsThis
                            || baseIsStatic && _insideMethod.has_value ()
                                   && *_insideMethod->second == *targetType;
    if (baseIsThis) {
        // Modifying 'this' object
        _insideMethod->first->IsConst = false;
    }
    auto field = std::ranges::find_if (sym->Fields, [&] (const symbols::Field &field) {
        return field.Name.Val == fieldExpr->Name ().Val;
    });
    if (field == sym->Fields.end ()) {
        return {};
    }
    if (!canAccessField (
            fieldExpr->Name (),
            *field,
            sym,
            baseIsStatic,
            canAccessPrivate)) {
        return {};
    }
    if (field->IsConst) {
        _diag
            .Report (
                DiagCode::ECannotModifyConst,
                "cannot assign to a constant field '" + field->Name.Val + "' in struct '"
                    + sym->Name.Val + "'",
                Severity::Error)
            .AddSpan (ae->Ptr ()->Start (), ae->Ptr ()->End ());
        return {};
    }
    auto op = AsgnOpToBinOp (ae->Op ());
    if (ae->Op () != AsgnOp::Eq) {
        expr.Node = _builder.CreateBinary (
            op,
            expr.Val->Type,
            expr.Val->Type,
            ptr.Node,
            expr.Node,
            ae->Init ()->Start (),
            ae->Init ()->End ());
    }
    expr.Val = Value (ValueKind::Unknown, expr.Val->Type);
    return expr;
}

Sema::SemanticResult
Sema::analyzeAsgnPtr (AsgnExpr *ae, SemanticResult &expr, const SemanticResult &ptr) {
    auto op = AsgnOpToBinOp (ae->Op ());
    if (ae->Op () != AsgnOp::Eq) {
        expr.Node = _builder.CreateBinary (
            op,
            expr.Val->Type,
            expr.Val->Type,
            ptr.Node,
            expr.Node,
            ae->Init ()->Start (),
            ae->Init ()->End ());
    }
    return expr;
}

Sema::SemanticResult
Sema::analyzeFieldExpr (FieldExpr *fe, Type *expectedType) {
    auto base = analyzeExpr (fe->Base (), nullptr);
    if (!base.Val.has_value ()) {
        return {};
    }
    auto *targetType = base.Val->Type->CanonicalType ();
    while (targetType->IsPointer ()) {
        targetType = targetType->AsPointer ()->Base ()->CanonicalType ();
        base.Node  = _builder.CreateDereference (
            base.Node,
            targetType,
            fe->Start (),
            fe->End ());
    }
    if (!targetType->IsStruct ()) {
        _diag
            .Report (
                DiagCode::ECannotAccessFromNonStruct,
                "attempted to access a field on a non-structure type '"
                    + typeToString (targetType) + "'",
                Severity::Error)
            .AddSpan (fe->Name ().Start, fe->Name ().End);
        return {};
    }
    auto *s = targetType->AsStruct ()->BaseSymbol ();
    if (!s->IsComplete) {
        _diag
            .Report (
                DiagCode::EIncompleteType,
                "type '" + typeToString (targetType) + "' is incomplete",
                Severity::Error)
            .AddSpan (fe->Start (), fe->End ());
        return {};
    }
    bool baseIsStatic     = base.Val->Kind == ValueKind::Type;
    bool baseIsThis       = !baseIsStatic && _insideMethod.has_value ()
                            && *_insideMethod->second == *targetType;
    bool canAccessPrivate = baseIsThis
                            || baseIsStatic && _insideMethod.has_value ()
                                   && *_insideMethod->second == *targetType;
    auto it = std::ranges::find_if (s->Fields, [&] (const symbols::Field &field) {
        return field.Name.Val == fe->Name ().Val;
    });
    if (it == s->Fields.end ()) {
        std::ostringstream availableFields;
        size_t             index = 0;
        for (const auto &field : s->Fields) {
            if (!field.IsStatic && field.Access == AccessModifier::Pub) {
                if (index != 0) {
                    availableFields << ", ";
                }
                availableFields << field.Name.Val;
                ++index;
            }
        }
        auto &err = _diag
                        .Report (
                            DiagCode::EUndefined,
                            "type '" + s->Name.Val + "' has no field named '"
                                + fe->Name ().Val + "'",
                            Severity::Error)
                        .AddSpan (fe->Name ().Start, fe->Name ().End);
        if (!availableFields.str ().empty ()) {
            err.AddNote ("available fields: " + availableFields.str ());
        }
        return {};
    }
    if (!canAccessField (fe->Name (), *it, s, baseIsStatic, canAccessPrivate)) {
        return {};
    }
    auto       val  = Value (ValueKind::Unknown, it->Type);
    hir::Node *node = nullptr;
    if (it->IsStatic) {
        node = _builder.CreateLoadGlobalVarByName (
            Mangler::MangleStaticField (s, it->Name.Val),
            it->Type,
            fe->Start (),
            fe->End ());
    } else {
        node = _builder.CreateFieldExpr (
            base.Node,
            it->Index,
            val.Type,
            fe->Start (),
            fe->End ());
    }
    auto res = SemanticResult (val, node);
    res      = implicitlyCast (res, &expectedType, fe->Start (), fe->End ());
    return res;
}

Sema::SemanticResult
Sema::analyzeStructInstance (StructInstance *si, Type *expectedType) {
    resolveType (&si->StructType ());
    Type *structType = si->StructType ()->CanonicalType ();
    if (!structType->IsStruct ()) {
        _diag
            .Report (
                DiagCode::EUndefined,
                "struct '" + typeToString (structType) + "' is not defined",
                Severity::Error)
            .AddSpan (si->Start (), si->End ());
        return {};
    }
    auto *s = structType->AsStruct ()->BaseSymbol ();
    std::vector<std::pair<size_t, hir::Node *>> fields;
    std::unordered_map<size_t, NameObj>         initializedFields;
    ValueKind                                   valKind = ValueKind::Const;
    bool                                        checkAccessModifiers
        = !_insideMethod.has_value ()
          || *_insideMethod->second != *createType<basic::StructType> (s);
    if (checkAccessModifiers) {
        for (const auto &field : s->Fields) {
            if (field.Access != AccessModifier::Pub) {
                _diag
                    .Report (
                        DiagCode::ECannotInitStructWithPrivFields,
                        "cannot structurally initialize '" + s->Name.Val
                            + "' outside of its implementation because it contains "
                              "private fields",
                        Severity::Error)
                    .AddSpan (si->Start (), si->End ())
                    .AddNote (
                        "use the constructor function '" + s->Name.Val
                        + ".new()' instead if available");
                return {};
            }
        }
    }
    for (const auto &[name, expr] : si->Fields ()) {
        auto it = std::ranges::find_if (s->Fields, [&] (const symbols::Field &f) {
            return f.Name.Val == name.Val; // NOLINT(modernize-type-traits)
        });
        if (it == s->Fields.end ()) {
            _diag
                .Report (
                    DiagCode::EUndefined,
                    "stuct '" + s->Name.Val + "' has no field named '" + name.Val + "'",
                    Severity::Error)
                .AddSpan (name.Start, name.End);
            continue;
        }
        const auto *field = it.base ();
        size_t      index = field->Index;
        if (auto it = initializedFields.find (index); it != initializedFields.end ()) {
            _diag
                .Report (
                    DiagCode::EFieldSpecifiedMoreOnce,
                    "field '" + name.Val + "' specified more than once",
                    Severity::Error)
                .AddSpan (it->second.Start, it->second.End, "first assignment", false)
                .AddSpan (name.Start, name.End, "field already initialized here");
            continue;
        }
        if (it->IsConst) {
            _diag
                .Report (
                    DiagCode::ECannotInitConstField,
                    "cannot initialize constant field '" + name.Val + "'",
                    Severity::Error)
                .AddSpan (name.Start, name.End);
            continue;
        }
        if (it->IsStatic) {
            _diag
                .Report (
                    DiagCode::ECannotInitStaticField,
                    "cannot initialize static field '" + name.Val + "'",
                    Severity::Error)
                .AddSpan (name.Start, name.End);
            continue;
        }
        initializedFields.emplace (index, name);
        auto val = analyzeExpr (expr, field->Type);
        if (!val.Val.has_value ()) {
            continue;
        }
        if (val.Val->Kind == ValueKind::Unknown) {
            valKind = ValueKind::Unknown;
        }
        fields.emplace_back (index, val.Node);
    }
    auto  val  = Value (valKind, createType<StructType> (s));
    auto *node = _builder.CreateStructInstance (
        std::move (fields),
        s,
        val.Type,
        si->Start (),
        si->End ());
    auto res = SemanticResult (val, node);
    res      = implicitlyCast (res, &expectedType, si->Start (), si->End ());
    return res;
}

Sema::SemanticResult
Sema::analyzeMethodCall (MethodCall *mc, Type *expectedType) {
    auto base = analyzeExpr (mc->Base (), nullptr);
    if (!base.Val.has_value ()) {
        return {};
    }
    auto *targetType = base.Val->Type->CanonicalType ();
    while (targetType->IsPointer ()) {
        targetType = targetType->AsPointer ()->Base ()->CanonicalType ();
        base.Node  = _builder.CreateDereference (
            base.Node,
            targetType,
            mc->Start (),
            mc->End ());
    }
    auto *s = targetType->IsStruct () ? targetType->AsStruct ()->BaseSymbol () : nullptr;
    if (s != nullptr && !s->IsComplete) {
        _diag
            .Report (
                DiagCode::EIncompleteType,
                "type '" + typeToString (targetType) + "' is incomplete",
                Severity::Error)
            .AddSpan (mc->Start (), mc->End ());
        return {};
    }
    bool baseIsConstVar   = base.Val->Kind == ValueKind::Const
                            && base.Node->Kind () == hir::NodeKind::LoadVar;
    bool baseIsStatic     = base.Val->Kind == ValueKind::Type;
    bool baseIsThis       = !baseIsStatic && _insideMethod.has_value ()
                            && *_insideMethod->second == *targetType;
    bool canAccessPrivate = baseIsThis
                            || baseIsStatic && _insideMethod.has_value ()
                                   && *_insideMethod->second == *targetType;
    std::unordered_map<std::string, MethodCandidates> *methods = nullptr;
    if (s != nullptr) {
        methods = &s->Methods;
    } else if (targetType->IsTrait ()) {
        methods = &targetType->AsTrait ()->BaseSymbol ()->Methods;
    } else {
        methods = &_mod->PrimitiveMethods[targetType];
    }
    auto it = methods->find (mc->Name ().Val);
    if (it == methods->end ()) {
        _diag
            .Report (
                DiagCode::EUndefined,
                "type '" + typeToString (targetType) + "' has no method named '"
                    + mc->Name ().Val + "'",
                Severity::Error)
            .AddSpan (mc->Name ().Start, mc->Name ().End);
        return {};
    }
    auto *candidates = &it->second;

    std::vector<Type *>         argTypes;
    std::vector<SemanticResult> argResults;
    for (auto &a : mc->Args ()) {
        auto argRes = analyzeExpr (a, nullptr);
        if (!argRes.Val.has_value ()) {
            continue;
        }
        argResults.emplace_back (argRes);
        argTypes.emplace_back (argRes.Val->Type);
    }

    if (s != nullptr && s->StructDef != nullptr) {
        for (auto &param : s->StructDef->GenericParams ()) {
            mc->GenericParams ().push_back (createType<GenericType> (param.Name.Val));
        }
    }

    auto *method = resolveBestOverload (
        candidates,
        mc->GenericParams (),
        argTypes,
        mc->Start (),
        mc->End ());
    if (method == nullptr) {
        return {};
    }

    ast::FuncDef *methodDef = nullptr;
    if (auto it = _genericMethods.find (method); it != _genericMethods.end ()) {
        methodDef = it->second;
    }
    auto *finalMethod = method;

    if (!canAccessMethod (
            mc->Name (),
            *method,
            targetType,
            baseIsStatic,
            canAccessPrivate)) {
        return {};
    }
    if (baseIsThis && _insideMethod.has_value ()) {
        auto it = _methodCallFromAnotherMethod.find (_insideMethod->first);
        if (it == _methodCallFromAnotherMethod.end ()) {
            _methodCallFromAnotherMethod.emplace (
                _insideMethod->first,
                std::move (std::vector<symbols::Method *>{}));
        }
        _methodCallFromAnotherMethod.at (_insideMethod->first).push_back (method);
    }
    if (baseIsConstVar) {
        _methodCallOnConstBase.emplace_back (mc, method);
    }

    if (methodDef != nullptr && methodDef->IsGeneric ()) {
        std::unordered_map<std::string, Type *> substMap;
        const auto &genericParams = methodDef->GenericParams ();

        if (!mc->GenericParams ().empty ()) {
            for (size_t i = 0; i < genericParams.size (); ++i) {
                substMap[genericParams[i].Name.Val] = mc->GenericParams ()[i];
            }
        } else {
            std::unordered_map<std::string, Type *> inferredMap;
            for (size_t i = 0; i < argTypes.size (); ++i) {
                deduceGenericTypes (method->Func->Args[i].Type, argTypes[i], inferredMap);
            }
            substMap = std::move (inferredMap);
        }

        finalMethod = generateGenericMethod (method, targetType, methodDef, substMap);
        if (finalMethod == nullptr) {
            return {};
        }
    }
    hir::Function *hirFunc = nullptr;
    if (auto it = _methods.find (finalMethod); it != _methods.end ()) {
        hirFunc = it->second;
    }
    std::vector<hir::Node *> hirArgs;
    if (!finalMethod->IsStatic) {
        hirArgs.emplace_back (_builder.CreateReference (
            base.Node,
            createType<PointerType> (targetType),
            mc->Start (),
            mc->End ()));
    }
    for (size_t i = 0; i < argResults.size (); ++i) {
        auto res = analyzeExpr (mc->Args ()[i], finalMethod->Func->Args[i].Type);
        hirArgs.emplace_back (res.Node);
    }

    auto *node = targetType->IsTrait () ? nullptr
                                        : _builder.CreateCall (
                                              hirFunc,
                                              std::move (hirArgs),
                                              mc->Start (),
                                              mc->End ());
    auto  val  = Value (ValueKind::Unknown, finalMethod->Func->RetType);
    auto  res  = SemanticResult (val, node);
    res        = implicitlyCast (res, &expectedType, mc->Start (), mc->End ());
    return res;
}

symbols::Method *
Sema::generateGenericMethod (
    symbols::Method                               *method,
    Type                                          *targetType,
    ast::FuncDef                                  *fd,
    const std::unordered_map<std::string, Type *> &substMap) {
    auto       *oldBB = _builder.InsertBlock ();
    const auto &name  = fd->Name ().Val;

    pushTypeScope ();
    auto *thisAlias = createType<AliasType> (NameObj ("This", {}, {}), targetType);
    registerLocalType ("This", thisAlias);

    auto specFunc     = std::make_unique<symbols::Function> ();
    specFunc->Name    = basic::NameObj (name, fd->Name ().Start, fd->Name ().End);
    specFunc->RetType = substituteGenericTypes (fd->RetType (), substMap);

    for (const auto &arg : fd->Args ()) {
        Type *concreteArgType = substituteGenericTypes (arg.Type, substMap);
        specFunc->Args.emplace_back (arg.Name, concreteArgType);
    }

    auto specMethod      = std::make_unique<symbols::Method> ();
    specMethod->Func     = std::move (specFunc);
    specMethod->IsConst  = method->IsConst;
    specMethod->IsStatic = method->IsStatic;
    specMethod->Access   = method->Access;

    symbols::Method *methodPtr = specMethod.get ();
    auto            &methods
        = targetType->CanonicalType ()->IsStruct ()
              ? targetType->CanonicalType ()->AsStruct ()->BaseSymbol ()->Methods[name]
              : _mod->PrimitiveMethods[targetType->CanonicalType ()][name];
    methods.Candidates.push_back (std::move (specMethod));

    std::vector<hir::VarDef *> hirArgs;
    if (!method->IsStatic) {
        auto *node = _builder.CreateVariable (
            basic::NameObj ("this", fd->Name ().Start, fd->Name ().End),
            createType<PointerType> (targetType),
            nullptr,
            false,
            false,
            fd->Name ().Start,
            fd->Name ().End,
            nullptr,
            hir::MangleKind::Veo,
            false,
            false);
        hirArgs.emplace_back (node);
    }
    for (const auto &arg : methodPtr->Func->Args) {
        hirArgs.emplace_back (_builder.CreateVariable (
            arg.Name,
            arg.Type,
            nullptr,
            false,
            false,
            arg.Name.Start,
            arg.Name.End,
            nullptr,
            hir::MangleKind::Veo,
            false,
            false));
    }

    auto *methodNode = _builder.CreateFunction (
        methodPtr->Func->Name,
        methodPtr->Func->RetType,
        std::move (hirArgs),
        fd->Start (),
        fd->End (),
        methodPtr->Func.get (),
        hir::MangleKind::Veo);
    auto *entry = _builder.CreateBasicBlock (methodNode, "entry");
    _builder.SetInsertionPoint (entry);
    _methods.emplace (methodPtr, methodNode);

    _funcRetTypes.push (methodPtr->Func->RetType);
    _vars.emplace ();

    if (methodNode != nullptr && !method->IsStatic) {
        auto *thisArg = methodNode->Args ()[0];
        _vars.top ().Vars.emplace (
            thisArg->Name ().Val,
            Variable (
                thisArg->Name (),
                thisArg->Type (),
                false,
                false,
                nullptr,
                hir::MangleKind::Veo,
                std::nullopt,
                thisArg));
    }
    size_t i = method->IsStatic ? 0 : 1;
    for (const auto &arg : methodPtr->Func->Args) {
        _vars.top ().Vars.emplace (
            arg.Name.Val,
            symbols::Variable (
                arg.Name,
                arg.Type,
                false,
                false,
                nullptr,
                hir::MangleKind::Veo,
                std::nullopt,
                methodNode->Args ()[i]));
        ++i;
    }

    for (auto *stmt : fd->Body ()) {
        analyzeStmt (stmt);
    }

    _vars.pop ();
    _funcRetTypes.pop ();

    _builder.SetInsertionPoint (oldBB);
    popTypeScope ();

    return methodPtr;
}

Sema::SemanticResult
Sema::analyzeTernaryExpr (TernaryExpr *te, Type *expectedType) {
    auto cond = analyzeExpr (te->Cond (), _typePool.GetOrCreate<BoolType> ());
    if (!cond.Val.has_value ()) {
        return {};
    }
    if (cond.Val->Kind == ValueKind::Const) {
        auto trueVal = analyzeExpr (te->TrueVal (), expectedType);
        if (!trueVal.Val.has_value ()) {
            return {};
        }
        if (expectedType == nullptr) {
            expectedType = trueVal.Val->Type;
        }
        auto falseVal = analyzeExpr (te->FalseVal (), expectedType);
        if (!falseVal.Val.has_value ()) {
            return {};
        }
        return std::get<0> (cond.Val->Data) == 1 ? trueVal : falseVal;
    }
    auto *tmp = _builder.CreateVariable (
        basic::NameObj (".tmp", {}, {}),
        expectedType,
        nullptr,
        false,
        false,
        te->Start (),
        te->End (),
        nullptr,
        hir::MangleKind::Veo,
        false);
    auto *trueBB  = _builder.CreateBasicBlock (_builder.Parent (), "cond.true");
    auto *falseBB = _builder.CreateBasicBlock (_builder.Parent (), "cond.false");
    auto *mergeBB = _builder.CreateBasicBlock (_builder.Parent (), "cond.merge");
    _builder.CreateBr (
        cond.Node,
        trueBB,
        falseBB,
        te->Cond ()->Start (),
        te->Cond ()->End ());

    _builder.SetInsertionPoint (trueBB);
    auto trueVal = analyzeExpr (te->TrueVal (), expectedType);
    if (!trueVal.Val.has_value ()) {
        return {};
    }
    auto *storeTrue = _builder.CreateStore (
        tmp,
        trueVal.Node,
        trueVal.Val->Type,
        te->TrueVal ()->Start (),
        te->TrueVal ()->End ());
    _builder.CreateExprStmt (storeTrue, te->TrueVal ()->Start (), te->TrueVal ()->End ());
    if (expectedType == nullptr) {
        expectedType = trueVal.Val->Type;
        tmp->SetType (expectedType);
    }
    _builder.CreateBr (mergeBB, te->Start (), te->End ());

    _builder.SetInsertionPoint (falseBB);
    auto falseVal = analyzeExpr (te->FalseVal (), expectedType);
    if (!falseVal.Val.has_value ()) {
        return {};
    }
    auto *storeFalse = _builder.CreateStore (
        tmp,
        falseVal.Node,
        falseVal.Val->Type,
        te->FalseVal ()->Start (),
        te->FalseVal ()->End ());
    _builder.CreateExprStmt (
        storeFalse,
        te->FalseVal ()->Start (),
        te->FalseVal ()->End ());
    _builder.CreateBr (mergeBB, te->Start (), te->End ());

    _builder.SetInsertionPoint (mergeBB);
    auto  val = Value (ValueKind::Unknown, expectedType);
    auto *node
        = _builder.CreateLoadVar (tmp, expectedType, false, te->Start (), te->End ());
    auto res = SemanticResult (val, node);
    res      = implicitlyCast (res, &expectedType, te->Start (), te->End ());
    return res;
}

Sema::SemanticResult
Sema::analyzeCastExpr (CastExpr *ce, Type *expectedType) {
    auto  val  = analyzeExpr (ce->GetExpr (), nullptr);
    auto *type = ce->Type ();
    if (!val.Val.has_value () || type == nullptr) {
        return {};
    }
    resolveType (&val.Val->Type);
    resolveType (&type);

    Type *src = val.Val->Type;
    Type *dst = type;

    if (!canExplicitCast (src, dst)) {
        _diag
            .Report (
                DiagCode::ECannotCast,
                "cannot cast '" + typeToString (src) + "' to '" + typeToString (dst)
                    + "'",
                Severity::Error)
            .AddSpan (ce->Start (), ce->End ());

        return {};
    }

    if (*src == *dst) {
        return implicitlyCast (val, &expectedType, ce->Start (), ce->End ());
    }

    auto kind = hir::CastKind::Invalid;

    auto isIntegral
        = [] (Type *t) { return t->IsIntOrSize () || t->IsBool () || t->IsChar (); };

    if (isIntegral (src) && isIntegral (dst)) {
        kind = castIntegrals (src, dst);
    } else if (src->IsFloating () && dst->IsFloating ()) {
        kind = castFloats (src->AsFloating (), dst->AsFloating ());
    } else if (src->IsFloating () && isIntegral (dst)) {
        bool     dstUnsigned = false;
        unsigned dstWidth    = 0;

        getIntegralProps (dst, dstUnsigned, dstWidth);

        kind = dstUnsigned ? hir::CastKind::FPToUI : hir::CastKind::FPToSI;
    } else if (isIntegral (src) && dst->IsFloating ()) {
        bool     srcUnsigned = false;
        unsigned srcWidth    = 0;

        getIntegralProps (src, srcUnsigned, srcWidth);

        kind = srcUnsigned ? hir::CastKind::UIToFP : hir::CastKind::SIToFP;
    } else if (src->IsPointer () || dst->IsPointer ()) {
        kind = hir::CastKind::BitCast;
    }

    auto res = cast (kind, dst, val, ce->Start (), ce->End ());
    return implicitlyCast (res, &expectedType, ce->Start (), ce->End ());
}

void
Sema::getIntegralProps (Type *t, bool &isUnsigned, unsigned &width) const {
    if (t->IsInteger ()) {
        const auto *it = t->AsInteger ();
        isUnsigned     = it->IsUnsigned ();
        width          = it->BitWidth ();
    } else if (t->IsSize ()) {
        const auto *st = t->AsSize ();
        isUnsigned     = st->IsUnsigned ();
        width          = _ptrBitWidth;
    } else if (t->IsBool ()) {
        isUnsigned = false;
        width      = 1;
    } else if (t->IsChar ()) {
        isUnsigned = false;
        width      = 32;
    }
}

hir::CastKind
Sema::castIntegrals (Type *src, Type *dst) {
    bool     srcUnsigned = false;
    bool     dstUnsigned = false;
    unsigned srcWidth    = 0;
    unsigned dstWidth    = 0;

    getIntegralProps (src, srcUnsigned, srcWidth);
    getIntegralProps (dst, dstUnsigned, dstWidth);

    if (srcWidth < dstWidth) {
        return srcUnsigned ? hir::CastKind::ZExt : hir::CastKind::SExt;
    }
    if (srcWidth > dstWidth) {
        return hir::CastKind::Trunc;
    }
    return hir::CastKind::BitCast;
}

Sema::SemanticResult
Sema::analyzeRefExpr (RefExpr *re, Type *expectedType) {
    if (re->GetExpr ()->Kind () != NodeKind::VarExpr
        && re->GetExpr ()->Kind () != NodeKind::FieldExpr) {
        _diag
            .Report (
                DiagCode::ECannotTakeAddress,
                "cannot take the address of an rvalue",
                Severity::Error)
            .AddSpan (re->GetExpr ()->Start (), re->GetExpr ()->End ());
        return {};
    }
    auto expr = analyzeExpr (re->GetExpr (), nullptr);
    if (!expr.Val.has_value ()) {
        return {};
    }
    auto *ptrType = createType<PointerType> (expr.Val->Type);
    auto  val     = Value (ValueKind::Unknown, ptrType);
    auto *node = _builder.CreateReference (expr.Node, ptrType, re->Start (), re->End ());
    auto  res  = SemanticResult (val, node);
    res        = implicitlyCast (res, &expectedType, re->Start (), re->End ());
    return res;
}

Sema::SemanticResult
Sema::analyzeDerefExpr (DerefExpr *de, Type *expectedType) {
    auto expr = analyzeExpr (de->GetExpr (), nullptr);
    if (!expr.Val.has_value ()) {
        return {};
    }
    if (!expr.Val->Type->IsPointer ()) {
        _diag
            .Report (
                DiagCode::ECannotDereferenceNonPointer,
                "type '" + typeToString (expr.Val->Type) + "' cannot be dereferenced",
                Severity::Error)
            .AddSpan (de->GetExpr ()->Start (), de->GetExpr ()->End ());
        return {};
    }
    auto *ptrBaseType = expr.Val->Type->AsPointer ()->Base ();
    auto  val         = Value (ValueKind::Unknown, ptrBaseType);
    auto *node
        = _builder.CreateDereference (expr.Node, ptrBaseType, de->Start (), de->End ());
    auto res = SemanticResult (val, node);
    res      = implicitlyCast (res, &expectedType, de->Start (), de->End ());
    return res;
}

Sema::SemanticResult
Sema::analyzeNilExpr (NilExpr *ne, Type *expectedType) {
    if (expectedType == nullptr) {
        _diag
            .Report (
                DiagCode::ECannotInferType,
                "cannot infer pointer type for 'nil'",
                Severity::Error)
            .AddSpan (ne->Start (), ne->End ());
        return {};
    }
    if (!expectedType->IsPointer ()) {
        _diag
            .Report (
                DiagCode::ECannotAssignNilToNonPointer,
                "cannot assign 'nil' to a non-pointer type '"
                    + typeToString (expectedType) + "'",
                Severity::Error)
            .AddSpan (
                ne->Start (),
                ne->End (),
                "expected '" + typeToString (expectedType) + "', found 'nil'");
        return {};
    }
    auto  val  = Value (ValueKind::Unknown, expectedType);
    auto *node = _builder.CreateNil (ne->Start (), ne->End ());
    return { val, node };
}

Sema::SemanticResult
Sema::analyzeTypeExpr (TypeExpr *te, Type *expectedType) {
    auto *type = te->Type ();
    resolveType (&type);
    auto val = Value (ValueKind::Type, type);
    auto res = SemanticResult (val, nullptr);
    res      = implicitlyCast (res, &expectedType, te->Start (), te->End ());
    return res;
}
}
