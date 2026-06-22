#pragma once
#include <basic/name.h>
#include <basic/types/type.h>
#include <hir/basic_block.h>
#include <hir/bin_expr.h>
#include <hir/branch.h>
#include <hir/cast.h>
#include <hir/context.h>
#include <hir/deref.h>
#include <hir/expr_stmt.h>
#include <hir/field_expr.h>
#include <hir/func_call.h>
#include <hir/lit_expr.h>
#include <hir/load_glob_var_by_name.h>
#include <hir/load_var.h>
#include <hir/nil.h>
#include <hir/ptr_arith.h>
#include <hir/ref.h>
#include <hir/ret.h>
#include <hir/store.h>
#include <hir/struct_def.h>
#include <hir/struct_instance.h>
#include <hir/un_expr.h>

namespace veo::hir {

class Builder {
    Context    &_ctx; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
    BasicBlock *_insertBlock = nullptr;

public:
    explicit Builder (Context &ctx) : _ctx (ctx) {}

    Context &
    GetContext () {
        return _ctx;
    }

    Function *
    Parent () const {
        return _insertBlock->Parent ();
    }

    BasicBlock *
    InsertBlock () const {
        return _insertBlock;
    }

    void
    SetInsertionPoint (BasicBlock *bb) {
        _insertBlock = bb;
    }

    BasicBlock *
    CreateBasicBlock (Function *parent, std::string name = "") {
        auto *node = _ctx.CreateNode<BasicBlock> (parent, std::move (name));
        if (parent != nullptr) {
            parent->Body ().push_back (node);
        }
        return node;
    }

    void
    AddToBlock (Node *node) {
        if (_insertBlock == nullptr) {
            return;
        }
        _insertBlock->AddInst (node);
    }

    Function *
    CreateFunction (
        basic::NameObj        name,
        basic::Type          *retType,
        std::vector<VarDef *> args,
        llvm::SMLoc           start,
        llvm::SMLoc           end,
        symbols::Function    *base,
        MangleKind            mangleKind) {
        auto *node = _ctx.CreateNode<Function> (
            std::move (name),
            retType,
            std::move (args),
            start,
            end,
            base,
            mangleKind);
        _ctx.AddFunction (node);
        return node;
    }

    Function *
    CreateMethod (
        basic::NameObj        name,
        basic::Type          *retType,
        std::vector<VarDef *> args,
        llvm::SMLoc           start,
        llvm::SMLoc           end,
        symbols::Method      *base,
        MangleKind            mangleKind,
        basic::Type          *methodBaseType,
        bool                  isStatic) {
        auto *node = _ctx.CreateNode<Function> (
            std::move (name),
            retType,
            std::move (args),
            start,
            end,
            base->Func.get (),
            mangleKind,
            methodBaseType,
            isStatic);
        _ctx.AddFunction (node);
        return node;
    }

    Return *
    CreateRet (Node *expr, llvm::SMLoc start, llvm::SMLoc end) {
        auto *node = _ctx.CreateNode<Return> (expr, start, end);
        AddToBlock (node);
        return node;
    }

    VarDef *
    CreateVariable (
        basic::NameObj     name,
        basic::Type       *type,
        Node              *init,
        bool               isConst,
        bool               isGlobal,
        llvm::SMLoc        start,
        llvm::SMLoc        end,
        symbols::Variable *base,
        MangleKind         mangleKind,
        bool               isDeclaration,
        bool               addToBlock = true) {
        auto *node = _ctx.CreateNode<VarDef> (
            std::move (name),
            type,
            init,
            isConst,
            isGlobal,
            start,
            end,
            base,
            mangleKind,
            isDeclaration);
        if (isGlobal) {
            _ctx.AddGlobal (node);
        } else if (addToBlock) {
            _insertBlock->AddInst (node);
        }
        return node;
    }

    LiteralExpr *
    CreateLiteral (basic::Value val, llvm::SMLoc start, llvm::SMLoc end) {
        return _ctx.CreateNode<LiteralExpr> (std::move (val), start, end);
    }

    BinaryExpr *
    CreateBinary (
        ast::BinOp   op,
        basic::Type *commonType,
        basic::Type *resType,
        Node        *lhs,
        Node        *rhs,
        llvm::SMLoc  start,
        llvm::SMLoc  end) {
        return _ctx
            .CreateNode<BinaryExpr> (op, commonType, resType, lhs, rhs, start, end);
    }

    UnaryExpr *
    CreateUnary (
        ast::UnOp    op,
        basic::Type *commonType,
        Node        *rhs,
        llvm::SMLoc  start,
        llvm::SMLoc  end) {
        return _ctx.CreateNode<UnaryExpr> (op, commonType, rhs, start, end);
    }

    LoadVar *
    CreateLoadVar (
        VarDef      *ptr,
        basic::Type *type,
        bool         isGlobal,
        llvm::SMLoc  start,
        llvm::SMLoc  end) {
        return _ctx.CreateNode<LoadVar> (ptr, type, isGlobal, start, end);
    }

    LoadGlobalVarByName *
    CreateLoadGlobalVarByName (
        std::string name, basic::Type *type, llvm::SMLoc start, llvm::SMLoc end) {
        return _ctx.CreateNode<LoadGlobalVarByName> (std::move (name), type, start, end);
    }

    Store *
    CreateStore (
        Node *ptr, Node *expr, basic::Type *type, llvm::SMLoc start, llvm::SMLoc end) {
        return _ctx.CreateNode<Store> (ptr, expr, type, start, end);
    }

    ExprStmt *
    CreateExprStmt (Node *expr, llvm::SMLoc start, llvm::SMLoc end) {
        auto *node = _ctx.CreateNode<ExprStmt> (expr, start, end);
        AddToBlock (node);
        return node;
    }

    FuncCall *
    CreateCall (
        Function *func, std::vector<Node *> args, llvm::SMLoc start, llvm::SMLoc end) {
        return _ctx.CreateNode<FuncCall> (func, std::move (args), start, end);
    }

    Branch *
    CreateBr (
        Node       *cond,
        BasicBlock *thenBB,
        BasicBlock *elseBB,
        llvm::SMLoc start,
        llvm::SMLoc end) {
        auto *node = _ctx.CreateNode<Branch> (cond, thenBB, elseBB, start, end);
        AddToBlock (node);
        return node;
    }

    Branch *
    CreateBr (BasicBlock *branch, llvm::SMLoc start, llvm::SMLoc end) {
        return CreateBr (nullptr, branch, nullptr, start, end);
    }

    StructDef *
    CreateStruct (
        basic::NameObj     name,
        std::vector<Field> fields,
        symbols::Struct   *base,
        llvm::SMLoc        start,
        llvm::SMLoc        end) {
        auto *node = _ctx.CreateNode<StructDef> (
            std::move (name),
            std::move (fields),
            base,
            start,
            end);
        _ctx.AddStruct (node);
        return node;
    }

    FieldExpr *
    CreateFieldExpr (
        Node *base, size_t index, basic::Type *type, llvm::SMLoc start, llvm::SMLoc end) {
        return _ctx.CreateNode<FieldExpr> (base, index, type, start, end);
    }

    StructInstance *
    CreateStructInstance (
        std::vector<std::pair<size_t, Node *>> fields,
        symbols::Struct                       *base,
        basic::Type                           *type,
        llvm::SMLoc                            start,
        llvm::SMLoc                            end) {
        return _ctx
            .CreateNode<StructInstance> (std::move (fields), base, type, start, end);
    }

    Cast *
    CreateCast (
        CastKind     kind,
        basic::Type *type,
        Node        *expr,
        llvm::SMLoc  start,
        llvm::SMLoc  end) {
        return _ctx.CreateNode<Cast> (kind, type, expr, start, end);
    }

    RefExpr *
    CreateReference (Node *expr, basic::Type *type, llvm::SMLoc start, llvm::SMLoc end) {
        return _ctx.CreateNode<RefExpr> (expr, type, start, end);
    }

    DerefExpr *
    CreateDereference (
        Node *expr, basic::Type *type, llvm::SMLoc start, llvm::SMLoc end) {
        return _ctx.CreateNode<DerefExpr> (expr, type, start, end);
    }

    NilExpr *
    CreateNil (llvm::SMLoc start, llvm::SMLoc end) {
        return _ctx.CreateNode<NilExpr> (start, end);
    }

    PtrArith *
    CreatePtrArith (
        ast::BinOp   op,
        Node        *ptr,
        Node        *offset,
        basic::Type *resType,
        llvm::SMLoc  start,
        llvm::SMLoc  end) {
        return _ctx.CreateNode<PtrArith> (op, ptr, offset, resType, start, end);
    }
};

}
