#pragma once
#include "hir/load_glob_var_by_name.h"

#include <hir/bin_expr.h>
#include <hir/branch.h>
#include <hir/expr_stmt.h>
#include <hir/field_expr.h>
#include <hir/func.h>
#include <hir/func_call.h>
#include <hir/lit_expr.h>
#include <hir/load_var.h>
#include <hir/ret.h>
#include <hir/store.h>
#include <hir/struct_def.h>
#include <hir/struct_instance.h>
#include <hir/un_expr.h>
#include <hir/var_def.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <memory>
#include <unordered_map>
#include <vector>

namespace veo {

class CodeGen {
    std::vector<hir::VarDef *>                               &_hirGlobals;
    std::vector<hir::Function *>                             &_hirFuncs;
    std::vector<hir::StructDef *>                            &_hirStructs;
    std::vector<llvm::GlobalVariable *>                       _globals;
    std::unordered_map<symbols::Function *, llvm::Function *> _funcsMap;
    std::unordered_map<symbols::Function *, llvm::Function *> _methodsMap;
    std::vector<llvm::Function *>                             _funcs;
    llvm::LLVMContext                                         _ctx;
    llvm::IRBuilder<>                                         _builder;
    std::unique_ptr<llvm::Module>                             _mod;
    std::unordered_map<hir::BasicBlock *, llvm::BasicBlock *> _basicBlocksMap;

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
        const std::string             &name,
        std::vector<hir::VarDef *>    &globals,
        std::vector<hir::Function *>  &funcs,
        std::vector<hir::StructDef *> &structs)
        : _hirGlobals (globals),
          _hirFuncs (funcs),
          _hirStructs (structs),
          _builder (_ctx),
          _mod (std::make_unique<llvm::Module> (name, _ctx)) {}

    std::unique_ptr<llvm::Module>
    Generate () {
        for (auto &s : _hirStructs) {
            generateStruct (s);
        }
        for (auto &f : _hirFuncs) {
            declareFunc (f);
        }
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

    static std::string
    MangleStaticField (symbols::Struct *sym, const std::string &fieldName);

private:
    void
    generate (hir::Node *node);

    void
    generateBasicBlock (hir::BasicBlock *bb);

    void
    generateVarDef (hir::VarDef *vd);

    void
    declareFunc (hir::Function *fd);

    void
    generateFuncDef (hir::Function *fd);

    void
    generateRet (hir::Return *ret);

    void
    generateExprStmt (hir::ExprStmt *es);

    void
    generateBranch (hir::Branch *br);

    void
    generateStruct (hir::StructDef *sd);

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
    generateFuncCall (hir::FuncCall *fc);

    llvm::Value *
    generateStore (hir::Store *store);

    llvm::Value *
    generateFieldExpr (hir::FieldExpr *fe);

    llvm::Value *
    generateStructInstance (hir::StructInstance *si);

    llvm::Value *
    generateLoadGlobalVarByName (hir::LoadGlobalVarByName *load);

    llvm::Value *
    generateLValue (hir::Node *node);

    llvm::Type *
    getType (basic::Type *type);

    static std::string
    mangleFunction (hir::Function *func);

    static std::string
    mangleMethod (symbols::Struct *sym, hir::Function *func);

    static std::string
    mangleGlobalVar (hir::VarDef *var);

    static std::string
    mangleStruct (hir::StructDef *sd);

    static std::string
    mangleStructSymbol (symbols::Struct *sym);

    static std::string
    mangleModule (symbols::Module *mod);

    static std::string
    mangleType (basic::Type *type);

    void
    generateImplicitMain ();
};

}
