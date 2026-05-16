#pragma once
#include <basic/name.h>
#include <basic/types/type.h>
#include <hir/basic_block.h>
#include <hir/bin_expr.h>
#include <hir/context.h>
#include <hir/expr_stmt.h>
#include <hir/func_call.h>
#include <hir/lit_expr.h>
#include <hir/load_var.h>
#include <hir/ret.h>
#include <hir/store_var.h>
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

    void
    SetInsertionPoint (BasicBlock *bb) {
        _insertBlock = bb;
    }

    BasicBlock *
    CreateBasicBlock (Function *parent, std::string name = "") {
        auto *node = _ctx.CreateNode<BasicBlock> (parent, std::move (name));
        parent->Body ().push_back (node);
        return node;
    }

    void
    AddToBlock (Node *node) {
        _insertBlock->AddInst (node);
    }

    Function *
    CreateFunction (
        basic::NameObj             name,
        basic::Type               *retType,
        std::vector<ast::Argument> args,
        llvm::SMLoc                start,
        llvm::SMLoc                end,
        symbols::Function         *base) {
        auto *node = _ctx.CreateNode<Function> (
            std::move (name),
            retType,
            std::move (args),
            start,
            end,
            base);
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
        symbols::Variable *base) {
        auto *node = _ctx.CreateNode<VarDef> (
            std::move (name),
            type,
            init,
            isConst,
            start,
            end,
            base);
        if (isGlobal) {
            _ctx.AddGlobal (node);
        } else {
            _insertBlock->AddInst (node);
        }
        return node;
    }

    LiteralExpr *
    CreateLiteral (basic::Value val, llvm::SMLoc start, llvm::SMLoc end) {
        return _ctx.CreateNode<LiteralExpr> (val, start, end);
    }

    BinaryExpr *
    CreateBinary (
        ast::BinOp   op,
        basic::Type *commonType,
        Node        *lhs,
        Node        *rhs,
        llvm::SMLoc  start,
        llvm::SMLoc  end) {
        return _ctx.CreateNode<BinaryExpr> (op, commonType, lhs, rhs, start, end);
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
        size_t id, basic::Type *type, bool isGlobal, llvm::SMLoc start, llvm::SMLoc end) {
        return _ctx.CreateNode<LoadVar> (id, type, isGlobal, start, end);
    }

    StoreVar *
    CreateStoreVar (Node *ptr, Node *expr, llvm::SMLoc start, llvm::SMLoc end) {
        return _ctx.CreateNode<StoreVar> (ptr, expr, start, end);
    }

    ExprStmt *
    CreateExprStmt (Node *expr, llvm::SMLoc start, llvm::SMLoc end) {
        auto *node = _ctx.CreateNode<ExprStmt> (expr, start, end);
        AddToBlock (node);
        return node;
    }

    FuncCall *
    CreateCall (
        symbols::Function  *func,
        std::vector<Node *> args,
        llvm::SMLoc         start,
        llvm::SMLoc         end) {
        return _ctx.CreateNode<FuncCall> (func, std::move (args), start, end);
    }
};

}
