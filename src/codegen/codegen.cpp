#include <basic/symbols/module.h>
#include <basic/types/floating.h>
#include <basic/types/integer.h>
#include <codegen/codegen.h>
#include <sstream>

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
    default: return;
    }
#undef variant
}

void
CodeGen::generateBasicBlock (BasicBlock *bb) {
    auto *block = llvm::BasicBlock::Create (
        _ctx,
        bb->Name (),
        _mod->getFunction (mangleFunction (bb->Parent ())));
    _builder.SetInsertPoint (block);
    for (auto &node : bb->Body ()) {
        generate (node);
    }
}

void
CodeGen::generateVarDef (VarDef *vd) {
    bool        isGlobal = !_curFunc.has_value ();
    std::string name     = isGlobal ? mangleGlobalVar (vd) : vd->Name ().Val;
    auto       *init     = generateExpr (vd->Init ());
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
CodeGen::generateFuncDef (Function *fd) {
    _curFunc                       = CurrentFunction ();
    std::string               name = mangleFunction (fd);
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

    size_t i = 0;
    for (auto &a : func->args ()) {
        a.setName (fd->Args ()[i].Name.Val);
        _curFunc->Locals.emplace_back (&a);
    }

    for (auto &bb : fd->Body ()) {
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
CodeGen::generateLValue (Node *node) {
    if (node == nullptr) {
        return nullptr;
    }
    switch (node->Kind ()) {
    case hir::NodeKind::LoadVar: {
        auto *lv = llvm::cast<LoadVar> (node);
        if (lv->IsGlobal ()) {
            return _globals[lv->Id ()];
        }
        return _curFunc->Locals[lv->Id ()];
    }
    default: return nullptr;
    }
}

llvm::Type *
CodeGen::getType (basic::Type *type) {
    switch (type->Kind ()) {
    case basic::TypeKind::Integer:
        return _builder.getIntNTy (type->AsInteger ()->BitWidth ());
    case basic::TypeKind::Floating:
        return type->AsFloating ()->IsFloat () ? _builder.getFloatTy ()
                                               : _builder.getDoubleTy ();
    case basic::TypeKind::Bool: return _builder.getInt1Ty ();
    case basic::TypeKind::Char: return _builder.getInt32Ty ();
    }
}

std::string
CodeGen::mangleFunction (Function *func) const {
    auto              *sym = func->BaseSymbol ();
    std::ostringstream oss;
    oss << "_BF";
    oss << mangleModule (sym->Parent);
    oss << "E" << sym->Name.Val.size () << sym->Name.Val << "I";
    for (auto &a : sym->Args) {
        oss << mangleType (a.Type);
    }
    oss << "E";
    return oss.str ();
}

std::string
CodeGen::mangleGlobalVar (VarDef *var) const {
    auto              *sym = var->BaseSymbol ();
    std::ostringstream oss;
    oss << "_BG";
    oss << mangleModule (sym->Parent);
    oss << "E" << sym->Name.Val.size () << sym->Name.Val;
    return oss.str ();
}

std::string
CodeGen::mangleModule (symbols::Module *mod) const {
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
CodeGen::mangleType (basic::Type *type) const {
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
