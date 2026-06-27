#pragma once
#include <basic/symbols/module.h>
#include <hir/bin_expr.h>
#include <hir/branch.h>
#include <hir/cast.h>
#include <hir/deref.h>
#include <hir/expr_stmt.h>
#include <hir/field_expr.h>
#include <hir/func.h>
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
#include <hir/var_def.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/SourceMgr.h>
#include <memory>
#include <unordered_map>
#include <vector>

namespace veo {

class CodeGen {
    std::vector<hir::VarDef *>                               &_hirGlobals;
    std::vector<hir::Function *>                             &_hirFuncs;
    std::vector<hir::StructDef *>                            &_hirStructs;
    std::unordered_map<hir::VarDef *, llvm::GlobalVariable *> _globals;
    std::unordered_map<hir::Function *, llvm::Function *>     _funcsMap;
    std::vector<llvm::Function *>                             _funcs;
    llvm::LLVMContext                                         _ctx;
    llvm::IRBuilder<>                                         _builder;
    std::unique_ptr<llvm::Module>                             _mod;
    std::unordered_map<hir::BasicBlock *, llvm::BasicBlock *> _basicBlocksMap;
    symbols::Module                                          *_semaMod;
    llvm::SourceMgr                                          &_srcMgr;

    struct CurrentFunction {
        std::unordered_map<hir::VarDef *, llvm::Value *> Locals;
    };
    std::optional<CurrentFunction> _curFunc = std::nullopt;

    struct UserMain {
        hir::Function  *HIR;
        llvm::Function *LLVM;
    };
    std::optional<UserMain> _userMain = std::nullopt;

public:
    CodeGen (
        llvm::SourceMgr               &srcMgr,
        symbols::Module               *semaMod,
        std::vector<hir::VarDef *>    &globals,
        std::vector<hir::Function *>  &funcs,
        std::vector<hir::StructDef *> &structs,
        const llvm::Triple            &triple,
        const llvm::DataLayout        &dataLayout)
        : _srcMgr (srcMgr),
          _semaMod (semaMod),
          _hirGlobals (globals),
          _hirFuncs (funcs),
          _hirStructs (structs),
          _builder (_ctx),
          _mod (std::make_unique<llvm::Module> (semaMod->Name, _ctx)) {
        _mod->setTargetTriple (triple);
        _mod->setDataLayout (dataLayout);
    }

    std::unique_ptr<llvm::Module>
    Generate () {
        generateRuntime ();

        for (auto &s : _hirStructs) {
            declareStruct (s);
        }
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

        generateInitFunction ();
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
    declareStruct (hir::StructDef *sd);

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
    generateCast (hir::Cast *cast);

    llvm::Value *
    generateRefExpr (hir::RefExpr *re);

    llvm::Value *
    generateDerefExpr (hir::DerefExpr *de);

    llvm::Value *
    generateNilExpr (hir::NilExpr *ne);

    llvm::Value *
    generatePtrArith (hir::PtrArith *pa);

    llvm::Value *
    generateLValue (hir::Node *node);

    llvm::Value *
    findGlobal (hir::VarDef *vd);

    llvm::Value *
    findLocal (hir::VarDef *vd);

    llvm::Type *
    getType (basic::Type *type);

    void
    generateInitFunction ();

    void
    generateImplicitMain ();

    void
    generateRuntime ();

    void
    generateCheckNil (llvm::Value *ptr, llvm::SMLoc start);
};

}
