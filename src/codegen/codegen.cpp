#include <basic/symbols/module.h>
#include <basic/types/all.h>
#include <codegen/codegen.h>
#include <codegen/mangler.h>

namespace veo {

using namespace hir;

void
CodeGen::generate (Node *node) {
#define variant(kind, func, type)                                                        \
    case NodeKind::kind: func (llvm::cast<type> (node)); break;
    switch (node->Kind ()) {
        variant (VarDef, generateVarDef, VarDef);
        variant (Func, generateFuncDef, Function);
        variant (Ret, generateRet, Return);
        variant (ExprStmt, generateExprStmt, ExprStmt);
        variant (Branch, generateBranch, Branch);
        variant (StructDef, generateStruct, StructDef);
    default: return;
    }
#undef variant
}

void
CodeGen::generateBasicBlock (BasicBlock *bb) {
    auto *block = _basicBlocksMap.at (bb);
    _builder.SetInsertPoint (block);
    for (auto &node : bb->Body ()) {
        generate (node);
    }
}

void
CodeGen::generateVarDef (VarDef *vd) {
    bool        isGlobal = !_curFunc.has_value ();
    std::string name     = isGlobal ? Mangler::MangleGlobalVar (vd) : vd->Name ().Val;
    auto       *type     = getType (vd->Type ());
    if (isGlobal) {
        auto *gv = new llvm::GlobalVariable (
            *_mod,
            type,
            vd->IsConst (),
            llvm::GlobalValue::ExternalLinkage,
            llvm::ConstantExpr::getNullValue (type),
            name);
        _globals.emplace_back (gv);
    } else {
        auto *init   = vd->Init () != nullptr
                           ? generateExpr (vd->Init ())
                           : llvm::ConstantExpr::getNullValue (getType (vd->Type ()));
        auto *alloca = _builder.CreateAlloca (type, nullptr, name);
        _builder.CreateStore (init, alloca);
        _curFunc->Locals.emplace_back (alloca);
    }
}

void
CodeGen::declareFunc (Function *fd) {
    const std::string &name = fd->MethodBaseType () == nullptr
                                  ? Mangler::MangleFunction (fd)
                                  : Mangler::MangleMethod (
                                        fd->MethodBaseType ()->AsStruct ()->BaseSymbol (),
                                        fd);
    std::vector<llvm::Type *> args;
    for (auto &a : fd->Args ()) {
        args.emplace_back (getType (a.Type));
    }
    auto *funcType = llvm::FunctionType::get (getType (fd->RetType ()), args, false);
    auto *func     = llvm::Function::Create (
        funcType,
        llvm::GlobalValue::ExternalLinkage,
        name,
        *_mod);
    _funcs.emplace_back (func);
    if (fd->MethodBaseType () == nullptr) {
        _funcsMap.emplace (fd->BaseSymbol (), func);
    } else {
        _methodsMap.emplace (fd->BaseSymbol (), func);
    }
}

void
CodeGen::generateFuncDef (Function *fd) {
    _curFunc                = CurrentFunction ();
    const std::string &name = fd->MethodBaseType () == nullptr
                                  ? Mangler::MangleFunction (fd)
                                  : Mangler::MangleMethod (
                                        fd->MethodBaseType ()->AsStruct ()->BaseSymbol (),
                                        fd);
    auto              *func = _mod->getFunction (name);
    size_t             i    = 0;
    for (auto &a : func->args ()) {
        a.setName (fd->Args ()[i].Name.Val);
        _curFunc->Locals.emplace_back (&a);
        ++i;
    }

    for (auto &bb : fd->Body ()) {
        // declaration of basic blocks
        auto *block = llvm::BasicBlock::Create (_ctx, bb->Name (), func);
        _basicBlocksMap.emplace (bb, block);
    }
    for (auto &bb : fd->Body ()) {
        // definition of basic blocks
        generateBasicBlock (bb);
    }
    _curFunc = std::nullopt;

    if (fd->Name ().Val == "main" && fd->BaseSymbol ()->Parent != nullptr
        && fd->BaseSymbol ()->Parent->Parent == nullptr) {
        _userMain = { .HIR = fd, .LLVM = func };
    }
}

void
CodeGen::generateRet (Return *ret) {
    if (ret->Expr () == nullptr) {
        _builder.CreateRetVoid ();
        return;
    }
    auto *val = generateExpr (ret->Expr ());
    _builder.CreateRet (val);
}

void
CodeGen::generateExprStmt (ExprStmt *es) {
    generateExpr (es->Expr ());
}

void
CodeGen::generateBranch (Branch *br) {
    if (br->Cond () != nullptr) {
        auto *thenBB = _basicBlocksMap.at (br->Then ());
        auto *elseBB = _basicBlocksMap.at (br->Else ());
        _builder.CreateCondBr (generateExpr (br->Cond ()), thenBB, elseBB);
    } else {
        auto *bb = _basicBlocksMap.at (br->Then ());
        _builder.CreateBr (bb);
    }
}

void
CodeGen::generateStruct (hir::StructDef *sd) {
    const auto               &name = Mangler::MangleStruct (sd);
    std::vector<llvm::Type *> fields;
    fields.reserve (sd->Fields ().size ());
    for (const auto &field : sd->Fields ()) {
        auto *type = getType (field.Type);
        if (!field.IsStatic) {
            fields.emplace_back (type);
        } else {
            const auto &fieldName
                = Mangler::MangleStaticField (sd->BaseSymbol (), field.Name.Val);
            auto *gv = new llvm::GlobalVariable (
                *_mod,
                type,
                field.IsConst,
                llvm::GlobalValue::ExternalLinkage,
                llvm::ConstantExpr::getNullValue (type),
                fieldName);
            _globals.emplace_back (gv);
        }
    }
    llvm::StructType::create (_ctx, fields, name);
}

llvm::Value *
CodeGen::generateExpr (Node *node) {
#define variant(kind, func, type)                                                        \
    case NodeKind::kind: return func (llvm::cast<type> (node));
    if (node == nullptr) {
        return nullptr;
    }
    switch (node->Kind ()) {
        variant (LitExpr, generateLiteralExpr, LiteralExpr);
        variant (BinExpr, generateBinaryExpr, BinaryExpr);
        variant (UnExpr, generateUnaryExpr, UnaryExpr);
        variant (LoadVar, generateLoadVar, LoadVar);
        variant (FuncCall, generateFuncCall, FuncCall);
        variant (Store, generateStore, Store);
        variant (FieldExpr, generateFieldExpr, FieldExpr);
        variant (StructInstance, generateStructInstance, StructInstance);
        variant (LoadGlobalVarByName, generateLoadGlobalVarByName, LoadGlobalVarByName);
        variant (TernaryExpr, generateTernaryExpr, TernaryExpr);
        variant (Cast, generateCast, Cast);
        variant (RefExpr, generateRefExpr, RefExpr);
        variant (DerefExpr, generateDerefExpr, DerefExpr);
        variant (NilExpr, generateNilExpr, NilExpr);
    default: return nullptr;
    }
#undef variant
}

llvm::Value *
CodeGen::generateLiteralExpr (LiteralExpr *le) {
    const auto &val = le->Value ();
    switch (val.Type->Kind ()) {
    case basic::TypeKind::Integer:
        return _builder.getIntN (
            val.Type->AsInteger ()->BitWidth (),
            std::get<0> (val.Data));
    case basic::TypeKind::Floating:
        return llvm::ConstantFP::get (
            val.Type->AsFloating ()->IsFloat () ? _builder.getFloatTy ()
                                                : _builder.getDoubleTy (),
            std::get<1> (val.Data));
    case basic::TypeKind::Bool: return _builder.getInt1 (std::get<0> (val.Data) != 0);
    case basic::TypeKind::Char: return _builder.getInt32 (std::get<0> (val.Data));
    default: {
    }
    }
    return nullptr;
}

// NOLINTBEGIN(readability-function-cognitive-complexity)
llvm::Value *
CodeGen::generateBinaryExpr (BinaryExpr *be) {
    auto *lhs        = generateExpr (be->Lhs ());
    auto *rhs        = generateExpr (be->Rhs ());
    auto *commonType = be->CommonType ();
    bool isUnsigned = commonType->IsInteger () && commonType->AsInteger ()->IsUnsigned ();
    switch (be->Op ()) {
    case ast::BinOp::Plus:
        if (commonType->IsFloating ()) {
            return _builder.CreateFAdd (lhs, rhs);
        }
        return _builder.CreateAdd (lhs, rhs);
    case ast::BinOp::Minus:
        if (commonType->IsFloating ()) {
            return _builder.CreateFSub (lhs, rhs);
        }
        return _builder.CreateSub (lhs, rhs);
    case ast::BinOp::Mul:
        if (commonType->IsFloating ()) {
            return _builder.CreateFMul (lhs, rhs);
        }
        return _builder.CreateMul (lhs, rhs);
    case ast::BinOp::Div:
        if (commonType->IsFloating ()) {
            return _builder.CreateFDiv (lhs, rhs);
        }
        if (isUnsigned) {
            return _builder.CreateUDiv (lhs, rhs);
        }
        return _builder.CreateSDiv (lhs, rhs);
    case ast::BinOp::Rem:
        if (commonType->IsFloating ()) {
            return _builder.CreateFRem (lhs, rhs);
        }
        if (isUnsigned) {
            return _builder.CreateURem (lhs, rhs);
        }
        return _builder.CreateSRem (lhs, rhs);
    case ast::BinOp::Eq:
        if (commonType->IsFloating ()) {
            return _builder.CreateFCmpOEQ (lhs, rhs);
        }
        return _builder.CreateICmpEQ (lhs, rhs);
    case ast::BinOp::NEq:
        if (commonType->IsFloating ()) {
            return _builder.CreateFCmpONE (lhs, rhs);
        }
        return _builder.CreateICmpNE (lhs, rhs);
    case ast::BinOp::Lt:
        if (commonType->IsFloating ()) {
            return _builder.CreateFCmpOLT (lhs, rhs);
        }
        if (isUnsigned) {
            return _builder.CreateICmpULT (lhs, rhs);
        }
        return _builder.CreateICmpSLT (lhs, rhs);
    case ast::BinOp::LtEq:
        if (commonType->IsFloating ()) {
            return _builder.CreateFCmpOLE (lhs, rhs);
        }
        if (isUnsigned) {
            return _builder.CreateICmpULE (lhs, rhs);
        }
        return _builder.CreateICmpSLE (lhs, rhs);
    case ast::BinOp::Gt:
        if (commonType->IsFloating ()) {
            return _builder.CreateFCmpOGT (lhs, rhs);
        }
        if (isUnsigned) {
            return _builder.CreateICmpUGT (lhs, rhs);
        }
        return _builder.CreateICmpSGT (lhs, rhs);
    case ast::BinOp::GtEq:
        if (commonType->IsFloating ()) {
            return _builder.CreateFCmpOGE (lhs, rhs);
        }
        if (isUnsigned) {
            return _builder.CreateICmpUGE (lhs, rhs);
        }
        return _builder.CreateICmpSGE (lhs, rhs);
    case ast::BinOp::BitAnd: return _builder.CreateAnd (lhs, rhs);
    case ast::BinOp::BitOr: return _builder.CreateOr (lhs, rhs);
    case ast::BinOp::BitXor: return _builder.CreateXor (lhs, rhs);
    case ast::BinOp::LogAnd: return _builder.CreateLogicalAnd (lhs, rhs);
    case ast::BinOp::LogOr: return _builder.CreateLogicalOr (lhs, rhs);
    default: return nullptr;
    }
}
// NOLINTEND(readability-function-cognitive-complexity)

llvm::Value *
CodeGen::generateUnaryExpr (UnaryExpr *ue) {
    auto *rhs        = generateExpr (ue->Rhs ());
    auto *commonType = ue->CommonType ();
    switch (ue->Op ()) {
    case ast::UnOp::Minus:
        if (commonType->IsFloating ()) {
            return _builder.CreateFNeg (rhs);
        }
        return _builder.CreateNeg (rhs);
    case ast::UnOp::Not: return _builder.CreateNot (rhs);
    case ast::UnOp::Inverse:
        return _builder.CreateXor (
            rhs,
            _builder.getIntN (rhs->getType ()->getIntegerBitWidth (), -1));
    default: return nullptr;
    }
}

llvm::Value *
CodeGen::generateLoadVar (LoadVar *lv) {
    auto *lvalue = generateLValue (lv);
    if (!_curFunc.has_value ()) {
        return llvm::cast<llvm::GlobalVariable> (lvalue)->getInitializer ();
    }
    if (auto *arg = llvm::dyn_cast<llvm::Argument> (lvalue)) {
        return arg;
    }
    return _builder.CreateLoad (getType (lv->Type ()), lvalue);
}

llvm::Value *
CodeGen::generateFuncCall (FuncCall *fc) {
    auto                      *func = fc->IsMethod () ? _methodsMap.at (fc->Function ())
                                                      : _funcsMap.at (fc->Function ());
    std::vector<llvm::Value *> args;
    args.reserve (fc->Args ().size ());
    for (const auto &a : fc->Args ()) {
        args.emplace_back (generateExpr (a));
    }
    return _builder.CreateCall (func, args);
}

llvm::Value *
CodeGen::generateStore (Store *store) {
    auto *lvalue = generateLValue (store->Ptr ());
    auto *val    = generateExpr (store->Expr ());
    _builder.CreateStore (val, lvalue);
    return val;
}

llvm::Value *
CodeGen::generateFieldExpr (FieldExpr *fe) {
    auto *base = generateExpr (fe->Base ());
    return _builder.CreateExtractValue (base, fe->Index ());
}

llvm::Value *
CodeGen::generateStructInstance (StructInstance *si) {
    const auto       *sym  = si->BaseSymbol ();
    llvm::StructType *type = llvm::StructType::getTypeByName (
        _ctx,
        Mangler::MangleStructSymbol (si->BaseSymbol ()));
    if (_curFunc.has_value ()) {
        // local scope
        auto                      *structAlloca = _builder.CreateAlloca (type);
        std::vector<llvm::Value *> fields (sym->Fields.size ());
        llvm::Value               *indices[2] = {
            _builder.getInt64 (0) // get the pointer to the struct
        };
        for (const auto &[index, expr] : si->Fields ()) {
            indices[1] = _builder.getInt64 (index); // fields index
            auto *val  = generateExpr (expr);
            auto *gep  = _builder.CreateInBoundsGEP (type, structAlloca, indices);
            _builder.CreateStore (val, gep);
        }
        return _builder.CreateLoad (type, structAlloca);
    }
    // global scope
    std::vector<llvm::Constant *> fields (sym->Fields.size ());
    for (const auto &[index, expr] : si->Fields ()) {
        auto *val     = generateExpr (expr);
        fields[index] = llvm::cast<llvm::Constant> (val);
    }
    for (size_t i = 0; i < fields.size (); ++i) {
        if (fields[i] == nullptr) {
            fields[i] = llvm::Constant::getNullValue (getType (sym->Fields[i].Type));
        }
    }
    return llvm::ConstantStruct::get (type, fields);
}

llvm::Value *
CodeGen::generateLoadGlobalVarByName (LoadGlobalVarByName *load) {
    auto *lvalue = generateLValue (load);
    return _builder.CreateLoad (getType (load->Type ()), lvalue);
}

llvm::Value *
CodeGen::generateTernaryExpr (TernaryExpr *te) {
    auto *trueBB  = _basicBlocksMap.at (te->TrueBB ());
    auto *falseBB = _basicBlocksMap.at (te->FalseBB ());
    auto *phi     = _builder.CreatePHI (getType (te->Type ()), 2);
    phi->addIncoming (generateExpr (te->TrueVal ()), trueBB);
    phi->addIncoming (generateExpr (te->FalseVal ()), falseBB);
    return phi;
}

llvm::Value *
CodeGen::generateCast (Cast *cast) {
    switch (cast->GetCastKind ()) {
#define variant(kind)                                                                    \
    case CastKind::kind:                                                                 \
        return _builder.Create##kind (                                                   \
            generateExpr (cast->Expr ()),                                                \
            getType (cast->Type ()));
        variant (UIToFP);
        variant (SIToFP);
        variant (FPToUI);
        variant (FPToSI);
        variant (SExt);
        variant (ZExt);
        variant (FPTrunc);
        variant (Trunc);
        variant (BitCast);
        variant (FPExt);
#undef variant
    default: return nullptr;
    }
}

llvm::Value *
CodeGen::generateRefExpr (hir::RefExpr *re) {
    return generateLValue (re->Expr ());
}

llvm::Value *
CodeGen::generateDerefExpr (hir::DerefExpr *de) {
    auto *ptr = generateExpr (de->Expr ());
    generateCheckNil (ptr, de->Start ());
    return _builder.CreateLoad (getType (de->Type ()), ptr);
}

llvm::Value *
CodeGen::generateNilExpr (hir::NilExpr *ne) {
    return llvm::ConstantPointerNull::get (_builder.getPtrTy ());
}

llvm::Value *
CodeGen::generateLValue (Node *node) {
    if (node == nullptr) {
        return nullptr;
    }
    switch (node->Kind ()) {
    case NodeKind::LoadVar: {
        auto *lv = llvm::cast<LoadVar> (node);
        if (lv->IsGlobal ()) {
            return _globals[lv->Id ()];
        }
        return _curFunc->Locals[lv->Id ()];
    }
    case NodeKind::FieldExpr: {
        auto *fe   = llvm::cast<FieldExpr> (node);
        auto *base = generateExpr (fe->Base ());   // LOADED struct
        auto *ptr  = generateLValue (fe->Base ()); // POINTER to struct
        return _builder.CreateStructGEP (base->getType (), ptr, fe->Index ());
    }
    case NodeKind::LoadGlobalVarByName: {
        auto *load = llvm::cast<LoadGlobalVarByName> (node);
        return _mod->getNamedGlobal (load->Name ());
    }
    case NodeKind::DerefExpr: {
        return generateExpr (llvm::cast<DerefExpr> (node)->Expr ());
    }
    default: {
        auto *val       = generateExpr (node);
        auto *tmpAlloca = _builder.CreateAlloca (val->getType ());
        _builder.CreateStore (val, tmpAlloca);
        return tmpAlloca;
    }
    }
}

llvm::Type *
CodeGen::getType (basic::Type *type) {
    if (type == nullptr) {
        return _builder.getVoidTy ();
    }

    switch (type->Kind ()) {
    case basic::TypeKind::Integer:
        return _builder.getIntNTy (type->AsInteger ()->BitWidth ());
    case basic::TypeKind::Floating:
        return type->AsFloating ()->IsFloat () ? _builder.getFloatTy ()
                                               : _builder.getDoubleTy ();
    case basic::TypeKind::Bool: return _builder.getInt1Ty ();
    case basic::TypeKind::Char: return _builder.getInt32Ty ();
    case basic::TypeKind::Struct:
        return llvm::StructType::getTypeByName (
            _ctx,
            Mangler::MangleStructSymbol (type->AsStruct ()->BaseSymbol ()));
    case basic::TypeKind::Pointer: return _builder.getPtrTy ();
    default: {
    }
    }
    return nullptr;
}

void
CodeGen::generateInitFunction () {
    const std::string  &name     = "__veo_init_mod_" + _semaMod->ToString ();
    llvm::FunctionType *funcType = llvm::FunctionType::get (_builder.getVoidTy (), false);
    llvm::Function     *func     = llvm::Function::Create (
        funcType,
        llvm::GlobalValue::ExternalLinkage,
        name,
        *_mod);
    llvm::BasicBlock *entry = llvm::BasicBlock::Create (_ctx, "entry", func);
    _builder.SetInsertPoint (entry);
    for (const auto *global : _hirGlobals) {
        const std::string &name = Mangler::MangleGlobalVar (global);
        auto              *gv   = _mod->getGlobalVariable (name);
        if (gv->getInitializer () == nullptr) { // is declaration
            continue;
        }
        auto *val = generateExpr (global->Init ());
        _builder.CreateStore (val, gv);
    }
    _builder.CreateRetVoid ();
}

void
CodeGen::generateImplicitMain () {
    auto                     *sym = _userMain->HIR->BaseSymbol ();
    std::vector<llvm::Type *> argsTypes;
    argsTypes.reserve (sym->Args.size ());
    for (auto &a : sym->Args) {
        argsTypes.emplace_back (getType (a.Type));
    }
    auto *mainType = llvm::FunctionType::get (getType (sym->RetType), argsTypes, false);
    auto *main     = llvm::Function::Create (
        mainType,
        llvm::GlobalValue::ExternalLinkage,
        "main",
        *_mod);
    std::vector<llvm::Value *> args;
    size_t                     i = 0;
    for (auto &a : main->args ()) {
        a.setName (sym->Args[i].Name.Val);
        args.emplace_back (&a);
        ++i;
    }
    auto *entry = llvm::BasicBlock::Create (_ctx, "entry", main);
    _builder.SetInsertPoint (entry);

    auto *initMod = _mod->getFunction ("__veo_init_mod_" + _semaMod->ToString ());
    if (initMod != nullptr) {
        auto *callInitMod = _builder.CreateCall (initMod, {}); // call init mod
    }

    auto *callMain = _builder.CreateCall (_userMain->LLVM, args); // call user main
    _builder.CreateRet (callMain);
}

void
CodeGen::generateRuntime () {
    auto *abortFuncType = llvm::FunctionType::get (_builder.getVoidTy (), false);
    auto *abortFunc     = llvm::Function::Create (
        abortFuncType,
        llvm::GlobalValue::ExternalLinkage,
        "abort",
        *_mod);

    auto *printfFuncType = llvm::FunctionType::get (
        _builder.getInt32Ty (),
        { _builder.getPtrTy () },
        true);
    auto *printfFunc = llvm::Function::Create (
        printfFuncType,
        llvm::GlobalValue::ExternalLinkage,
        "printf",
        *_mod);
}

void
CodeGen::generateCheckNil (llvm::Value *ptr, llvm::SMLoc start) {
    auto *parent  = _builder.GetInsertBlock ()->getParent ();
    auto *cond    = _builder.CreateIsNull (ptr);
    auto *thenBB  = llvm::BasicBlock::Create (_ctx, "then", parent);
    auto *mergeBB = llvm::BasicBlock::Create (_ctx, "merge", parent);
    _builder.CreateCondBr (cond, thenBB, mergeBB);

    _builder.SetInsertPoint (thenBB);
    auto               bufferId   = _srcMgr.FindBufferContainingLoc (start);
    const auto        &buffer     = _srcMgr.getBufferInfo (bufferId);
    const std::string &bufferName = buffer.Buffer->getBufferIdentifier ().str ();
    auto               lineCol    = _srcMgr.getLineAndColumn (start);
    auto              *msg        = _builder.CreateGlobalString (
        "Null pointer dereferencing at: " + bufferName + ":"
        + std::to_string (lineCol.first) + ":" + std::to_string (lineCol.second) + '\n');
    auto *printfFunc = _mod->getFunction ("printf");
    _builder.CreateCall (printfFunc, { msg });
    auto *abortFunc = _mod->getFunction ("abort");
    _builder.CreateCall (abortFunc, {});
    _builder.CreateUnreachable ();

    _builder.SetInsertPoint (mergeBB);
}

}
