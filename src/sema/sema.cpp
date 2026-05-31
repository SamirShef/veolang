#include <algorithm>
#include <ast/exprs/struct_instance.h>
#include <basic/types/all.h>
#include <basic/value.h>
#include <codegen/mangler.h>
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
        variant (ImplStmt, analyzeImplStmt, ImplStmt);
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
    if (!val.Val.has_value () && vd->Init () != nullptr) {
        return;
    }
    if (val.Val.has_value ()) {
        if (vd->IsConst () && val.Val->Kind != ValueKind::Const) {
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
    if (!allowInScope (fd)) {
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
    if (!allowInScope (ret, false)) {
        return;
    }
    if (ret->RetExpr () == nullptr) {
        if (_funcRetTypes.top () != nullptr) {
            _diag
                .Report (
                    DiagCode::ECannotCast,
                    "cannot implicitly cast 'noth' to '"
                        + typeToString (_funcRetTypes.top ()) + "'",
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
                    DiagCode::ECannotCast,
                    "cannot implicitly cast '" + typeToString (res.Val->Type)
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
    if (!allowInScope (es, false)) {
        return;
    }
    auto res = analyzeExpr (es->GetExpr (), nullptr);
    _builder.CreateExprStmt (res.Node, es->Start (), es->End ());
}

void
Sema::analyzeIfElseStmt (ast::IfElseStmt *ies) {
    if (!allowInScope (ies, false)) {
        return;
    }
    auto  cond    = analyzeExpr (ies->Cond (), createType<BoolType> ());
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
    if (!allowInScope (fls, false)) {
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
    auto          *boolType = createType<BoolType> ();
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
    if (!allowInScope (bc, false)) {
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
    if (!allowInScope (sd)) {
        return;
    }
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
        auto it = std::ranges::find_if (fields, [&] (const symbols::Field &f) {
            return field.Name.Val == f.Name.Val;
        });
        if (it != fields.end ()) {
            _diag
                .Report (
                    DiagCode::ERedefinition,
                    "field '" + field.Name.Val + "' is already defined",
                    Severity::Error)
                .AddSpan (
                    it->Name.Start,
                    it->Name.End,
                    "previous definition was here",
                    false)
                .AddSpan (field.Name.Start, field.Name.End, "redefined here");
            continue;
        }
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
    _mod->Structs.emplace (sd->Name ().Val, std::move (s));
    _builder.CreateStruct (
        sd->Name (),
        hirFields,
        &_mod->Structs.at (sd->Name ().Val),
        sd->Start (),
        sd->End ());
}

void
Sema::declareImplMethods (ast::ImplStmt *is) {
    if (!allowInScope (is)) {
        return;
    }
    resolveType (&is->StructType ());
    if (is->StructType () == nullptr) {
        return;
    }
    if (!is->StructType ()->IsStruct ()) {
        _diag
            .Report (
                DiagCode::ECannotImplForNonStructType,
                "cannot implement methods for non-structure type '"
                    + typeToString (is->StructType ()) + "'",
                Severity::Error)
            .AddSpan (is->Start (), is->End ());
        return;
    }
    auto *sym = is->StructType ()->AsStruct ()->BaseSymbol ();
    for (const auto &method : is->Methods ()) {
        auto *fd = method.Func;
        if (auto *m = getMethod (sym, fd->Name ().Val, fd->Args ()); m != nullptr) {
            _diag
                .Report (
                    DiagCode::ERedefinition,
                    "method '" + fd->Name ().Val + "' is already defined in struct '"
                        + sym->Name.Val + "'",
                    Severity::Error)
                .AddSpan (
                    m->Func->Name.Start,
                    m->Func->Name.End,
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
        auto m    = symbols::Method (
            std::make_unique<Function> (std::move (func)),
            fd->Access (),
            method.IsStatic);
        auto it = sym->Methods.find (m.Func->Name.Val);
        if (it == sym->Methods.end ()) {
            sym->Methods.emplace (m.Func->Name.Val, MethodCandidates ());
        }
        sym->Methods.at (m.Func->Name.Val)
            .Candidates.emplace_back (std::make_unique<symbols::Method> (std::move (m)));
    }
}

void
Sema::analyzeImplStmt (ImplStmt *is) {
    if (!is->StructType ()->IsStruct ()) {
        return;
    }
    auto *sym = is->StructType ()->AsStruct ()->BaseSymbol ();
    for (const auto &method : is->Methods ()) {
        auto            *fd         = method.Func;
        auto            *candidates = &sym->Methods.at (fd->Name ().Val);
        symbols::Method *m          = nullptr;
        for (auto &f : candidates->Candidates) {
            if (f->Func->Args == fd->Args ()) {
                m = f.get ();
            }
        }

        std::vector<Argument> args;
        args.reserve (fd->Args ().size () + (method.IsStatic ? 1 : 0));
        if (!method.IsStatic) {
            args.emplace_back (
                basic::NameObj ("this", fd->Name ().Start, fd->Name ().End),
                createType<PointerType> (is->StructType ()));
        }
        for (const auto &arg : fd->Args ()) {
            args.emplace_back (arg);
        }
        auto *methodNode = _builder.CreateMethod (
            m->Func->Name,
            fd->RetType (),
            args,
            fd->Start (),
            fd->End (),
            m,
            createType<StructType> (sym),
            method.IsStatic);
        auto *entry = _builder.CreateBasicBlock (methodNode, "entry");
        _builder.SetInsertionPoint (entry);

        _insideMethod = { m, sym };
        _funcRetTypes.push (fd->RetType ());
        _vars.emplace ();

        _localsCount = 0;

        for (const auto &arg : args) {
            _vars.top ().Vars.emplace (
                arg.Name.Val,
                Variable (arg.Name, arg.Type, false, false, _localsCount++, nullptr));
        }

        for (const auto &stmt : fd->Body ()) {
            analyzeStmt (stmt);
        }

        _vars.pop ();
        _funcRetTypes.pop ();
        _insideMethod = std::nullopt;
    }
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
    default: return {};
    }
#undef variant
}

Sema::SemanticResult
Sema::analyzeLiteralExpr (LiteralExpr *le, Type *expectedType) {
    std::string val = le->Value ();
    switch (le->Kind ()) {
#define int_lit(kind, bitWidth, isUnsigned, min, max)                                    \
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
        int64_t ival  = std::stoll (val, nullptr, base);                                 \
        auto    value = Value (                                                          \
            ValueKind::Const,                                                            \
            ValueData (ival),                                                            \
            createType<IntegerType> (bitWidth, isUnsigned));                             \
        auto res = SemanticResult (                                                      \
            value,                                                                       \
            _builder.CreateLiteral (value, le->Start (), le->End ()));                   \
        if (ival < (min) || ival > (max)) {                                              \
            _diag                                                                        \
                .Report (                                                                \
                    DiagCode::ELitOutOfRange,                                            \
                    "literal out of range for '" + typeToString (res.Val->Type) + "'",   \
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
            createType<FloatingType> (FloatingKind::floatingKind));                      \
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
            expectedType = createType<IntegerType> (
                32); // NOLINT(cppcoreguidelines-avoid-magic-numbers)
        }
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
        int64_t ival  = std::stoll (val, nullptr, base);
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
                    "literal out of range for '" + typeToString (value.Type) + "'",
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
        auto value = Value (ValueKind::Const, ValueData (bval), createType<BoolType> ());
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
        int64_t cval = val.size () == 1 ? val[0] : 0;
        auto value = Value (ValueKind::Const, ValueData (cval), createType<CharType> ());
        auto res   = SemanticResult (
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

Sema::SemanticResult
Sema::analyzeBinaryExpr (BinaryExpr *be, Type *expectedType) {
    auto lhs = analyzeExpr (be->Lhs (), nullptr);
    auto rhs = analyzeExpr (be->Rhs (), nullptr);
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
                    "unary operator '" + std::string (UnOpToString (op))
                        + "' cannot be applied to type '" + typeToString (rhs.Val->Type)
                        + "'",
                    Severity::Error)
                .AddSpan (ue->Start (), ue->End ());
            return {};
        }
        resType = rhs.Val->Type;
        break;

    case UnOp::Not:
        if (!rhs.Val->Type->IsBool ()) {
            Type *boolType = createType<BoolType> ();
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
        if (expectedType != nullptr) {
            res = implicitlyCast (res, &expectedType, ue->Start (), ue->End ());
        }
        if (op == UnOp::Inverse) {
            auto iVal       = std::get<0> (rhs.Val->Data);
            bool isUnsigned = resType->AsInteger ()->IsUnsigned ();
            res.Val         = Value (
                ValueKind::Const,
                ValueData (
                    isUnsigned ? static_cast<int64_t> (~static_cast<uint64_t> (iVal))
                               : ~iVal), // NOLINT(hicpp-signed-bitwise)
                resType);
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

    auto res = SemanticResult (
        Value (ValueKind::Unknown, func->RetType),
        _builder.CreateCall (func, std::move (hirArgs), fc->Start (), fc->End ()));
    res = implicitlyCast (res, &expectedType, fc->Start (), fc->End ());
    return res;
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
    auto *node = _builder.CreateStore (ptr.Node, expr.Node, ae->Start (), ae->End ());
    return { expr.Val, node };
}

Sema::SemanticResult
Sema::analyzeAsgnVar (
    AsgnExpr *ae, Sema::SemanticResult &expr, const Sema::SemanticResult &ptr) {
    auto var = getVariable (llvm::cast<VarExpr> (ae->Ptr ())->Name ().Val);
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
    while (base.Val->Type->IsPointer ()) {
        base.Val->Type = base.Val->Type->AsPointer ()->Base ();
        base.Node      = _builder.CreateDereference (
            base.Node,
            base.Val->Type,
            fieldExpr->Start (),
            fieldExpr->End ());
    }
    if (!base.Val->Type->IsStruct ()) {
        _diag
            .Report (
                DiagCode::ECannotAccessFromNonStruct,
                "attempted to access a field on a non-structure type '"
                    + typeToString (base.Val->Type) + "'",
                Severity::Error)
            .AddSpan (fieldExpr->Name ().Start, fieldExpr->Name ().End);
        return {};
    }
    auto *sym          = base.Val->Type->AsStruct ()->BaseSymbol ();
    bool  baseIsStatic = base.Val->Kind == ValueKind::Type;
    bool  baseIsThis
        = !baseIsStatic && _insideMethod.has_value () && *_insideMethod->second == *sym;
    bool canAccessPrivate
        = baseIsThis
          || baseIsStatic && _insideMethod.has_value () && *_insideMethod->second == *sym;
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
    if (_insideMethod.has_value () && *_insideMethod->second == *sym) {
        _insideMethod->first->IsConst = false;
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
    return expr;
}

Sema::SemanticResult
Sema::analyzeAsgnPtr (
    ast::AsgnExpr *ae, SemanticResult &expr, const SemanticResult &ptr) {
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
    return expr;
}

Sema::SemanticResult
Sema::analyzeFieldExpr (FieldExpr *fe, Type *expectedType) {
    auto base = analyzeExpr (fe->Base (), nullptr);
    if (!base.Val.has_value ()) {
        return {};
    }
    while (base.Val->Type->IsPointer ()) {
        base.Val->Type = base.Val->Type->AsPointer ()->Base ();
        base.Node      = _builder.CreateDereference (
            base.Node,
            base.Val->Type,
            fe->Start (),
            fe->End ());
    }
    if (!base.Val->Type->IsStruct ()) {
        _diag
            .Report (
                DiagCode::ECannotAccessFromNonStruct,
                "attempted to access a field on a non-structure type '"
                    + typeToString (base.Val->Type) + "'",
                Severity::Error)
            .AddSpan (fe->Name ().Start, fe->Name ().End);
        return {};
    }
    auto *s            = base.Val->Type->AsStruct ()->BaseSymbol ();
    bool  baseIsStatic = base.Val->Kind == ValueKind::Type;
    bool  baseIsThis
        = !baseIsStatic && _insideMethod.has_value () && *_insideMethod->second == *s;
    bool canAccessPrivate
        = baseIsThis
          || baseIsStatic && _insideMethod.has_value () && *_insideMethod->second == *s;
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
    hir::Node *node = nullptr;
    if (it->IsStatic) {
        node = _builder.CreateLoadGlobalVarByName (
            Mangler::MangleStaticField (s, it->Name.Val),
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
    bool                                        checkAccessModifiers
        = !_insideMethod.has_value () || *_insideMethod->second != *s;
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
                    .AddSpan (si->Path ().Start, si->Path ().End)
                    .AddNote (
                        "use the constructor function '" + s->Name.Val
                        + ".New()' instead if available");
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
    auto *node
        = _builder.CreateStructInstance (std::move (fields), s, si->Start (), si->End ());
    auto val = Value (valKind, createType<StructType> (s));
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
    while (base.Val->Type->IsPointer ()) {
        base.Val->Type = base.Val->Type->AsPointer ()->Base ();
        base.Node      = _builder.CreateDereference (
            base.Node,
            base.Val->Type,
            mc->Start (),
            mc->End ());
    }
    if (!base.Val->Type->IsStruct ()) {
        _diag
            .Report (
                DiagCode::ECannotAccessFromNonStruct,
                "attempted to access a method on a non-structure type '"
                    + typeToString (base.Val->Type) + "'",
                Severity::Error)
            .AddSpan (mc->Name ().Start, mc->Name ().End);
        return {};
    }
    auto *s            = base.Val->Type->AsStruct ()->BaseSymbol ();
    bool  baseIsStatic = base.Val->Kind == ValueKind::Type;
    bool  baseIsThis
        = !baseIsStatic && _insideMethod.has_value () && *_insideMethod->second == *s;
    bool canAccessPrivate
        = baseIsThis
          || baseIsStatic && _insideMethod.has_value () && *_insideMethod->second == *s;
    auto it = s->Methods.find (mc->Name ().Val);
    if (it == s->Methods.end ()) {
        _diag
            .Report (
                DiagCode::EUndefined,
                "type '" + s->Name.Val + "' has no method named '" + mc->Name ().Val
                    + "'",
                Severity::Error)
            .AddSpan (mc->Name ().Start, mc->Name ().End);
        return {};
    }
    auto *candidates = &it->second;

    std::vector<Type *>         argTypes;
    std::vector<SemanticResult> argResults;
    for (auto &a : mc->Args ()) {
        auto argRes = analyzeExpr (a, nullptr);
        argResults.emplace_back (argRes);
        argTypes.emplace_back (argRes.Val->Type);
    }

    symbols::Method *method
        = resolveBestOverload (candidates, argTypes, mc->Start (), mc->End ());
    if (method == nullptr) {
        return {};
    }

    if (!canAccessMethod (mc->Name (), *method, s, baseIsStatic, canAccessPrivate)) {
        return {};
    }
    std::vector<hir::Node *> hirArgs;
    if (!method->IsStatic) {
        hirArgs.emplace_back (
            _builder.CreateReference (base.Node, mc->Start (), mc->End ()));
    }
    for (size_t i = 0; i < argResults.size (); ++i) {
        auto res = analyzeExpr (mc->Args ()[i], method->Func->Args[i].Type);
        hirArgs.emplace_back (res.Node);
    }

    auto *node = _builder.CreateCallMethod (
        method->Func.get (),
        std::move (hirArgs),
        mc->Start (),
        mc->End ());
    auto val = Value (ValueKind::Unknown, method->Func->RetType);
    auto res = SemanticResult (val, node);
    res      = implicitlyCast (res, &expectedType, mc->Start (), mc->End ());
    return res;
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
    if (expectedType == nullptr) {
        expectedType = trueVal.Val->Type;
    }
    _builder.CreateBr (mergeBB, te->Start (), te->End ());

    _builder.SetInsertionPoint (falseBB);
    auto falseVal = analyzeExpr (te->FalseVal (), expectedType);
    if (!falseVal.Val.has_value ()) {
        return {};
    }
    _builder.CreateBr (mergeBB, te->Start (), te->End ());

    _builder.SetInsertionPoint (mergeBB);
    auto  val  = Value (ValueKind::Unknown, expectedType);
    auto *node = _builder.CreateTernary (
        expectedType,
        trueVal.Node,
        trueBB,
        falseVal.Node,
        falseBB,
        te->Start (),
        te->End ());
    auto res = SemanticResult (val, node);
    res      = implicitlyCast (res, &expectedType, te->Start (), te->End ());
    return res;
}

Sema::SemanticResult
Sema::analyzeCastExpr (ast::CastExpr *ce, Type *expectedType) {
    auto  val  = analyzeExpr (ce->GetExpr (), nullptr);
    auto *type = ce->Type ();
    if (!val.Val.has_value () || type == nullptr) {
        return val;
    }
    resolveType (&val.Val->Type);
    resolveType (&type);

    Type *src = val.Val->Type;
    Type *dst = type;

    if (*src == *dst) {
        return val;
    }

    auto kind    = hir::CastKind::Invalid;
    bool canCast = false;
    if (src->IsInteger () && dst->IsInteger ()) {
        canCast = true;
        kind    = castInts (src->AsInteger (), dst->AsInteger ());
    } else if (src->IsFloating () && dst->IsFloating ()) {
        canCast = true;
        kind    = castFloats (src->AsFloating (), dst->AsFloating ());
    } else if (src->IsInteger () && dst->IsFloating ()) {
        canCast = true;
        kind    = src->AsInteger ()->IsUnsigned () ? hir::CastKind::UIToFP
                                                   : hir::CastKind::SIToFP;
    } else if (src->IsFloating () && dst->IsInteger ()) {
        canCast = true;
        kind    = dst->AsInteger ()->IsUnsigned () ? hir::CastKind::FPToUI
                                                   : hir::CastKind::FPToSI;
    }

    if (!canCast) {
        _diag
            .Report (
                DiagCode::ECannotCast,
                "cannot cast '" + typeToString (src) + "' to '" + typeToString (dst)
                    + "'",
                Severity::Error)
            .AddSpan (ce->Start (), ce->End ());

        return {};
    }
    return cast (kind, dst, val, ce->Start (), ce->End ());
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
    auto *node    = _builder.CreateReference (expr.Node, re->Start (), re->End ());
    auto  res     = SemanticResult (val, node);
    res           = implicitlyCast (res, &expectedType, re->Start (), re->End ());
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

hir::CastKind
Sema::castInts (const IntegerType *src, const IntegerType *dst) {
    if (src->BitWidth () < dst->BitWidth ()) {
        return src->IsUnsigned () ? hir::CastKind::ZExt : hir::CastKind::SExt;
    }
    if (src->BitWidth () > dst->BitWidth ()) {
        return hir::CastKind::Trunc;
    }
    return hir::CastKind::BitCast;
}

hir::CastKind
Sema::castFloats (const FloatingType *src, const FloatingType *dst) {
    if (src->IsFloat () && dst->IsDouble ()) {
        return hir::CastKind::FPExt;
    }
    return hir::CastKind::FPTrunc;
}

Sema::SemanticResult
Sema::cast (
    hir::CastKind  kind,
    Type          *dst,
    SemanticResult val,
    llvm::SMLoc    start,
    llvm::SMLoc    end) {
    auto *castNode = _builder.CreateCast (kind, dst, val.Node, start, end);

    Value newValue = val.Val.value ();
    if (newValue.Kind == ValueKind::Const) {
        ValueData newData;
        std::visit (
            [&] (auto val) {
                if (dst->IsInteger ()) {
                    newData = ValueData (static_cast<int64_t> (val));
                } else if (dst->IsFloating ()) {
                    newData = ValueData (static_cast<double> (val));
                }
            },
            val.Val->Data);

        newValue = Value (ValueKind::Const, newData, dst);
    } else {
        newValue.Type = dst;
    }

    auto res = SemanticResult (newValue, castNode);
    return implicitlyCast (res, &dst, start, end);
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
    *type = createType<StructType> (&curMod->Structs.at (typeName.Val));
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

symbols::Method *
Sema::getMethod (
    symbols::Struct                  *sym,
    const std::string                &name,
    const std::vector<ast::Argument> &args) {
    if (!sym->Methods.contains (name)) {
        return nullptr;
    }
    const auto &candidates = sym->Methods.at (name);
    for (const auto &f : candidates.Candidates) {
        unsigned coincidences = 0;
        if (f->Func->Args.size () == args.size ()) {
            for (size_t i = 0; i < args.size (); ++i) {
                if (*f->Func->Args[i].Type == *args[i].Type) {
                    ++coincidences;
                }
            }
        } else {
            continue;
        }

        if (coincidences == args.size ()) {
            return f.get ();
        }
    }
    return nullptr;
}

Type *
Sema::getCommonType (Type *lhs, Type *rhs, llvm::SMLoc start, llvm::SMLoc end) {
    if (*lhs == *rhs) {
        return lhs;
    }
    if (lhs->IsInteger () && rhs->IsInteger ()) {
        return getCommonIngeter (lhs, rhs, start, end);
    }
    if (lhs->IsFloating () && rhs->IsFloating ()) {
        return getCommonFloating (lhs, rhs, start, end);
    }
    if (lhs->IsFloating () && rhs->IsInteger ()
        || lhs->IsInteger () && rhs->IsFloating ()) {
        return getCommonFloatingAndInteger (lhs, rhs, start, end);
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

Type *
Sema::getCommonIngeter (Type *lhs, Type *rhs, llvm::SMLoc start, llvm::SMLoc end) {
    const auto *lhsT = lhs->AsInteger ();
    const auto *rhsT = rhs->AsInteger ();
    if (lhsT->IsUnsigned () == rhsT->IsUnsigned ()) {
        if (lhsT->BitWidth () > rhsT->BitWidth ()) {
            return lhs;
        }
        return rhs;
    }
    const auto *unsignedType    = lhsT->IsUnsigned () ? lhsT : rhsT;
    auto       *unsignedTypeSrc = lhsT->IsUnsigned () ? lhs : rhs;
    const auto *signedType      = lhsT->IsUnsigned () ? rhsT : lhsT;
    auto       *signedTypeSrc   = lhsT->IsUnsigned () ? rhs : lhs;
    if (unsignedType->BitWidth () >= signedType->BitWidth ()) {
        return unsignedTypeSrc;
    }
    return signedTypeSrc;
}

Type *
Sema::getCommonFloating (Type *lhs, Type *rhs, llvm::SMLoc start, llvm::SMLoc end) {
    const auto *lhsT = lhs->AsFloating ();
    const auto *rhsT = rhs->AsFloating ();
    if (lhsT->IsDouble ()) {
        return lhs;
    }
    return rhs;
}

Type *
Sema::getCommonFloatingAndInteger (
    Type *lhs, Type *rhs, llvm::SMLoc start, llvm::SMLoc end) {
    const auto *floatingType
        = lhs->IsFloating () ? lhs->AsFloating () : rhs->AsFloating ();
    const auto *integerType = lhs->IsFloating () ? rhs->AsInteger () : lhs->AsInteger ();
    // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
    const unsigned maxWidth
        = 2U
          << (4U
              + static_cast<unsigned> (
                  floatingType->IsDouble ())); /* 2^(5 + IsDouble()) */
    // NOLINTEND(cppcoreguidelines-avoid-magic-numbers)
    if (integerType->BitWidth () >= maxWidth) {
        _diag
            .Report (
                DiagCode::WLossPrecision,
                "casting types can lead to loss of precision",
                Severity::Warning)
            .AddSpan (start, end);
    }
    return lhs->IsFloating () ? lhs : rhs;
}

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
#define variant(kind, op)                                                                \
    case BinOp::kind: resData = l op r; break;
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
    case BinOp::kind:                                                                    \
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
    UnOp op, const Value &rhs, Type *resType, llvm::SMLoc start, llvm::SMLoc end) {
    return std::visit (
        [&] (auto &&r) -> OptValue {
            using T = std::decay_t<decltype (r)>;
            ValueData resData;

            switch (op) {
            case UnOp::Minus: resData = -r; break;
            case UnOp::Not: {
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

            return Value (ValueKind::Const, resData, resType);
        },
        rhs.Data);
}

bool
Sema::canImplicitlyCast (Sema::SemanticResult val, Type **expectedType) {
    if (!val.Val.has_value () || *expectedType == nullptr) {
        return false;
    }
    resolveType (&val.Val->Type);
    resolveType (expectedType);

    Type *src = val.Val->Type;
    Type *dst = *expectedType;

    if (*src == *dst) {
        return true;
    }
    if (src->IsInteger () && dst->IsInteger () || src->IsFloating () && dst->IsFloating ()
        || src->IsInteger () && dst->IsFloating ()) {
        return true;
    }
    return false;
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
    if (!canImplicitlyCast (val, expectedType)) {
        _diag
            .Report (
                DiagCode::ECannotCast,
                "cannot implicitly cast '" + typeToString (src) + "' to '"
                    + typeToString (dst) + "'",
                Severity::Error)
            .AddSpan (start, end);

        return {};
    }

    auto kind = hir::CastKind::Invalid;
    if (src->IsInteger () && dst->IsInteger ()) {
        kind = castInts (src->AsInteger (), dst->AsInteger ());
    } else if (src->IsFloating () && dst->IsFloating ()) {
        kind = castFloats (src->AsFloating (), dst->AsFloating ());
    } else if (src->IsInteger () && dst->IsFloating ()) {
        kind = src->AsInteger ()->IsUnsigned () ? hir::CastKind::UIToFP
                                                : hir::CastKind::SIToFP;
    } else if (src->IsFloating () && dst->IsInteger ()) {
        kind = dst->AsInteger ()->IsUnsigned () ? hir::CastKind::FPToUI
                                                : hir::CastKind::FPToSI;
    }
    return cast (kind, dst, val, start, end);
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

        size_t costSum = 0;
        if (viableFuncCandidate (cand.get (), argTypes, costSum)) {
            viableCandidates.emplace_back (cand.get (), costSum);
        }
    }

    if (viableCandidates.empty ()) {
        std::vector<std::string> foundCandidates;
        candidatesToStringVector (candidates, foundCandidates);
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

symbols::Method *
Sema::resolveBestOverload (
    symbols::MethodCandidates *candidates,
    const std::vector<Type *> &argTypes,
    llvm::SMLoc                start,
    llvm::SMLoc                end) {
    std::vector<std::pair<symbols::Method *, int>> viableCandidates;

    for (auto &cand : candidates->Candidates) {
        if (cand->Func->Args.size () != argTypes.size ()) {
            continue;
        }

        size_t costSum = 0;
        if (viableFuncCandidate (cand->Func.get (), argTypes, costSum)) {
            viableCandidates.emplace_back (cand.get (), costSum);
        }
    }

    if (viableCandidates.empty ()) {
        std::vector<std::string> foundCandidates;
        candidatesToStringVector (candidates, foundCandidates);
        _diag
            .Report (
                DiagCode::ENoMatchingFunction,
                "no matching function for call to '"
                    + candidates->Candidates[0]->Func->Name.Val + "'",
                Severity::Error)
            .AddSpan (start, end)
            .AddNote ("candidate functions found:", std::move (foundCandidates));
        return nullptr;
    }

    symbols::Method *bestCand    = viableCandidates[0].first;
    size_t           minCost     = viableCandidates[0].second;
    bool             isAmbiguous = false;

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
                c->Func->Name.Start,
                c->Func->Name.End,
                "candidate " + std::to_string (i),
                false);
            ++i;
        }
        return nullptr;
    }

    return bestCand;
}

void
Sema::candidatesToStringVector (
    symbols::FunctionCandidates *candidates, std::vector<std::string> &res) {
    for (auto &func : candidates->Candidates) {
        res.emplace_back (funcCandidateToString (func.get ()));
    }
}

void
Sema::candidatesToStringVector (
    symbols::MethodCandidates *candidates, std::vector<std::string> &res) {
    for (auto &method : candidates->Candidates) {
        res.emplace_back (funcCandidateToString (method->Func.get ()));
    }
}

std::string
Sema::funcCandidateToString (symbols::Function *func) {
    std::ostringstream oss;
    oss << "func " << func->Name.Val << "(";
    size_t i = 0;
    for (const auto &a : func->Args) {
        oss << typeToString (a.Type);
        if (i < func->Args.size () - 1) {
            oss << ", ";
        }
        ++i;
    }
    oss << "): " << typeToString (func->RetType);
    return oss.str ();
}

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
Sema::viableFuncCandidate (
    symbols::Function *func, const std::vector<Type *> &args, size_t &costSum) {
    bool viable = true;
    for (size_t i = 0; i < args.size (); ++i) {
        CastCost cost = checkCastCost (args[i], func->Args[i].Type);
        if (cost == CastCost::Incompatible) {
            viable = false;
            break;
        }
        costSum += static_cast<size_t> (cost);
    }
    return viable;
}

bool
Sema::inGlobalScope () const {
    return _vars.size () == 1;
}

bool
Sema::allowInScope (ast::Stmt *stmt, bool allowInGlobal) {
    if (inGlobalScope () != allowInGlobal) {
        _diag
            .Report (
                DiagCode::ENotAllowedInThisScope,
                "statement not allowed in this scope",
                Severity::Error)
            .AddSpan (
                stmt->Start (),
                stmt->End (),
                "statement found outside of a function");
        return false;
    }
    return true;
}

bool
Sema::canAccessField (
    const basic::NameObj  &feName,
    const symbols::Field  &field,
    const symbols::Struct *s,
    bool                   canAccessStatic,
    bool                   canAccessPrivate) {
    if (field.IsStatic && !canAccessStatic) {
        _diag
            .Report (
                DiagCode::ECannotAccessStaticMemberFromInstance,
                "static field '" + feName.Val
                    + "' cannot be accessed through an instance",
                Severity::Error)
            .AddSpan (feName.Start, feName.End)
            .AddNote (
                "use the type name instead to access this field: '" + s->Name.Val + "."
                + feName.Val + "'");
        return false;
    }
    if (!field.IsStatic && canAccessStatic) {
        _diag
            .Report (
                DiagCode::ECannotAccessNonStaticMemberFromType,
                "instance field '" + feName.Val + "' cannot be accessed through a type",
                Severity::Error)
            .AddSpan (feName.Start, feName.End);
        return false;
    }
    if (field.Access != AccessModifier::Pub && !canAccessPrivate) {
        _diag
            .Report (
                DiagCode::ECannotAccessToPrivMember,
                "field '" + feName.Val + "' of struct '" + s->Name.Val + "' is private",
                Severity::Error)
            .AddSpan (feName.Start, feName.End);
        return false;
    }
    return true;
}

bool
Sema::canAccessMethod (
    const basic::NameObj  &mcName,
    const symbols::Method &method,
    const symbols::Struct *s,
    bool                   canAccessStatic,
    bool                   canAccessPrivate) {
    if (method.IsStatic && !canAccessStatic) {
        _diag
            .Report (
                DiagCode::ECannotAccessStaticMemberFromInstance,
                "static method '" + mcName.Val
                    + "' cannot be accessed through an instance",
                Severity::Error)
            .AddSpan (mcName.Start, mcName.End);
        return false;
    }
    if (!method.IsStatic && canAccessStatic) {
        _diag
            .Report (
                DiagCode::ECannotAccessNonStaticMemberFromType,
                "instance method '" + mcName.Val + "' cannot be accessed through a type",
                Severity::Error)
            .AddSpan (mcName.Start, mcName.End);
        return false;
    }
    if (method.Access != AccessModifier::Pub && !canAccessPrivate) {
        _diag
            .Report (
                DiagCode::ECannotAccessToPrivMember,
                "method '" + mcName.Val + "' of struct '" + s->Name.Val + "' is private",
                Severity::Error)
            .AddSpan (mcName.Start, mcName.End);
        return false;
    }
    return true;
}

std::string
Sema::typeToString (Type *type) {
    if (!type->IsStruct ()) {
        return type->ToString ();
    }
    const auto *sType  = type->AsStruct ();
    std::string res    = sType->BaseSymbol ()->Name.Val;
    Module     *curMod = sType->BaseSymbol ()->Parent;
    while (curMod != nullptr) {
        if (*curMod == *_mod) {
            break;
        }
        res = curMod->Name + '.' + res; // NOLINT
    }
    return res;
}

}
