#include <basic/types/all.h>
#include <sema/sema.h>

namespace veo {

using namespace ast;
using namespace symbols;

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
    if (type->IsTrait ()) {
        _diag
            .Report (
                DiagCode::ECannotUseTraitForVar,
                "cannot use trait '" + typeToString (type)
                    + "' as a concrete type for variable",
                Severity::Error)
            .AddSpan (vd->Start (), vd->End ());
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
                "function '" + fd->Name ().Val + "' is already defined",
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
    if (fd->RetType ()->IsTrait ()) {
        _diag
            .Report (
                DiagCode::ECannotUseTraitForFunc,
                "cannot use trait '" + typeToString (fd->RetType ())
                    + "' as a concrete return type",
                Severity::Error)
            .AddSpan (fd->Start (), fd->End ());
        return;
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

    auto it = _mod->Funcs.find (fd->Name ().Val);
    if (it == _mod->Funcs.end ()) {
        return;
    }
    auto     *candidates = &it->second;
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
        auto res = analyzeExpr (ret->RetExpr (), _funcRetTypes.top ());
        if (!res.Val.has_value ()) {
            return;
        }
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
        if (field.Type->IsTrait ()) {
            _diag
                .Report (
                    DiagCode::ECannotUseTraitForVar,
                    "cannot use trait '" + typeToString (field.Type)
                        + "' as a concrete type for field",
                    Severity::Error)
                .AddSpan (field.Name.Start, field.Name.End);
            continue;
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
    pushTypeScope ();
    auto *thisAlias
        = createType<AliasType> (NameObj ("This", is->Start (), is->End ()), targetType);
    registerLocalType ("This", thisAlias);
    if (traitType != nullptr && isStruct) {
        sym->TraitsImplements.emplace (traitType->AsTrait ()->BaseSymbol ());
    }
    for (auto &method : is->Methods ()) {
        declareImplMethod (method, sym, targetType);
    }

    popTypeScope ();
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
            for (const auto &traitMethod : candidates.Candidates) {
                bool             foundExactMatch = false;
                symbols::Method *looseMatch      = nullptr;
                MatchResult      bestResult;

                for (const auto &implMethod : structCandidates) {
                    auto res = checkMethodSignature (
                        traitMethod.get (),
                        implMethod.get (),
                        targetType);
                    if (res.Status == SignatureMatchResult::Success) {
                        foundExactMatch = true;
                        break;
                    }
                    looseMatch = implMethod.get ();
                    bestResult = res;
                }

                if (foundExactMatch) {
                    continue;
                }

                auto &err = _diag
                                .Report (
                                    DiagCode::EMethodSignatureMismatch,
                                    "method '" + name
                                        + "' has a mismatched signature compared to the "
                                          "trait definition",
                                    Severity::Error)

                                .AddSpan (
                                    looseMatch->Func->Name.Start,
                                    looseMatch->Func->Name.End,
                                    "mismatched method implementation here");

                switch (bestResult.Status) {
                case SignatureMatchResult::StaticMismatch:
                    err.AddNote (
                        traitMethod->IsStatic
                            ? "expected method to be 'static'"
                            : "method cannot be 'static' as defined in the trait");
                    break;
                case SignatureMatchResult::ArgCountMismatch:
                    err.AddNote (
                        "expected " + std::to_string (traitMethod->Func->Args.size ())
                        + " arguments, but found "
                        + std::to_string (looseMatch->Func->Args.size ()));
                    break;
                case SignatureMatchResult::ArgTypeMismatch: {
                    size_t      idx = bestResult.ErrorArgIndex;
                    std::string expectedName
                        = typeToString (traitMethod->Func->Args[idx].Type);
                    err.AddNote (
                        "argument " + std::to_string (idx + 1) + " ('"
                        + looseMatch->Func->Args[idx].Name.Val + "') expects type '"
                        + expectedName + "', but got '"
                        + typeToString (looseMatch->Func->Args[idx].Type) + "'");
                    break;
                }
                case SignatureMatchResult::RetTypeMismatch: {
                    std::string expectedRet = typeToString (traitMethod->Func->RetType);
                    err.AddNote (
                        "expected return type '" + expectedRet + "', but got '"
                        + typeToString (looseMatch->Func->RetType) + "'");
                    break;
                }
                default: break;
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
    if (fd->RetType ()->IsTrait ()) {
        _diag
            .Report (
                DiagCode::ECannotUseTraitForFunc,
                "cannot use trait '" + typeToString (fd->RetType ())
                    + "' as a concrete return type",
                Severity::Error)
            .AddSpan (fd->Start (), fd->End ());
        return;
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

Sema::MatchResult
Sema::checkMethodSignature (
    symbols::Method *traitMethod, symbols::Method *implMethod, Type *concreteTarget) {
    if (traitMethod->IsStatic != implMethod->IsStatic) {
        return MatchResult (SignatureMatchResult::StaticMismatch);
    }

    auto *traitFunc = traitMethod->Func.get ();
    auto *implFunc  = implMethod->Func.get ();

    if (traitFunc->Args.size () != implFunc->Args.size ()) {
        return MatchResult (SignatureMatchResult::ArgCountMismatch);
    }

    for (size_t i = 0; i < traitFunc->Args.size (); ++i) {
        if (!compareTypesWithThis (
                traitFunc->Args[i].Type,
                implFunc->Args[i].Type,
                concreteTarget)) {
            return MatchResult (SignatureMatchResult::ArgTypeMismatch, i);
        }
    }

    if (!compareTypesWithThis (traitFunc->RetType, implFunc->RetType, concreteTarget)) {
        return MatchResult (SignatureMatchResult::RetTypeMismatch);
    }

    return MatchResult (SignatureMatchResult::Success);
}

void
Sema::analyzeImplStmt (ImplStmt *is) {
    auto *targetType = is->StructType ();
    resolveType (&targetType);
    auto *sym
        = targetType->IsStruct () ? targetType->AsStruct ()->BaseSymbol () : nullptr;
    pushTypeScope ();
    auto *thisAlias
        = createType<AliasType> (NameObj ("This", is->Start (), is->End ()), targetType);
    registerLocalType ("This", thisAlias);
    for (auto &method : is->Methods ()) {
        analyzeImplMethodDef (method, sym, targetType);
    }
    popTypeScope ();
}

void
Sema::analyzeImplMethodDef (
    ast::Method method, symbols::Struct *sym, basic::Type *targetType) {
    auto *fd = method.Func;
    if (fd->IsDeclaration ()) {
        return;
    }
    symbols::MethodCandidates *candidates = nullptr;
    if (sym != nullptr) {
        auto it = sym->Methods.find (fd->Name ().Val);
        if (it == sym->Methods.end ()) {
            return;
        }
        candidates = &it->second;
    } else {
        auto methodsIt = _mod->PrimitiveMethods.find (targetType);
        if (methodsIt == _mod->PrimitiveMethods.end ()) {
            return;
        }
        auto it = methodsIt->second.find (fd->Name ().Val);
        if (it == methodsIt->second.end ()) {
            return;
        }
        candidates = &it->second;
    }
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
    if (fd->RetType () == nullptr || fd->RetType ()->IsNoth ()) {
        _builder.CreateRet (nullptr, fd->Name ().Start, fd->Name ().End);
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
    pushTypeScope ();
    auto *traitSymbol  = &_mod->Traits.at (ts->Name ().Val);
    auto *abstractThis = createType<TraitThisType> (traitSymbol);
    registerLocalType ("This", abstractThis);

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
            continue;
        }
        if (!fd->IsDeclaration ()) {
            _diag
                .Report (
                    DiagCode::ETraitMethodsCannotHaveBody,
                    "trait methods cannot have a body",
                    Severity::Error)
                .AddSpan (fd->Start (), fd->End ());
            continue;
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
    popTypeScope ();
}

}
