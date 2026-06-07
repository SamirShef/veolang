#include <algorithm>
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
    } else if (type != nullptr) {
        val.Val = Value (ValueKind::Const, type);
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
    if (type == nullptr || type->IsNoth ()) {
        _diag
            .Report (
                DiagCode::EVarCannotHaveTypeNoth,
                "variable cannot have type 'noth'",
                Severity::Error)
            .AddSpan (
                vd->Name ().Start,
                vd->Name ().End,
                "'noth' is not a valid type for a variable")
            .AddNote (
                "variables must have a type that can represent data and be stored in "
                "memory");
        return;
    }
    auto var = Variable (vd->Name (), type, vd->IsConst (), isGlobal, _mod, val.Val);
    symbols::Variable *sym = nullptr;
    _vars.top ().Vars.emplace (var.Name.Val, var);
    sym = &_vars.top ().Vars.at (var.Name.Val);
    if (isGlobal) {
        _mod->Vars.emplace (var.Name.Val, var);
        sym = &_mod->Vars.at (var.Name.Val);
    }
    auto *node = _builder.CreateVariable (
        var.Name,
        type,
        val.Node,
        vd->IsConst (),
        isGlobal,
        vd->Start (),
        vd->End (),
        isGlobal ? sym : nullptr);
    sym->HIR                                = node;
    _vars.top ().Vars.at (var.Name.Val).HIR = node;
}

void
Sema::declareFunc (FuncDef *fd) {
    if (fd->IsDeclaration ()) {
        _diag
            .Report (
                DiagCode::EFuncOutsideTraitMustHaveBody,
                "functions outside of traits must have a body",
                Severity::Error)
            .AddSpan (fd->Start (), fd->End (), "missing function body");
        return;
    }
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
    bool isGeneric = false;
    for (auto &arg : fd->Args ()) {
        resolveType (&arg.Type);
        if (arg.Type->IsTrait ()) {
            isGeneric = true;
        }
    }
    auto func = Function (fd->Name (), fd->RetType (), fd->Args (), isGeneric, _mod);
    auto it   = _mod->Funcs.find (func.Name.Val);
    if (it == _mod->Funcs.end ()) {
        _mod->Funcs.emplace (func.Name.Val, FunctionCandidates ());
    }
    auto &candidates = _mod->Funcs.at (func.Name.Val).Candidates;
    auto  funcPtr    = std::make_unique<Function> (func);
    if (!isGeneric) {
        auto *funcNode = _builder.CreateFunction (
            func.Name,
            fd->RetType (),
            {},
            fd->Start (),
            fd->End (),
            funcPtr.get ());
        _funcs.emplace (funcPtr.get (), funcNode);
    }
    candidates.emplace_back (std::move (funcPtr));
    if (isGeneric) {
        auto it
            = std::ranges::find_if (candidates, [&] (const std::unique_ptr<Function> &f) {
                  return func == *f;
              });
        _genericFuncs.emplace (it->get (), fd);
    }
}

void
Sema::analyzeFuncDef (FuncDef *fd, bool generatingGeneric) {
    if (!generatingGeneric && !allowInScope (fd)) {
        return;
    }
    if (fd->IsDeclaration ()) {
        return;
    }

    auto     *candidates = &_mod->Funcs.at (fd->Name ().Val);
    Function *func       = nullptr;
    for (auto &f : candidates->Candidates) {
        if (f->Args == fd->Args ()) {
            func = f.get ();
        }
    }
    auto *funcNode = func->IsGeneric ? nullptr : _funcs.at (func);
    auto *entry    = _builder.CreateBasicBlock (funcNode, "entry");
    _builder.SetInsertionPoint (entry);

    std::vector<hir::VarDef *> args;
    args.reserve (fd->Args ().size ());
    for (const auto &arg : fd->Args ()) {
        auto *node = _builder.CreateVariable (
            arg.Name,
            arg.Type,
            nullptr,
            false,
            false,
            arg.Name.Start,
            arg.Name.End,
            nullptr,
            false);
        args.emplace_back (node);
    }
    if (funcNode != nullptr) {
        funcNode->Args () = std::move (args);
    }

    _funcRetTypes.push (fd->RetType ());
    _vars.emplace ();

    _localsCount = 0;

    size_t index = 0;
    for (const auto &arg : fd->Args ()) {
        _vars.top ().Vars.emplace (
            arg.Name.Val,
            Variable (
                arg.Name,
                arg.Type,
                false,
                false,
                nullptr,
                std::nullopt,
                funcNode != nullptr ? funcNode->Args ()[index] : nullptr));
        ++index;
    }

    for (const auto &stmt : fd->Body ()) {
        analyzeStmt (stmt);
    }
    if (func->RetType == nullptr || func->RetType->IsNoth ()) {
        _builder.CreateRet (nullptr, fd->Name ().Start, fd->Name ().End);
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
        if (!_funcRetTypes.top ()->IsNoth ()) {
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
        res = implicitlyCast (
            res,
            &_funcRetTypes.top (),
            ret->RetExpr ()->Start (),
            ret->RetExpr ()->End ());
        _builder.CreateRet (res.Node, ret->Start (), ret->End ());
    }
}

void
Sema::analyzeExprStmt (ExprStmt *es) {
    if (!allowInScope (es, false)) {
        return;
    }
    auto res = analyzeExpr (es->GetExpr (), nullptr);
    _builder.CreateExprStmt (res.Node, es->Start (), es->End ());
}

void
Sema::analyzeIfElseStmt (IfElseStmt *ies) {
    if (!allowInScope (ies, false)) {
        return;
    }
    auto cond = analyzeExpr (ies->Cond (), createType<BoolType> ());
    if (!cond.Val.has_value ()) {
        return;
    }
    if (cond.Val->Kind == ValueKind::Const) {
        bool                 condRes     = std::get<0> (cond.Val->Data) != 0;
        std::vector<Stmt *> &realBranch  = condRes ? ies->Then () : ies->Else ();
        std::vector<Stmt *> &otherBranch = condRes ? ies->Else () : ies->Then ();
        _vars.emplace ();
        for (const auto &stmt : realBranch) {
            analyzeStmt (stmt);
        }
        _vars.pop ();

        auto *lastBlock = _builder.InsertBlock ();
        _builder.SetInsertionPoint (
            nullptr); // analyze statements but do not add to the block

        _vars.emplace ();
        for (const auto &stmt : otherBranch) {
            analyzeStmt (stmt);
        }
        _vars.pop ();

        _builder.SetInsertionPoint (lastBlock);
        return;
    }
    auto *thenBB  = _builder.CreateBasicBlock (_builder.Parent (), "then");
    auto *elseBB  = ies->Else ().empty ()
                        ? nullptr
                        : _builder.CreateBasicBlock (_builder.Parent (), "else");
    auto *mergeBB = _builder.CreateBasicBlock (_builder.Parent (), "merge");

    _builder.CreateBr (
        cond.Node,
        thenBB,
        elseBB != nullptr ? elseBB : mergeBB,
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

    if (elseBB != nullptr) { // else
        _builder.SetInsertionPoint (elseBB);
        _vars.emplace ();
        for (const auto &stmt : ies->Else ()) {
            analyzeStmt (stmt);
        }
        _vars.pop ();
        _builder.CreateBr (mergeBB, ies->Cond ()->Start (), ies->Cond ()->End ());
    }

    // merge
    _builder.SetInsertionPoint (mergeBB);
}

void
Sema::analyzeForLoop (ForLoopStmt *fls) {
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
        if (cond.Val->Kind == ValueKind::Const) {
            if (std::get<0> (cond.Val->Data) == 1) {
                _builder.CreateBr (bodyBB, fls->Start (), fls->End ());
            } else {
                _builder.CreateBr (mergeBB, fls->Start (), fls->End ());
            }
        } else {
            _builder.CreateBr (cond.Node, bodyBB, mergeBB, fls->Start (), fls->End ());
        }
    } else {
        auto val = Value (ValueKind::Const, ValueData (1), boolType);
        cond     = { val, _builder.CreateLiteral (val, fls->Start (), fls->End ()) };
        _builder.CreateBr (bodyBB, fls->Start (), fls->End ());
    }

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

    auto s = Struct (sd->Name (), {}, _mod);
    _mod->Structs.emplace (sd->Name ().Val, std::move (s));

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
        if (field.Type->IsStruct ()) {
            const auto *sType = field.Type->AsStruct ();
            if (*sType->BaseSymbol () == _mod->Structs.at (sd->Name ().Val)) {
                _diag
                    .Report (
                        DiagCode::ERecursiveType,
                        "recursive type '" + sd->Name ().Val + "' has infinite size",
                        Severity::Error)
                    .AddSpan (
                        sd->Name ().Start,
                        sd->Name ().End,
                        "recursive type has infinite size")
                    .AddSpan (
                        field.Name.Start,
                        field.Name.End,
                        "recursive without indirection",
                        false)
                    .AddNote (
                        "insert an indirection (e.g., a pointer or reference) to "
                        "make '"
                            + sd->Name ().Val + "' representable:",
                        { "change '" + field.Name.Val + ": " + sd->Name ().Val + "' to '"
                          + field.Name.Val + ": *" + sd->Name ().Val + "'" });
                continue;
            }
        }
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
    _mod->Structs.at (sd->Name ().Val).Fields = std::move (fields);
    _builder.CreateStruct (
        sd->Name (),
        hirFields,
        &_mod->Structs.at (sd->Name ().Val),
        sd->Start (),
        sd->End ());
}

void
Sema::declareImplMethods (ImplStmt *is) {
    if (!allowInScope (is)) {
        return;
    }
    resolveType (&is->StructType ());
    auto *targetType = is->StructType ();
    if (targetType == nullptr) {
        return;
    }
    auto *traitType = is->TraitType ();
    resolveType (&traitType);
    if (traitType != nullptr && !traitType->IsTrait ()) {
        _diag
            .Report (
                DiagCode::ECannotImplNonTraitForType,
                "cannot implement non-trait '" + typeToString (traitType) + "' for type '"
                    + typeToString (targetType) + "'",
                Severity::Error)
            .AddSpan (is->Start (), is->End ());
        return;
    }
    bool  isStruct = targetType->IsStruct ();
    auto *sym      = isStruct ? targetType->AsStruct ()->BaseSymbol () : nullptr;
    if (traitType != nullptr && isStruct) {
        sym->TraitsImplements.emplace (traitType->AsTrait ()->BaseSymbol ());
    }
    for (auto &method : is->Methods ()) {
        declareImplMethod (method, sym, targetType);
    }
    if (traitType != nullptr) {
        const auto *trait           = traitType->AsTrait ();
        const auto &traitCandidates = trait->BaseSymbol ()->Methods;
        const auto &methods
            = sym != nullptr ? sym->Methods : _mod->PrimitiveMethods[targetType];
        for (const auto &[name, candidates] : traitCandidates) {
            if (!methods.contains (name)) {
                _diag
                    .Report (
                        DiagCode::EStructDoesNotImplementedMethod,
                        "type '" + typeToString (targetType)
                            + "' does not implement method '" + name + "' from trait '"
                            + typeToString (traitType) + "'",
                        Severity::Error)
                    .AddSpan (is->Start (), is->End ());
                continue;
            }
            const auto &structCandidates = methods.at (name).Candidates;
            for (const auto &method : candidates.Candidates) {
                auto it = std::ranges::find_if (
                    structCandidates,
                    [&] (const std::unique_ptr<symbols::Method> &m) {
                        return *m == *method;
                    });
                if (it == structCandidates.end ()) {
                    _diag
                        .Report (
                            DiagCode::EStructDoesNotImplementedMethod,
                            "type '" + typeToString (targetType)
                                + "' does not implement method '" + name
                                + "' from trait '" + typeToString (traitType) + "'",
                            Severity::Error)
                        .AddSpan (is->Start (), is->End ());
                    continue;
                }
            }
        }
    }
}

void
Sema::declareImplMethod (
    ast::Method method, symbols::Struct *sym, basic::Type *targetType) {
    auto *fd = method.Func;
    if (fd->IsDeclaration ()) {
        _diag
            .Report (
                DiagCode::EFuncOutsideTraitMustHaveBody,
                "methods outside of traits must have a body",
                Severity::Error)
            .AddSpan (fd->Start (), fd->End (), "missing function body");
        return;
    }
    if (auto *m = getMethod (targetType, fd->Name ().Val, fd->Args ()); m != nullptr) {
        _diag
            .Report (
                DiagCode::ERedefinition,
                "method '" + fd->Name ().Val + "' is already defined in type '"
                    + typeToString (targetType) + "'",
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
    bool isGeneric = false;
    for (auto &arg : fd->Args ()) {
        resolveType (&arg.Type);
        if (arg.Type->IsTrait ()) {
            isGeneric = true;
        }
    }
    auto func = Function (fd->Name (), fd->RetType (), fd->Args (), isGeneric, _mod);
    auto m    = symbols::Method (
        std::make_unique<Function> (func),
        fd->Access (),
        method.IsStatic,
        isGeneric);
    std::unordered_map<std::string, MethodCandidates> *methods = nullptr;
    if (targetType->IsStruct ()) {
        methods = &sym->Methods;
    } else {
        methods = &_mod->PrimitiveMethods[targetType];
    }
    auto it = methods->find (m.Func->Name.Val);
    if (it == methods->end ()) {
        methods->emplace (m.Func->Name.Val, MethodCandidates ());
    }
    auto methodPtr = std::make_unique<symbols::Method> (std::move (m));
    if (!isGeneric) {
        auto *methodNode = _builder.CreateMethod (
            methodPtr->Func->Name,
            fd->RetType (),
            {},
            fd->Start (),
            fd->End (),
            methodPtr.get (),
            targetType,
            method.IsStatic);
        _methods.emplace (methodPtr.get (), methodNode);
    }
    auto &candidates = methods->at (methodPtr->Func->Name.Val).Candidates;
    candidates.emplace_back (std::move (methodPtr));
    if (isGeneric) {
        auto it = std::ranges::find_if (
            candidates,
            [&] (const std::unique_ptr<symbols::Method> &method) {
                return *method == *candidates.back ();
            });
        _genericMethods.emplace (it->get (), fd);
    }
}

void
Sema::analyzeImplStmt (ImplStmt *is) {
    auto *targetType = is->StructType ();
    resolveType (&targetType);
    auto *sym
        = targetType->IsStruct () ? targetType->AsStruct ()->BaseSymbol () : nullptr;
    for (auto &method : is->Methods ()) {
        analyzeImplMethodDef (method, sym, targetType);
    }
}

void
Sema::analyzeImplMethodDef (
    ast::Method method, symbols::Struct *sym, basic::Type *targetType) {
    auto *fd = method.Func;
    if (fd->IsDeclaration ()) {
        return;
    }
    auto *candidates = sym != nullptr
                           ? &sym->Methods.at (fd->Name ().Val)
                           : &_mod->PrimitiveMethods.at (targetType).at (fd->Name ().Val);
    symbols::Method *m = nullptr;
    for (auto &f : candidates->Candidates) {
        if (f->Func->Args == fd->Args ()) {
            m = f.get ();
        }
    }

    auto *methodNode = m->IsGeneric ? nullptr : _methods.at (m);
    auto *entry      = _builder.CreateBasicBlock (methodNode, "entry");
    _builder.SetInsertionPoint (entry);

    std::vector<hir::VarDef *> args;
    args.reserve (fd->Args ().size () + (method.IsStatic ? 1 : 0));
    if (!method.IsStatic) {
        auto *node = _builder.CreateVariable (
            basic::NameObj ("this", fd->Name ().Start, fd->Name ().End),
            createType<PointerType> (targetType),
            nullptr,
            false,
            false,
            fd->Name ().Start,
            fd->Name ().End,
            nullptr,
            false);
        args.emplace_back (node);
    }
    for (const auto &arg : fd->Args ()) {
        auto *node = _builder.CreateVariable (
            arg.Name,
            arg.Type,
            nullptr,
            false,
            false,
            arg.Name.Start,
            arg.Name.End,
            nullptr,
            false);
        args.emplace_back (node);
    }
    if (methodNode != nullptr) {
        methodNode->Args () = std::move (args);
    }

    _insideMethod = { m, targetType };
    _funcRetTypes.push (fd->RetType ());
    _vars.emplace ();

    _localsCount = 0;

    auto index = static_cast<size_t> (!m->IsStatic);
    if (methodNode != nullptr && !method.IsStatic) {
        auto *thisArg = methodNode->Args ()[0];
        _vars.top ().Vars.emplace (
            thisArg->Name ().Val,
            Variable (
                thisArg->Name (),
                thisArg->Type (),
                false,
                false,
                nullptr,
                std::nullopt,
                thisArg));
    }
    for (const auto &arg : fd->Args ()) {
        _vars.top ().Vars.emplace (
            arg.Name.Val,
            Variable (
                arg.Name,
                arg.Type,
                false,
                false,
                nullptr,
                std::nullopt,
                m->IsGeneric ? nullptr : methodNode->Args ()[index]));
        ++index;
    }

    for (const auto &stmt : fd->Body ()) {
        analyzeStmt (stmt);
    }

    _vars.pop ();
    _funcRetTypes.pop ();
    _insideMethod = std::nullopt;
}

void
Sema::analyzeTraitStmt (TraitStmt *ts) {
    if (!allowInScope (ts)) {
        return;
    }
    if (auto it = _mod->Traits.find (ts->Name ().Val); it != _mod->Traits.end ()) {
        auto &t = it->second; // trait
        _diag
            .Report (
                DiagCode::ERedefinition,
                "trait '" + ts->Name ().Val + "' is already defined",
                Severity::Error)
            .AddSpan (t.Name.Start, t.Name.End, "previous definition was here", false)
            .AddSpan (ts->Name ().Start, ts->Name ().End, "redefined here");
        return;
    }
    auto trait = symbols::Trait (ts->Name (), _mod);
    _mod->Traits.emplace (ts->Name ().Val, std::move (trait));
    auto *targetType = createType<TraitType> (&_mod->Traits.at (ts->Name ().Val));

    std::unordered_map<std::string, MethodCandidates> methods;
    for (const auto &method : ts->Methods ()) {
        auto *fd = method.Func;
        if (auto *m = getMethod (targetType, fd->Name ().Val, fd->Args ());
            m != nullptr) {
            _diag
                .Report (
                    DiagCode::ERedefinition,
                    "method '" + fd->Name ().Val + "' is already defined in type '"
                        + typeToString (targetType) + "'",
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
        bool isGeneric = false;
        for (auto &arg : fd->Args ()) {
            resolveType (&arg.Type);
            if (arg.Type->IsTrait ()) {
                isGeneric = true;
            }
        }
        auto func = Function (fd->Name (), fd->RetType (), fd->Args (), isGeneric, _mod);
        auto m    = symbols::Method (
            std::make_unique<Function> (func),
            fd->Access (),
            method.IsStatic,
            isGeneric);
        auto it = methods.find (m.Func->Name.Val);
        if (it == methods.end ()) {
            methods.emplace (m.Func->Name.Val, MethodCandidates ());
        }
        auto  methodPtr  = std::make_unique<symbols::Method> (std::move (m));
        auto &candidates = methods.at (methodPtr->Func->Name.Val).Candidates;
        candidates.emplace_back (std::move (methodPtr));
    }
    _mod->Traits.at (ts->Name ().Val).Methods = std::move (methods);
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
    }
}

#undef float_lit
#undef int_lit

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
        resType,
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

    if (expectedType != nullptr) {
        res = implicitlyCast (res, &expectedType, ue->Start (), ue->End ());
    }

    return res;
}

// NOLINTBEGIN(readability-convert-member-functions-to-static)
Sema::SemanticResult
Sema::analyzeVarExpr (VarExpr *ve, Type *expectedType) {
    auto *var = getVariable (ve->Name ().Val);
    if (var == nullptr) {
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
        if (!argRes.Val.has_value ()) {
            continue;
        }
        argResults.emplace_back (argRes);
        argTypes.emplace_back (argRes.Val->Type);
    }

    auto *func = resolveBestOverload (candidates, argTypes, fc->Start (), fc->End ());
    if (func == nullptr) {
        return {};
    }

    if (func->IsGeneric) {
        auto                 *lastBB  = _builder.InsertBlock ();
        auto                 *oldFunc = _genericFuncs.at (func);
        std::vector<Argument> args;
        args.reserve (oldFunc->Args ().size ());
        size_t i = 0;
        for (const auto &a : oldFunc->Args ()) {
            Type *type = nullptr;
            if (a.Type->IsTrait ()) {
                type = argTypes[i];
            } else {
                type = a.Type;
            }
            args.emplace_back (a.Name, type);
            ++i;
        }
        auto *newFunc = createNode<FuncDef> (
            oldFunc->Name (),
            oldFunc->RetType (),
            std::move (args),
            oldFunc->Body (),
            false,
            oldFunc->Access (),
            oldFunc->Start (),
            oldFunc->End ());
        declareFunc (newFunc);
        analyzeFuncDef (newFunc, true);
        _builder.SetInsertionPoint (lastBB);
        func = resolveBestOverload (candidates, argTypes, fc->Start (), fc->End ());
        if (func == nullptr) {
            return {};
        }
    }

    std::vector<hir::Node *> hirArgs;
    for (size_t i = 0; i < argResults.size (); ++i) {
        auto res = analyzeExpr (fc->Args ()[i], func->Args[i].Type);
        hirArgs.emplace_back (res.Node);
    }

    auto res = SemanticResult (
        Value (ValueKind::Unknown, func->RetType),
        _builder.CreateCall (
            _funcs.at (func),
            std::move (hirArgs),
            fc->Start (),
            fc->End ()));
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
    auto *targetType = base.Val->Type;
    while (targetType->IsPointer ()) {
        targetType = targetType->AsPointer ()->Base ();
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
    auto *sym              = targetType->AsStruct ()->BaseSymbol ();
    bool  baseIsStatic     = base.Val->Kind == ValueKind::Type;
    bool  baseIsThis       = !baseIsStatic && _insideMethod.has_value ()
                             && *_insideMethod->second == *targetType;
    bool  canAccessPrivate = baseIsThis
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
    auto *targetType = base.Val->Type;
    while (targetType->IsPointer ()) {
        targetType = targetType->AsPointer ()->Base ();
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
                    + typeToString (base.Val->Type) + "'",
                Severity::Error)
            .AddSpan (fe->Name ().Start, fe->Name ().End);
        return {};
    }
    auto *s                = targetType->AsStruct ()->BaseSymbol ();
    bool  baseIsStatic     = base.Val->Kind == ValueKind::Type;
    bool  baseIsThis       = !baseIsStatic && _insideMethod.has_value ()
                             && *_insideMethod->second == *targetType;
    bool  canAccessPrivate = baseIsThis
                             || baseIsStatic && _insideMethod.has_value ()
                                    && *_insideMethod->second == *targetType;
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
    auto *targetType = base.Val->Type;
    while (targetType->IsPointer ()) {
        targetType = targetType->AsPointer ()->Base ();
        base.Node  = _builder.CreateDereference (
            base.Node,
            targetType,
            mc->Start (),
            mc->End ());
    }
    auto *s = targetType->IsStruct () ? targetType->AsStruct ()->BaseSymbol () : nullptr;
    bool  baseIsConstVar   = base.Val->Kind == ValueKind::Const
                             && base.Node->Kind () == hir::NodeKind::LoadVar;
    bool  baseIsStatic     = base.Val->Kind == ValueKind::Type;
    bool  baseIsThis       = !baseIsStatic && _insideMethod.has_value ()
                             && *_insideMethod->second == *targetType;
    bool  canAccessPrivate = baseIsThis
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

    auto *method = resolveBestOverload (candidates, argTypes, mc->Start (), mc->End ());
    if (method == nullptr) {
        return {};
    }

    if (!canAccessMethod (
            mc->Name (),
            *method,
            targetType,
            baseIsStatic,
            canAccessPrivate)) {
        return {};
    }
    if (baseIsConstVar) {
        _methodCallOnConstBase.emplace_back (mc, method);
    }
    if (method->Func->IsGeneric) {
        auto                 *lastBB    = _builder.InsertBlock ();
        auto                 *oldMethod = _genericMethods.at (method);
        std::vector<Argument> args;
        args.reserve (oldMethod->Args ().size ());
        size_t i = 0;
        for (const auto &a : oldMethod->Args ()) {
            Type *type = nullptr;
            if (a.Type->IsTrait ()) {
                type = argTypes[i];
            } else {
                type = a.Type;
            }
            args.emplace_back (a.Name, type);
            ++i;
        }
        auto *newFunc = createNode<FuncDef> (
            oldMethod->Name (),
            oldMethod->RetType (),
            std::move (args),
            oldMethod->Body (),
            false,
            oldMethod->Access (),
            oldMethod->Start (),
            oldMethod->End ());
        auto newMethod = ast::Method (newFunc, method->IsStatic);
        declareImplMethod (newMethod, s, targetType);
        analyzeImplMethodDef (newMethod, s, targetType);
        _builder.SetInsertionPoint (lastBB);
        method = resolveBestOverload (candidates, argTypes, mc->Start (), mc->End ());
        if (method == nullptr) {
            return {};
        }
    }
    std::vector<hir::Node *> hirArgs;
    if (!method->IsStatic) {
        hirArgs.emplace_back (_builder.CreateReference (
            base.Node,
            createType<PointerType> (targetType),
            mc->Start (),
            mc->End ()));
    }
    for (size_t i = 0; i < argResults.size (); ++i) {
        auto res = analyzeExpr (mc->Args ()[i], method->Func->Args[i].Type);
        hirArgs.emplace_back (res.Node);
    }

    auto *node = targetType->IsTrait () ? nullptr
                                        : _builder.CreateCall (
                                              _methods.at (method),
                                              std::move (hirArgs),
                                              mc->Start (),
                                              mc->End ());
    auto  val  = Value (ValueKind::Unknown, method->Func->RetType);
    auto  res  = SemanticResult (val, node);
    res        = implicitlyCast (res, &expectedType, mc->Start (), mc->End ());
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
    auto *tmp = _builder.CreateVariable (
        basic::NameObj (".tmp", {}, {}),
        expectedType,
        nullptr,
        false,
        false,
        te->Start (),
        te->End (),
        nullptr);
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
        return val;
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
        return val;
    }

    auto kind = hir::CastKind::Invalid;

    auto getIntegralProps = [this] (Type *t, bool &isUnsigned, unsigned &width) {
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
    };

    auto isIntegral
        = [] (Type *t) { return t->IsIntOrSize () || t->IsBool () || t->IsChar (); };

    if (isIntegral (src) && isIntegral (dst)) {
        bool     srcUnsigned = false;
        bool     dstUnsigned = false;
        unsigned srcWidth    = 0;
        unsigned dstWidth    = 0;

        getIntegralProps (src, srcUnsigned, srcWidth);
        getIntegralProps (dst, dstUnsigned, dstWidth);

        if (srcWidth < dstWidth) {
            kind = srcUnsigned ? hir::CastKind::ZExt : hir::CastKind::SExt;
        } else if (srcWidth > dstWidth) {
            kind = hir::CastKind::Trunc;
        } else {
            kind = hir::CastKind::BitCast;
        }
    } else if (src->IsFloating () && dst->IsFloating ()) {
        const auto *srcF = src->AsFloating ();
        const auto *dstF = dst->AsFloating ();
        kind             = castFloats (src->AsFloating (), dst->AsFloating ());
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
                if (dst->IsIntOrSize ()) {
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
    if ((*type)->IsPointer ()) {
        auto *ptrBase = (*type)->AsPointer ()->Base ();
        resolveType (&ptrBase);
        *type = createType<PointerType> (ptrBase);
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
    auto        structIt = curMod->Structs.find (typeName.Val);
    if (structIt != curMod->Structs.end ()) {
        *type = createType<StructType> (&structIt->second);
        return *type;
    }
    auto traitIt = curMod->Traits.find (typeName.Val);
    if (traitIt != curMod->Traits.end ()) {
        *type = createType<TraitType> (&traitIt->second);
        return *type;
    }

    _diag
        .Report (
            DiagCode::ECannotFindType,
            "cannot find type '" + typeName.Val + "' in "
                + (path.size () == 1 ? "this scope" : curMod->ToString ()),
            Severity::Error)
        .AddSpan (typeName.Start, typeName.End, "not found");
    return nullptr;
}
// NOLINTEND(readability-convert-member-functions-to-static)

Variable *
Sema::getVariable (const std::string &name) {
    auto vars = _vars;
    while (!vars.empty ()) {
        if (auto it = vars.top ().Vars.find (name); it != vars.top ().Vars.end ()) {
            return &it->second;
        }
        vars.pop ();
    }
    return nullptr;
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
    basic::Type *base, const std::string &name, const std::vector<Argument> &args) {
    std::unordered_map<std::string, MethodCandidates> *methods = nullptr;
    if (base->IsStruct ()) {
        auto *sym = base->AsStruct ()->BaseSymbol ();
        methods   = &sym->Methods;
        if (!methods->contains (name)) {
            return nullptr;
        }
    } else if (base->IsTrait ()) {
        auto *sym = base->AsTrait ()->BaseSymbol ();
        methods   = &sym->Methods;
        if (!methods->contains (name)) {
            return nullptr;
        }
    } else {
        methods = &_mod->PrimitiveMethods[base];
        if (!methods->contains (name)) {
            return nullptr;
        }
    }
    const auto &candidates = methods->at (name);
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
    if (lhs == nullptr || rhs == nullptr) {
        return nullptr;
    }
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
        .AddNote ("сonsider using an explicit cast");
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
Sema::foldBinaryBitwise (ValueData lhs, ValueData rhs, Type *commonType, BinOp op) {
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
Sema::canImplicitCast (Sema::SemanticResult val, Type **expectedType) {
    if (!val.Val.has_value ()) {
        return false;
    }
    resolveType (&val.Val->Type);
    resolveType (expectedType);
    if (val.Val->Type == nullptr || *expectedType == nullptr) {
        return false;
    }

    Type *src = val.Val->Type;
    Type *dst = *expectedType;

    if (*src == *dst) {
        return true;
    }
    if (src->IsInteger () && dst->IsInteger ()) {
        const auto *srcI = src->AsInteger ();
        const auto *dstI = dst->AsInteger ();
        if (dstI->BitWidth () >= srcI->BitWidth ()) {
            if (srcI->IsUnsigned () == dstI->IsUnsigned ()
                || dstI->BitWidth () > srcI->BitWidth ()) {
                return true;
            }
        }
    }
    if (src->IsFloating () && dst->IsFloating ()) {
        const auto *srcF = src->AsFloating ();
        const auto *dstF = dst->AsFloating ();
        return (int) dstF->GetFloatingKind () >= (int) srcF->GetFloatingKind ();
    }
    if (src->IsInteger () && dst->IsFloating ()) {
        return true;
    }
    if (src->IsStruct () && dst->IsTrait ()) {
        auto *sSym = src->AsStruct ()->BaseSymbol ();
        auto *tSym = dst->AsTrait ()->BaseSymbol ();
        return sSym->TraitsImplements.contains (tSym);
    }
    return false;
}

bool
Sema::canExplicitCast (Type *src, Type *dst) {
    if (*src == *dst) {
        return true;
    }
    auto isScalar = [] (Type *t) {
        return t->IsInteger () || t->IsSize () || t->IsFloating () || t->IsChar ()
               || t->IsBool ();
    };
    if (isScalar (src) && isScalar (dst)) {
        return true;
    }
    if (src->IsPointer () && dst->IsSize ()) {
        return true;
    }
    if (src->IsSize () && dst->IsPointer ()) {
        return true;
    }
    if (src->IsPointer () && dst->IsPointer ()) {
        return true;
    }

    return false;
}

Sema::SemanticResult
Sema::implicitlyCast (
    SemanticResult val, Type **expectedType, llvm::SMLoc start, llvm::SMLoc end) {
    if (!val.Val.has_value ()) {
        return val;
    }
    resolveType (&val.Val->Type);
    resolveType (expectedType);
    if (val.Val->Type == nullptr || *expectedType == nullptr) {
        return val;
    }

    Type *src = val.Val->Type;
    Type *dst = *expectedType;

    if (*src == *dst) {
        return val;
    }
    if (!canImplicitCast (val, expectedType)) {
        _diag
            .Report (
                DiagCode::ECannotCast,
                "cannot implicitly cast '" + typeToString (src) + "' to '"
                    + typeToString (dst) + "'",
                Severity::Error)
            .AddSpan (start, end);

        return {};
    }

    auto kind = hir::CastKind::BitCast;
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

            if (targetType->IsIntOrSize ()) {
                unsigned bits       = targetType->IsInteger ()
                                          ? targetType->AsInteger ()->BitWidth ()
                                          : 32;
                bool     isUnsigned = targetType->IsInteger ()
                                          ? targetType->AsInteger ()->IsUnsigned ()
                                          : targetType->AsSize ()->IsUnsigned ();

                if (isUnsigned) {
                    if (arg < 0) {
                        return false;
                    }
                    auto uval = static_cast<uint64_t> (arg);
                    auto umax = (bits == 64) ? ~0ULL : (1ULL << bits) - 1;
                    return uval <= umax;
                }
                auto min = -static_cast<int64_t> ((1ULL << (bits - 1)));
                auto max = static_cast<uint64_t> ((1ULL << (bits - 1))) - 1;
                return arg >= min && static_cast<uint64_t> (arg) <= max;
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
        auto  &err = _diag
                         .Report (
                             DiagCode::ECallIsAmbiguous,
                             "call to '" + bestCand->Name.Val + "' is ambiguous",
                             Severity::Error)
                         .AddSpan (start, end, "ambiguously call");
        size_t i   = 1;
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
        auto  &err = _diag
                         .Report (
                             DiagCode::ECallIsAmbiguous,
                             "call to '" + bestCand->Func->Name.Val + "' is ambiguous",
                             Severity::Error)
                         .AddSpan (start, end, "ambiguously call");
        size_t i   = 1;
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
    if (src->IsStruct () && dst->IsTrait ()) {
        auto *sSym = src->AsStruct ()->BaseSymbol ();
        auto *tSym = dst->AsTrait ()->BaseSymbol ();
        return sSym->TraitsImplements.contains (tSym) ? CastCost::TraitMatch
                                                      : CastCost::Incompatible;
    }

    return CastCost::Incompatible;
}

bool
Sema::canApplyAsgnOp (AsgnOp op, Type *type) {
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
Sema::allowInScope (Stmt *stmt, bool allowInGlobal) {
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
    basic::Type           *base,
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
                "method '" + mcName.Val + "' of type '" + typeToString (base)
                    + "' is private",
                Severity::Error)
            .AddSpan (mcName.Start, mcName.End);
        return false;
    }
    return true;
}

std::string
Sema::typeToString (Type *type) {
    if (type == nullptr) {
        return "";
    }
    if (type->IsPointer ()) {
        return '*' + typeToString (type->AsPointer ()->Base ());
    }
    if (!type->IsStruct () && !type->IsTrait ()) {
        return type->ToString ();
    }
    std::string res    = type->IsStruct () ? type->AsStruct ()->BaseSymbol ()->Name.Val
                                           : type->AsTrait ()->BaseSymbol ()->Name.Val;
    Module     *curMod = type->IsStruct () ? type->AsStruct ()->BaseSymbol ()->Parent
                                           : type->AsTrait ()->BaseSymbol ()->Parent;
    while (curMod != nullptr) {
        if (*curMod == *_mod) {
            break;
        }
        res = curMod->Name + '.' + res; // NOLINT
    }
    return res;
}

void
Sema::analyzeMethodCallOnConstBase (MethodCall *mc, symbols::Method *method) {
    if (!method->IsConst) {
        _diag
            .Report (
                DiagCode::ECannotCallMethodOnImmutableReceiver,
                "method '" + mc->Name ().Val
                    + "' cannot be called on an immutable receiver",
                Severity::Error)
            .AddSpan (mc->Name ().Start, mc->Name ().End);
        return;
    }
}
}
