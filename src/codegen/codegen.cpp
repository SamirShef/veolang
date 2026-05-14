#include "basic/types/floating.h"

#include <basic/types/integer.h>
#include <codegen/codegen.h>

namespace veo {

void
CodeGen::generate (Node *node) {
#define variant(kind, func, type)                                                        \
    case NodeKind::kind: func (llvm::cast<type> (node)); break;
    if (node == nullptr) {
        return;
    }
    switch (node->Kind ()) {
        variant (VarDef, generateVarDef, VarDef);
        variant (Func, generateFuncDef, Function);
        variant (Ret, generateRet, Return);
    default: return;
    }
#undef variant
}

void
CodeGen::generateVarDef (VarDef *vd) {}

void
CodeGen::generateFuncDef (Function *fd) {}

void
CodeGen::generateRet (Return *ret) {}

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

llvm::Value *
CodeGen::generateBinaryExpr (BinaryExpr *be) {
    auto       *lhs        = generateExpr (be->Lhs ());
    auto       *rhs        = generateExpr (be->Rhs ());
    llvm::Type *commonType = getType (be->CommonType ());
    return nullptr;
}

llvm::Value *
CodeGen::generateUnaryExpr (UnaryExpr *ue) {
    return nullptr;
}

llvm::Value *
CodeGen::generateLoadVar (LoadVar *lv) {
    return nullptr;
}

llvm::Value *
CodeGen::generateLValue (Node *node) {
    return nullptr;
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
    return "";
}

std::string
CodeGen::mangleGlobalVar (VarDef *var) const {
    return "";
}

}
