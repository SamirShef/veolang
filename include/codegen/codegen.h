#pragma once
#include <hir/bin_expr.h>
#include <hir/func.h>
#include <hir/lit_expr.h>
#include <hir/load_var.h>
#include <hir/ret.h>
#include <hir/un_expr.h>
#include <hir/var_def.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <memory>
#include <vector>

namespace veo {

class CodeGen {
    std::vector<hir::VarDef *>         &_hirGlobals;
    std::vector<hir::Function *>       &_hirFuncs;
    std::vector<llvm::GlobalVariable *> _globals;
    std::vector<llvm::Function *>       _funcs;
    llvm::LLVMContext                   _ctx;
    llvm::IRBuilder<>                   _builder;
    std::unique_ptr<llvm::Module>       _mod;

    struct CurrentFunction {
        std::vector<llvm::Value *> Locals;
    };
    std::optional<CurrentFunction> _curFunc = std::nullopt;

    struct UserMain {
        hir::Function  *HIR;
        llvm::Function *LLVM;
    };
    std::optional<UserMain> _userMain = std::nullopt;

public:
    CodeGen (
        const std::string            &name,
        std::vector<hir::VarDef *>   &globals,
        std::vector<hir::Function *> &funcs)
        : _hirGlobals (globals),
          _hirFuncs (funcs),
          _builder (_ctx),
          _mod (std::make_unique<llvm::Module> (name, _ctx)) {}

    std::unique_ptr<llvm::Module>
    Generate () {
        for (auto &g : _hirGlobals) {
            generateVarDef (g);
        }
        for (auto &f : _hirFuncs) {
            generateFuncDef (f);
        }
        if (_userMain.has_value ()) {
            generateImplicitMain ();
        }
        return std::move (_mod);
    }

private:
    void
    generate (hir::Node *node);

    void
    generateBasicBlock (hir::BasicBlock *bb);

    void
    generateVarDef (hir::VarDef *vd);

    void
    generateFuncDef (hir::Function *fd);

    void
    generateRet (hir::Return *ret);

    llvm::Value *
    generateExpr (hir::Node *node);

    llvm::Value *
    generateLiteralExpr (hir::LiteralExpr *le);

    llvm::Value *
    generateBinaryExpr (hir::BinaryExpr *be);

    llvm::Value *
    generateUnaryExpr (hir::UnaryExpr *ue);

    llvm::Value *
    generateLoadVar (hir::LoadVar *lv);

    llvm::Value *
    generateLValue (hir::Node *node);

    llvm::Type *
    getType (basic::Type *type);

    std::string
    mangleFunction (hir::Function *func) const;

    std::string
    mangleGlobalVar (hir::VarDef *var) const;

    std::string
    mangleModule (symbols::Module *mod) const;

    std::string
    mangleType (basic::Type *type) const;

    void
    generateImplicitMain ();
};

}
