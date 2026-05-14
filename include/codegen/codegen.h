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

using namespace hir;

class CodeGen {
    std::vector<VarDef *>              &_hirGlobals;
    std::vector<Function *>            &_hirFuncs;
    std::vector<llvm::GlobalVariable *> _globals;
    std::vector<llvm::Function *>       _funcs;
    std::unique_ptr<llvm::Module>       _mod;
    llvm::LLVMContext                   _ctx;
    llvm::IRBuilder<>                   _builder;

public:
    CodeGen (
        const std::string       &name,
        std::vector<VarDef *>   &globals,
        std::vector<Function *> &funcs)
        : _hirGlobals (globals),
          _hirFuncs (funcs),
          _mod (std::make_unique<llvm::Module> (name, _ctx)),
          _builder (_ctx) {}

    void
    Generate (const std::vector<Node *> &nodes) {
        for (const auto &node : nodes) {
            if (node == nullptr) {
                continue;
            }
            generate (node);
        }
    }

private:
    void
    generate (Node *node);

    void
    generateVarDef (VarDef *vd);

    void
    generateFuncDef (Function *fd);

    void
    generateRet (Return *ret);

    llvm::Value *
    generateExpr (Node *node);

    llvm::Value *
    generateLiteralExpr (LiteralExpr *le);

    llvm::Value *
    generateBinaryExpr (BinaryExpr *be);

    llvm::Value *
    generateUnaryExpr (UnaryExpr *ue);

    llvm::Value *
    generateLoadVar (LoadVar *lv);

    llvm::Value *
    generateLValue (Node *node);

    llvm::Type *
    getType (basic::Type *type);

    std::string
    mangleFunction (Function *func) const;

    std::string
    mangleGlobalVar (VarDef *var) const;
};

}
