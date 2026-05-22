#include "codegen/codegen.h"

#include <algorithm>
#include <ast/exprs/struct_instance.h>
#include <basic/types/all.h>
#include <basic/value.h>
#include <cstdint>
#include <diagnostic/codes.h>
#include <lexer/token_kind.h>
#include <sema/sema.h>
#include <sstream>

namespace veo {

using namespace symbols;
using namespace ast;

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
    default: return BinOp::Invalid;
    }
#undef variant
}

void
Sema::analyzeStmt (Stmt *stmt) {
#define variant(kind, func, type)                                                        \
    case NodeKind::kind: func (llvm::cast<type> (stmt)); break;
    switch (stmt->Kind ()) {
        variant (VarDef, analyzeVarDef, VarDef);
        variant (FuncDef, analyzeFuncDef, FuncDef);
        variant (Ret, analyzeRet, Return);
        variant (ExprStmt, analyzeExprStmt, ExprStmt);
        variant (IfElse, analyzeIfElseStmt, IfElseStmt);
        variant (ForLoop, analyzeForLoop, ForLoopStmt);
        variant (BreakContinue, analyzeBreakContinue, BreakContinue);
    default: break;
    }
#undef variant
}

void
Sema::analyzeVarDef (VarDef *vd) {
    if (auto it = _vars.top ().Vars.find (vd->Name ().Val);
        it != _vars.top ().Vars.end ()) {
        auto var = it->second;
        _diag
            .Report (
                DiagCode::ERedefinition,
                "variable '" + vd->Name ().Val + "' is already defined",
                Severity::Error)
            .AddSpan (var.Name.Start, var.Name.End, "previous definition was here", false)
            .AddSpan (vd->Name ().Start, vd->Name ().End, "redefined here");
        return;
    }
    bool  isGlobal = _vars.size () == 1;
    Type *type     = vd->Type ();
    resolveType (&type);
    auto val = analyzeExpr (vd->Init (), vd->Type ());
    if (val.Val.has_value ()) {
        if (isGlobal) {
            if (val.Val->Kind != ValueKind::Const) {
                _diag
                    .Report (
                        DiagCode::ECannotInitRuntimeVal,
                        "cannot initialize global variable with a runtime value",
                        Severity::Error)
                    .AddSpan (
                        vd->Init ()->Start (),
                        vd->Init ()->End (),
                        "evaluated at runtime")
                    .AddNote (
                        "global variables can only be initialized with compile-time "
                        "constants or other global variables");
                return;
            }
        }
        if (vd->IsConst ()) {
            if (val.Val->Kind != ValueKind::Const) {
                _diag
                    .Report (
                        DiagCode::ENotAConst,
                        "initializer element is not a constant expression",
                        Severity::Error)
                    .AddSpan (
                        vd->Init ()->Start (),
                        vd->Init ()->End (),
                        "non-constant expression");
                return;
            }
        }
    }
    if (type == nullptr) {
        if (val.Val.has_value ()) {
            type = val.Val->Type;
        } else {
            _diag
                .Report (
                    DiagCode::ECannotInferType,
                    "cannot infer type for variable '" + vd->Name ().Val + "'",
                    Severity::Error)
                .AddSpan (vd->Name ().Start, vd->Name ().End);
            return;
        }
    }
    auto var = Variable (
        vd->Name (),
        type,
        vd->IsConst (),
        isGlobal,
        isGlobal ? _vars.top ().Vars.size () : _localsCount++,
        _mod,
        val.Val);
    _vars.top ().Vars.emplace (var.Name.Val, var);
    if (isGlobal) {
        _mod->Vars.emplace (var.Name.Val, var);
    }
    _builder.CreateVariable (
        var.Name,
        type,
        val.Node,
        vd->IsConst (),
        isGlobal,
        vd->Start (),
        vd->End (),
        isGlobal ? &_mod->Vars.at (var.Name.Val) : nullptr);
}

void
Sema::declareFunc (ast::FuncDef *fd) {
    if (auto func = getFunction (fd->Name ().Val, fd->Args ()); func.has_value ()) {
        _diag
            .Report (
                DiagCode::ERedefinition,
                "funcion '" + fd->Name ().Val + "' is already defined",
                Severity::Error)
            .AddSpan (
                func->Name.Start,
                func->Name.End,
                "previous definition was here",
                false)
            .AddSpan (fd->Name ().Start, fd->Name ().End, "redefined here");
        return;
    }
    resolveType (&fd->RetType ());
    for (auto &arg : fd->Args ()) {
        resolveType (&arg.Type);
    }
    auto func = Function (fd->Name (), fd->RetType (), fd->Args (), _mod);
    auto it   = _mod->Funcs.find (func.Name.Val);
    if (it == _mod->Funcs.end ()) {
        _mod->Funcs.emplace (func.Name.Val, FunctionCandidates ());
    }
    _mod->Funcs.at (func.Name.Val)
        .Candidates.emplace_back (std::make_unique<Function> (func));
}

void
Sema::analyzeFuncDef (FuncDef *fd) {
    if (!inGlobalScope ()) {
        _diag
            .Report (
                DiagCode::ENotAllowedInThisScope,
                "statement not allowed in this scope",
                Severity::Error)
            .AddSpan (fd->Start (), fd->End (), "statement found inside of a function");
        return;
    }

    auto     *candidates = &_mod->Funcs.at (fd->Name ().Val);
    Function *func       = nullptr;
    for (auto &f : candidates->Candidates) {
        if (f->Args == fd->Args ()) {
            func = f.get ();
        }
    }

    auto *funcNode = _builder.CreateFunction (
        func->Name,
        fd->RetType (),
        fd->Args (),
        fd->Start (),
        fd->End (),
        func);
    auto *entry = _builder.CreateBasicBlock (funcNode, "entry");
    _builder.SetInsertionPoint (entry);

    _funcRetTypes.push (fd->RetType ());
    _vars.emplace ();

    _localsCount = 0;

    for (const auto &arg : fd->Args ()) {
        _vars.top ().Vars.emplace (
            arg.Name.Val,
            Variable (arg.Name, arg.Type, false, false, _localsCount++, nullptr));
    }

    for (const auto &stmt : fd->Body ()) {
        analyzeStmt (stmt);
    }

    _vars.pop ();
    _funcRetTypes.pop ();
}

void
Sema::analyzeRet (Return *ret) {
    if (inGlobalScope ()) {
        _diag
            .Report (
                DiagCode::ENotAllowedInThisScope,
                "statement not allowed in this scope",
                Severity::Error)
            .AddSpan (
                ret->Start (),
                ret->End (),
                "statement found outside of a function");
        return;
    }
    if (ret->RetExpr () == nullptr) {
        if (_funcRetTypes.top () != nullptr) {
            _diag
                .Report (
                    DiagCode::ECannotImplCast,
                    "cannot implicitly cast 'noth' to '"
                        + _funcRetTypes.top ()->ToString () + "'",
                    Severity::Error)
                .AddSpan (ret->Start (), ret->End ());
            return;
        }
        _builder.CreateRet (nullptr, ret->Start (), ret->End ());
    } else {
        auto res = analyzeExpr (ret->RetExpr (), nullptr);
        if (!res.Val.has_value ()) {
            return;
        }
        if (_funcRetTypes.top () == nullptr) {
            _diag
                .Report (
                    DiagCode::ECannotImplCast,
                    "cannot implicitly cast '" + res.Val->Type->ToString ()
                        + "' to 'noth'",
                    Severity::Error)
                .AddSpan (ret->RetExpr ()->Start (), ret->RetExpr ()->End ());
            return;
        }
        res = implicitlyCast (
            res,
            &_funcRetTypes.top (),
            ret->RetExpr ()->Start (),
            ret->RetExpr ()->End ());
        _builder.CreateRet (res.Node, ret->Start (), ret->End ());
    }
}

void
Sema::analyzeExprStmt (ast::ExprStmt *es) {
    if (inGlobalScope ()) {
        _diag
            .Report (
                DiagCode::ENotAllowedInThisScope,
                "statement not allowed in this scope",
                Severity::Error)
            .AddSpan (es->Start (), es->End (), "statement found outside of a function");
        return;
    }
    auto res = analyzeExpr (es->GetExpr (), nullptr);
    _builder.CreateExprStmt (res.Node, es->Start (), es->End ());
}

void
Sema::analyzeIfElseStmt (ast::IfElseStmt *ies) {
    if (inGlobalScope ()) {
        _diag
            .Report (
                DiagCode::ENotAllowedInThisScope,
                "statement not allowed in this scope",
                Severity::Error)
            .AddSpan (
                ies->Start (),
                ies->End (),
                "statement found outside of a function");
        return;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    auto  cond    = analyzeExpr (ies->Cond (), new BoolType ());
    auto *thenBB  = _builder.CreateBasicBlock (_builder.Parent (), "then");
    auto *elseBB  = _builder.CreateBasicBlock (_builder.Parent (), "else");
    auto *mergeBB = _builder.CreateBasicBlock (_builder.Parent (), "merge");

    _builder.CreateBr (
        cond.Node,
        thenBB,
        elseBB,
        ies->Cond ()->Start (),
        ies->Cond ()->End ());

    // then
    _builder.SetInsertionPoint (thenBB);
    _vars.emplace ();
    for (const auto &stmt : ies->Then ()) {
        analyzeStmt (stmt);
    }
    _vars.pop ();
    _builder.CreateBr (mergeBB, ies->Cond ()->Start (), ies->Cond ()->End ());

    // else
    _builder.SetInsertionPoint (elseBB);
    _vars.emplace ();
    for (const auto &stmt : ies->Else ()) {
        analyzeStmt (stmt);
    }
    _vars.pop ();
    _builder.CreateBr (mergeBB, ies->Cond ()->Start (), ies->Cond ()->End ());

    // merge
    _builder.SetInsertionPoint (mergeBB);
}

void
Sema::analyzeForLoop (ast::ForLoopStmt *fls) {
    if (inGlobalScope ()) {
        _diag
            .Report (
                DiagCode::ENotAllowedInThisScope,
                "statement not allowed in this scope",
                Severity::Error)
            .AddSpan (
                fls->Start (),
                fls->End (),
                "statement found outside of a function");
        return;
    }
    auto *indexatorBB = _builder.CreateBasicBlock (_builder.Parent (), "indexator");
    auto *condBB      = _builder.CreateBasicBlock (_builder.Parent (), "cond");
    auto *iterationBB = _builder.CreateBasicBlock (_builder.Parent (), "iteration");
    auto *bodyBB      = _builder.CreateBasicBlock (_builder.Parent (), "body");
    auto *mergeBB     = _builder.CreateBasicBlock (_builder.Parent (), "merge");
    _builder.CreateBr (indexatorBB, fls->Start (), fls->End ());

    _vars.emplace ();
    _loops.emplace (mergeBB, iterationBB);

    // indexator
    _builder.SetInsertionPoint (indexatorBB);
    if (fls->Indexator () != nullptr) {
        analyzeStmt (fls->Indexator ());
    }
    _builder.CreateBr (condBB, fls->Start (), fls->End ());

    // cond
    _builder.SetInsertionPoint (condBB);
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    auto          *boolType = new BoolType ();
    SemanticResult cond;
    if (fls->Cond () != nullptr) {
        cond = analyzeExpr (fls->Cond (), boolType);
        if (!cond.Val.has_value ()) {
            return;
        }
    } else {
        auto val = Value (ValueKind::Const, ValueData (1), boolType);
        cond     = { val, _builder.CreateLiteral (val, fls->Start (), fls->End ()) };
    }
    _builder.CreateBr (cond.Node, bodyBB, mergeBB, fls->Start (), fls->End ());

    // iteration
    _builder.SetInsertionPoint (iterationBB);
    if (fls->Iteration () != nullptr) {
        analyzeStmt (fls->Iteration ());
    }
    _builder.CreateBr (condBB, fls->Start (), fls->End ());

    // body
    _builder.SetInsertionPoint (bodyBB);
    for (const auto &stmt : fls->Body ()) {
        analyzeStmt (stmt);
    }
    _builder.CreateBr (iterationBB, fls->Start (), fls->End ());

    // merge
    _builder.SetInsertionPoint (mergeBB);

    _loops.pop ();
    _vars.pop ();
}

void
Sema::analyzeBreakContinue (BreakContinue *bc) {
    if (inGlobalScope ()) {
        _diag
            .Report (
                DiagCode::ENotAllowedInThisScope,
                "statement not allowed in this scope",
                Severity::Error)
            .AddSpan (bc->Start (), bc->End (), "statement found outside of a function");
        return;
    }
    if (bc->GetKind () == BreakContinue::Kind::Break) {
        _builder.CreateBr (_loops.top ().Break, bc->Start (), bc->End ());
    } else {
        _builder.CreateBr (_loops.top ().Continue, bc->Start (), bc->End ());
    }
}

void
Sema::analyzeStructDef (StructDef *sd) {
    if (auto it = _mod->Structs.find (sd->Name ().Val); it != _mod->Structs.end ()) {
        auto &s = it->second; // struct
        _diag
            .Report (
                DiagCode::ERedefinition,
                "struct '" + sd->Name ().Val + "' is already defined",
                Severity::Error)
            .AddSpan (s.Name.Start, s.Name.End, "previous definition was here", false)
            .AddSpan (sd->Name ().Start, sd->Name ().End, "redefined here");
        return;
    }

    std::vector<symbols::Field> fields;
    fields.reserve (sd->Fields ().size ());
    std::vector<hir::Field> hirFields;
    hirFields.reserve (sd->Fields ().size ());
    size_t index = 0;
    for (auto &field : sd->Fields ()) {
        resolveType (&field.Type);
        fields.emplace_back (
            field.Name,
            field.Type,
            field.IsStatic,
            field.IsConst,
            field.Access,
            index);
        hirFields.emplace_back (field.Name, field.Type, field.IsStatic, field.IsConst);
        if (!field.IsStatic) {
            ++index;
        }
    }
    auto s = Struct (sd->Name (), fields, _mod);
    _mod->Structs.emplace (sd->Name ().Val, s);
    _builder.CreateStruct (
        sd->Name (),
        hirFields,
        &_mod->Structs.at (sd->Name ().Val),
        sd->Start (),
        sd->End ());
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
    default: return {};
    }
#undef variant
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
            expectedType
                = new IntegerType (32); // NOLINT(cppcoreguidelines-avoid-magic-numbers)
        }
        if (!expectedType->IsNumber ()) {
            _diag
                .Report (
                    DiagCode::ECannotImplCast,
                    "cannot implicitly cast 'i32' to '" + expectedType->ToString () + "'",
                    Severity::Error)
                .AddSpan (le->Start (), le->End ());
            return {};
        }
        int64_t ival  = std::stoll (val);
        auto    value = Value (
            ValueKind::Const,
            expectedType->IsInteger () ? ValueData (ival)
                                       : ValueData (static_cast<double> (ival)),
            expectedType);
        if (expectedType->IsInteger () && !canFit (value, expectedType)) {
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

    auto *node = _builder.CreateUnary (op, resType, rhs.Node, ue->Start (), ue->End ());
    auto  res  = SemanticResult (Value (ValueKind::Unknown, resType), node);

    if (expectedType != nullptr) {
        res = implicitlyCast (res, &expectedType, ue->Start (), ue->End ());
    }

    return res;
}

// NOLINTBEGIN(readability-convert-member-functions-to-static)
Sema::SemanticResult
Sema::analyzeVarExpr (VarExpr *ve, Type *expectedType) {
    auto var = getVariable (ve->Name ().Val);
    if (!var.has_value ()) {
        if (auto *s = getStruct (ve->Name ().Val)) {
            return { Value (ValueKind::Type, new StructType (s)), nullptr };
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
    if (var->IsConst) {
        node = _builder.CreateLiteral (value, ve->Start (), ve->End ());
    } else {
        node = _builder.CreateLoadVar (
            var->Index,
            var->Type,
            var->IsGlobal,
            ve->Start (),
            ve->End ());
    }
    auto res = SemanticResult (value, node);
    if (expectedType != nullptr) {
        res = implicitlyCast (res, &expectedType, ve->Start (), ve->End ());
    }
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
        argResults.emplace_back (argRes);
        argTypes.emplace_back (argRes.Val->Type);
    }

    Function *func = resolveBestOverload (candidates, argTypes, fc->Start (), fc->End ());
    if (func == nullptr) {
        return {};
    }

    std::vector<hir::Node *> hirArgs;
    for (size_t i = 0; i < argResults.size (); ++i) {
        auto res = analyzeExpr (fc->Args ()[i], func->Args[i].Type);
        hirArgs.emplace_back (res.Node);
    }

    return { Value (ValueKind::Unknown, func->RetType),
             _builder.CreateCall (func, std::move (hirArgs), fc->Start (), fc->End ()) };
}

// NOLINTBEGIN(readability-function-cognitive-complexity)
Sema::SemanticResult
Sema::analyzeAsgnExpr (AsgnExpr *ae, Type *expectedType) {
    if (ae->Ptr () == nullptr) {
        return {};
    }
    if (ae->Ptr ()->Kind () != NodeKind::VarExpr
        && ae->Ptr ()->Kind () != NodeKind::FieldExpr) {
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
                    + "' with type '" + ptr.Val->Type->ToString () + "'",
                Severity::Error)
            .AddSpan (ae->Ptr ()->Start (), ae->Ptr ()->End ());
        return {};
    }
    if (ae->Ptr ()->Kind () == NodeKind::VarExpr) {
        auto var = getVariable (llvm::cast<VarExpr> (ae->Ptr ())->Name ().Val);
        auto op  = AsgnOpToBinOp (ae->Op ());
        if (ae->Op () != AsgnOp::Eq) {
            expr.Node = _builder.CreateBinary (
                op,
                expr.Val->Type,
                ptr.Node,
                expr.Node,
                ae->Init ()->Start (),
                ae->Init ()->End ());
        }
        if (var->IsConst) {
            _diag
                .Report (
                    DiagCode::ECannotModifyConst,
                    "cannot assign to a constant variable '" + var->Name.Val + "'",
                    Severity::Error)
                .AddSpan (ae->Ptr ()->Start (), ae->Ptr ()->End ());
            return {};
        }
        if (expr.Val->Kind == ValueKind::Const) {
            expr.Val = foldBinary (
                op,
                *var->Val,
                *expr.Val,
                expr.Val->Type,
                ae->Init ()->Start (),
                ae->Init ()->End ());
        } else {
            expr.Val = Value (ValueKind::Unknown, expr.Val->Type);
        }
    } else { // only field
        auto *fieldExpr = llvm::cast<FieldExpr> (ae->Ptr ());
        auto  base      = analyzeExpr (fieldExpr->Base (), nullptr);
        if (!base.Val.has_value ()) {
            return {};
        }
        if (!base.Val->Type->IsStruct ()) {
            _diag
                .Report (
                    DiagCode::ECannotAccessFromNonStruct,
                    "attempted to access a field on a non-structure type '"
                        + base.Val->Type->ToString () + "'",
                    Severity::Error)
                .AddSpan (fieldExpr->Name ().Start, fieldExpr->Name ().End);
            return {};
        }
        auto *sym          = base.Val->Type->AsStruct ()->BaseSymbol ();
        bool  baseIsStatic = base.Val->Kind == ValueKind::Type;
        auto  field
            = std::ranges::find_if (sym->Fields, [&] (const symbols::Field &field) {
                  return field.Name.Val == fieldExpr->Name ().Val;
              });
        if (field == sym->Fields.end ()) {
            return {};
        }
        if (field->IsStatic && !baseIsStatic) {
            _diag
                .Report (
                    DiagCode::ECannotAccessStaticMemberFromInstance,
                    "static field '" + fieldExpr->Name ().Val
                        + "' cannot be accessed through an instance",
                    Severity::Error)
                .AddSpan (fieldExpr->Name ().Start, fieldExpr->Name ().End)
                .AddNote (
                    "use the type name instead to access this field: '" + sym->Name.Val
                    + "." + fieldExpr->Name ().Val + "'");
            return {};
        }
        if (!field->IsStatic && baseIsStatic) {
            _diag
                .Report (
                    DiagCode::ECannotAccessNonStaticMemberFromType,
                    "instance field '" + fieldExpr->Name ().Val
                        + "' cannot be accessed through a type",
                    Severity::Error)
                .AddSpan (fieldExpr->Name ().Start, fieldExpr->Name ().End);
            return {};
        }
        if (field->IsConst) {
            _diag
                .Report (
                    DiagCode::ECannotModifyConst,
                    "cannot assign to a constant field '" + field->Name.Val
                        + "' in struct '" + sym->Name.Val + "'",
                    Severity::Error)
                .AddSpan (ae->Ptr ()->Start (), ae->Ptr ()->End ());
            return {};
        }
        auto op = AsgnOpToBinOp (ae->Op ());
        if (ae->Op () != AsgnOp::Eq) {
            expr.Node = _builder.CreateBinary (
                op,
                expr.Val->Type,
                ptr.Node,
                expr.Node,
                ae->Init ()->Start (),
                ae->Init ()->End ());
        }
        expr.Val = Value (ValueKind::Unknown, expr.Val->Type);
    }
    expr       = implicitlyCast (expr, &expectedType, ae->Start (), ae->End ());
    auto *node = _builder.CreateStore (ptr.Node, expr.Node, ae->Start (), ae->End ());
    return { expr.Val, node };
}
// NOLINTEND(readability-function-cognitive-complexity)

Sema::SemanticResult
Sema::analyzeFieldExpr (FieldExpr *fe, Type *expectedType) {
    auto base = analyzeExpr (fe->Base (), nullptr);
    if (!base.Val.has_value ()) {
        return {};
    }
    if (!base.Val->Type->IsStruct ()) {
        _diag
            .Report (
                DiagCode::ECannotAccessFromNonStruct,
                "attempted to access a field on a non-structure type '"
                    + base.Val->Type->ToString () + "'",
                Severity::Error)
            .AddSpan (fe->Name ().Start, fe->Name ().End);
        return {};
    }
    auto *s            = base.Val->Type->AsStruct ()->BaseSymbol ();
    bool  baseIsStatic = base.Val->Kind == ValueKind::Type;
    auto  it = std::ranges::find_if (s->Fields, [&] (const symbols::Field &field) {
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
    if (it->IsStatic && !baseIsStatic) {
        _diag
            .Report (
                DiagCode::ECannotAccessStaticMemberFromInstance,
                "static field '" + fe->Name ().Val
                    + "' cannot be accessed through an instance",
                Severity::Error)
            .AddSpan (fe->Name ().Start, fe->Name ().End)
            .AddNote (
                "use the type name instead to access this field: '" + s->Name.Val + "."
                + fe->Name ().Val + "'");
        return {};
    }
    if (!it->IsStatic && baseIsStatic) {
        _diag
            .Report (
                DiagCode::ECannotAccessNonStaticMemberFromType,
                "instance field '" + fe->Name ().Val
                    + "' cannot be accessed through a type",
                Severity::Error)
            .AddSpan (fe->Name ().Start, fe->Name ().End);
        return {};
    }
    if (it->Access != AccessModifier::Pub) {
        _diag
            .Report (
                DiagCode::ECannotAccessToPrivMember,
                "field '" + fe->Name ().Val + "' of struct '" + s->Name.Val
                    + "' is private",
                Severity::Error)
            .AddSpan (fe->Name ().Start, fe->Name ().End);
        return {};
    }
    hir::Node *node = nullptr;
    if (it->IsStatic) {
        node = _builder.CreateLoadGlobalVarByName (
            CodeGen::MangleStaticField (s, it->Name.Val),
            it->Type,
            fe->Start (),
            fe->End ());
    } else {
        node = _builder.CreateFieldExpr (base.Node, it->Index, fe->Start (), fe->End ());
    }
    auto val = Value (ValueKind::Unknown, it->Type);
    auto res = SemanticResult (val, node);
    res      = implicitlyCast (res, &expectedType, fe->Start (), fe->End ());
    return res;
}

Sema::SemanticResult
Sema::analyzeStructInstance (StructInstance *si, Type *expectedType) {
    auto it = _mod->Structs.find (si->Path ().Val);
    if (it == _mod->Structs.end ()) {
        _diag
            .Report (
                DiagCode::EUndefined,
                "struct '" + si->Path ().Val + "' is not defined",
                Severity::Error)
            .AddSpan (si->Path ().Start, si->Path ().End);
        return {};
    }
    auto                                       *s = &it->second;
    std::vector<std::pair<size_t, hir::Node *>> fields;
    std::unordered_map<size_t, NameObj>         initializedFields;
    ValueKind                                   valKind = ValueKind::Const;
    for (const auto &field : s->Fields) {
        if (field.Access != AccessModifier::Pub) {
            _diag
                .Report (
                    DiagCode::ECannotInitStructWithPrivFields,
                    "cannot structurally initialize '" + s->Name.Val
                        + "' because it contains private "
                          "fields",
                    Severity::Error)
                .AddSpan (si->Path ().Start, si->Path ().End)
                .AddNote (
                    "use the constructor function '" + s->Name.Val
                    + ".New()' instead if available");
            return {};
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
            return {};
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
            return {};
        }
        if (it->IsConst) {
            _diag
                .Report (
                    DiagCode::ECannotInitConstField,
                    "cannot initialize constant field '" + name.Val + "'",
                    Severity::Error)
                .AddSpan (name.Start, name.End);
            return {};
        }
        if (it->IsStatic) {
            _diag
                .Report (
                    DiagCode::ECannotInitStaticField,
                    "cannot initialize static field '" + name.Val + "'",
                    Severity::Error)
                .AddSpan (name.Start, name.End);
            return {};
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
    auto *node
        = _builder.CreateStructInstance (std::move (fields), s, si->Start (), si->End ());
    auto val = Value (valKind, new StructType (s));
    auto res = SemanticResult (val, node);
    res      = implicitlyCast (res, &expectedType, si->Start (), si->End ());
    return res;
}

Type *
Sema::resolveType (Type **type) {
    if (*type == nullptr) {
        return *type;
    }
    if (!(*type)->IsNamed ()) {
        return *type;
    }

    const auto *named  = (*type)->AsNamed ();
    Module     *curMod = _mod;
    const auto &path   = named->Path ();
    for (size_t i = 0; i < path.size () - 1; ++i) {
        const auto &name = path[i];
        if (auto it = curMod->Submods.find (name.Val); it != curMod->Submods.end ()) {
            curMod = it->second;
        } else if (auto it = curMod->Imports.find (name.Val);
                   it != curMod->Imports.end ()) {
            curMod = it->second;
        } else {
            _diag
                .Report (
                    DiagCode::ECannotFindMod,
                    "cannot find module '" + name.Val + "'",
                    Severity::Error)
                .AddSpan (name.Start, name.End, "no such module found");
            return nullptr;
        }
    }
    const auto &typeName = path.back ();
    if (!curMod->Structs.contains (typeName.Val)) {
        _diag
            .Report (
                DiagCode::ECannotFindType,
                "cannot find type '" + typeName.Val + "' in "
                    + (path.size () == 1 ? "this scope" : curMod->ToString ()),
                Severity::Error)
            .AddSpan (typeName.Start, typeName.End, "not found");
        return nullptr;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    *type = new StructType (&curMod->Structs.at (typeName.Val));
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

symbols::Struct *
Sema::getStruct (const std::string &name, symbols::Module *mod) {
    if (mod == nullptr) {
        mod = _mod;
    }

    if (auto it = mod->Structs.find (name); it != mod->Structs.end ()) {
        return &it->second;
    }
    return nullptr;
}

std::optional<Function>
Sema::getFunction (const std::string &name, const std::vector<Argument> &args) {
    if (!_mod->Funcs.contains (name)) {
        return std::nullopt;
    }
    const auto &candidates = _mod->Funcs.at (name);
    for (const auto &f : candidates.Candidates) {
        unsigned coincidences = 0;
        if (f->Args.size () == args.size ()) {
            for (size_t i = 0; i < args.size (); ++i) {
                if (*f->Args[i].Type == *args[i].Type) {
                    ++coincidences;
                }
            }
        } else {
            continue;
        }

        if (coincidences == args.size ()) {
            return *f;
        }
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
        // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
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
    // NOLINTEND(cppcoreguidelines-avoid-magic-numbers)
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
    if (!val.Val.has_value () || *expectedType == nullptr) {
        return val;
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

// NOLINTBEGIN(readability-function-cognitive-complexity)
Function *
Sema::resolveBestOverload (
    FunctionCandidates        *candidates,
    const std::vector<Type *> &argTypes,
    llvm::SMLoc                start,
    llvm::SMLoc                end) {
    std::vector<std::pair<Function *, int>> viableCandidates;

    for (auto &cand : candidates->Candidates) {
        if (cand->Args.size () != argTypes.size ()) {
            continue;
        }

        bool   viable  = true;
        size_t costSum = 0;
        for (size_t i = 0; i < argTypes.size (); ++i) {
            CastCost cost = checkCastCost (argTypes[i], cand->Args[i].Type);
            if (cost == CastCost::Incompatible) {
                viable = false;
                break;
            }
            costSum += static_cast<size_t> (cost);
        }

        if (viable) {
            viableCandidates.emplace_back (cand.get (), costSum);
        }
    }

    if (viableCandidates.empty ()) {
        std::vector<std::string> foundCandidates;
        for (auto &func : candidates->Candidates) {
            std::ostringstream oss;
            oss << "func " << func->Name.Val << "(";
            size_t i = 0;
            for (const auto &a : func->Args) {
                oss << a.Type->ToString ();
                if (i < func->Args.size () - 1) {
                    oss << ", ";
                }
                ++i;
            }
            oss << "): " << func->RetType->ToString ();
            foundCandidates.emplace_back (oss.str ());
        }
        _diag
            .Report (
                DiagCode::ENoMatchingFunction,
                "no matching function for call to '" + candidates->Candidates[0]->Name.Val
                    + "'",
                Severity::Error)
            .AddSpan (start, end)
            .AddNote ("candidate functions found:", std::move (foundCandidates));
        return nullptr;
    }

    Function *bestCand    = viableCandidates[0].first;
    size_t    minCost     = viableCandidates[0].second;
    bool      isAmbiguous = false;

    for (size_t i = 1; i < viableCandidates.size (); ++i) {
        if (viableCandidates[i].second < minCost) {
            minCost     = viableCandidates[i].second;
            bestCand    = viableCandidates[i].first;
            isAmbiguous = false;
        } else if (viableCandidates[i].second == minCost) {
            isAmbiguous = true;
        }
    }

    if (isAmbiguous) {
        auto &err = _diag.Report (
            DiagCode::ECallIsAmbiguous,
            "call to '' is ambiguous",
            Severity::Error);
        size_t i = 0;
        for (const auto &[c, _] : viableCandidates) {
            err.AddSpan (
                c->Name.Start,
                c->Name.End,
                "candidate " + std::to_string (i),
                false);
            ++i;
        }
        return nullptr;
    }

    return bestCand;
}
// NOLINTEND(readability-function-cognitive-complexity)

Sema::CastCost
Sema::checkCastCost (Type *src, Type *dst) {
    if (*src == *dst) {
        return CastCost::Exact;
    }

    if (src->IsInteger () && dst->IsInteger ()) {
        const auto *srcI = src->AsInteger ();
        const auto *dstI = dst->AsInteger ();

        if (dstI->BitWidth () >= srcI->BitWidth ()) {
            if (srcI->IsUnsigned () == dstI->IsUnsigned ()
                && dstI->BitWidth () == srcI->BitWidth ()) {
                return CastCost::Exact;
            }
            if (srcI->IsUnsigned () == dstI->IsUnsigned ()
                || dstI->BitWidth () > srcI->BitWidth ()) {
                return CastCost::SafeImplicit;
            }
        }
    }

    if (src->IsInteger () && dst->IsFloating ()) {
        return CastCost::SafeImplicit;
    }

    if (src->IsFloating () && dst->IsFloating ()) {
        const auto *srcF = src->AsFloating ();
        const auto *dstF = dst->AsFloating ();

        if (dstF->IsDouble () && srcF->IsFloat ()) {
            return CastCost::SafeImplicit;
        }
        if (dstF->IsFloat () == srcF->IsFloat ()
            || dstF->IsDouble () == srcF->IsDouble ()) {
            return CastCost::Exact;
        }
    }

    return CastCost::Incompatible;
}

bool
Sema::canApplyAsgnOp (ast::AsgnOp op, Type *type) {
    if (op == AsgnOp::Eq) {
        return true;
    }
    if (type->IsNumber ()) {
        return true;
    }
    return false;
}

bool
Sema::inGlobalScope () const {
    return _vars.size () == 1;
}

}
