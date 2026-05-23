#include <basic/symbols/module.h>
#include <basic/types/all.h>
#include <codegen/codegen.h>
#include <sstream>

namespace veo {

using namespace hir;

std::string
CodeGen::MangleStaticField (symbols::Struct *sym, const std::string &fieldName) {
    std::ostringstream oss;
    oss << "_VF";
    oss << mangleModule (sym->Parent);
    oss << "E" << sym->Name.Val.size () << sym->Name.Val;
    oss << "E" << fieldName.size () << fieldName;
    return oss.str ();
}

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
    std::string name     = isGlobal ? mangleGlobalVar (vd) : vd->Name ().Val;
    auto       *init     = vd->Init () != nullptr
                               ? generateExpr (vd->Init ())
                               : llvm::ConstantExpr::getNullValue (getType (vd->Type ()));
    auto       *type     = getType (vd->Type ());
    if (isGlobal) {
        auto *gv = new llvm::GlobalVariable (
            *_mod,
            type,
            vd->IsConst (),
            llvm::GlobalValue::ExternalLinkage,
            llvm::cast<llvm::Constant> (init),
            name);
        _globals.emplace_back (gv);
    } else {
        auto *alloca = _builder.CreateAlloca (type, nullptr, name);
        _builder.CreateStore (init, alloca);
        _curFunc->Locals.emplace_back (alloca);
    }
}

void
CodeGen::declareFunc (Function *fd) {
    const std::string &name
        = fd->MethodBaseType () == nullptr
              ? mangleFunction (fd)
              : mangleMethod (fd->MethodBaseType ()->AsStruct ()->BaseSymbol (), fd);
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
    _curFunc = CurrentFunction ();
    const std::string &name
        = fd->MethodBaseType () == nullptr
              ? mangleFunction (fd)
              : mangleMethod (fd->MethodBaseType ()->AsStruct ()->BaseSymbol (), fd);
    auto  *func = _mod->getFunction (name);
    size_t i    = 0;
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
    const auto               &name = mangleStruct (sd);
    std::vector<llvm::Type *> fields;
    fields.reserve (sd->Fields ().size ());
    for (const auto &field : sd->Fields ()) {
        auto *type = getType (field.Type);
        if (!field.IsStatic) {
            fields.emplace_back (type);
        } else {
            const auto &fieldName = MangleStaticField (sd->BaseSymbol (), field.Name.Val);
            auto       *gv        = new llvm::GlobalVariable (
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
CodeGen::generateFieldExpr (hir::FieldExpr *fe) {
    auto *base = generateExpr (fe->Base ());
    return _builder.CreateExtractValue (base, fe->Index ());
}

llvm::Value *
CodeGen::generateStructInstance (hir::StructInstance *si) {
    const auto       *sym = si->BaseSymbol ();
    llvm::StructType *type
        = llvm::StructType::getTypeByName (_ctx, mangleStructSymbol (si->BaseSymbol ()));
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
CodeGen::generateLoadGlobalVarByName (hir::LoadGlobalVarByName *load) {
    auto *lvalue = generateLValue (load);
    return _builder.CreateLoad (getType (load->Type ()), lvalue);
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
    default: return nullptr;
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
            mangleStructSymbol (type->AsStruct ()->BaseSymbol ()));
    default: {
    }
    }
    return nullptr;
}

std::string
CodeGen::mangleFunction (Function *func) {
    auto              *sym = func->BaseSymbol ();
    std::ostringstream oss;
    oss << "_VF";
    oss << mangleModule (sym->Parent);
    oss << "E" << sym->Name.Val.size () << sym->Name.Val << "I";
    for (auto &a : sym->Args) {
        oss << mangleType (a.Type);
    }
    oss << "E";
    return oss.str ();
}

std::string
CodeGen::mangleMethod (symbols::Struct *sym, hir::Function *func) {
    std::ostringstream oss;
    oss << "_VM";
    oss << mangleModule (sym->Parent);
    oss << "E" << sym->Name.Val.size () << sym->Name.Val;
    oss << "E" << func->Name ().Val.size () << func->Name ().Val << "I";
    for (auto &a : func->Args ()) {
        oss << mangleType (a.Type);
    }
    oss << "E";
    return oss.str ();
}

std::string
CodeGen::mangleGlobalVar (VarDef *var) {
    auto              *sym = var->BaseSymbol ();
    std::ostringstream oss;
    oss << "_VG";
    oss << mangleModule (sym->Parent);
    oss << "E" << sym->Name.Val.size () << sym->Name.Val;
    return oss.str ();
}

std::string
CodeGen::mangleStruct (hir::StructDef *sd) {
    return mangleStructSymbol (sd->BaseSymbol ());
}

std::string
CodeGen::mangleStructSymbol (symbols::Struct *sym) {
    std::ostringstream oss;
    oss << "_VS";
    oss << mangleModule (sym->Parent);
    oss << "E" << sym->Name.Val.size () << sym->Name.Val;
    return oss.str ();
}

std::string
CodeGen::mangleModule (symbols::Module *mod) {
    if (mod == nullptr) {
        return "";
    }
    std::ostringstream oss;
    oss << mangleModule (mod->Parent);
    oss << mod->Name.size () << mod->Name;
    return oss.str ();
}

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
std::string
CodeGen::mangleType (basic::Type *type) {
    switch (type->Kind ()) {
    case basic::TypeKind::Integer: {
        auto *it = llvm::cast<basic::IntegerType> (type);
        switch (it->BitWidth ()) {
        case 16: return "s";
        case 32: return "i";
        case 64: return "l";
        default: return "";
        }
    }
    case basic::TypeKind::Floating:
        return llvm::cast<basic::FloatingType> (type)->IsFloat () ? "f" : "d";
    case basic::TypeKind::Bool: return "b";
    case basic::TypeKind::Char: return "c";
    case basic::TypeKind::Struct: {
        auto *sym = type->AsStruct ()->BaseSymbol ();
        return "S" + std::to_string (sym->Name.Val.size ()) + sym->Name.Val;
    }
    default: {
    }
    }
    return "";
}
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers)

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
    auto *call = _builder.CreateCall (_userMain->LLVM, args);
    _builder.CreateRet (call);
}

}
