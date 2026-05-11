#pragma once
#include <ast/exprs/bin_expr.h>
#include <ast/exprs/lit_expr.h>
#include <ast/exprs/un_expr.h>
#include <ast/exprs/var_expr.h>
#include <ast/stmts/var_def.h>
#include <basic/symbols/module.h>
#include <basic/symbols/scope.h>
#include <basic/value.h>
#include <cstddef>
#include <diagnostic/engine.h>
#include <hir/builder.h>
#include <parser/parser.h>
#include <stack>

namespace veo {

using namespace symbols;
using namespace diagnostic;
using namespace basic;

class Sema {
    DiagnosticEngine &_diag; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
    size_t            _localsCount = 0;
    Module           *_mod;
    hir::Builder      _builder;
    std::stack<Scope> _vars;

    struct SemanticResult {
        OptValue   Val;
        hir::Node *Node;

        SemanticResult () : Val (Value::Incorrect ()), Node (nullptr) {}
        SemanticResult (OptValue val, hir::Node *node) : Val (val), Node (node) {}
    };

public:
    Sema (DiagnosticEngine &diag, hir::Context &ctx, Module *mod)
        : _diag (diag), _builder (ctx), _mod (mod) {
        _vars.emplace ();
    }

    void
    Analyze (ParseResult &res) {
        for (size_t i = 0; i < res.Count; ++i) {
            analyzeStmt (llvm::cast<Stmt> (res.Nodes[i]));
        }
    }

private:
    void
    analyzeStmt (Stmt *stmt);

    void
    analyzeVarDef (VarDef *vd);

    void
    analyzeFuncDef (FuncDef *fd);

    SemanticResult
    analyzeExpr (Expr *expr, Type *expectedType);

    SemanticResult
    analyzeLiteralExpr (LiteralExpr *le, Type *expectedType);

    SemanticResult
    analyzeBinaryExpr (BinaryExpr *be, Type *expectedType);

    SemanticResult
    analyzeUnaryExpr (UnaryExpr *ue, Type *expectedType);

    SemanticResult
    analyzeVarExpr (VarExpr *ve, Type *expectedType);

    Type *
    resolveType (Type **type);

    std::optional<Variable>
    getVariable (const std::string &name);

    Type *
    getCommonType (Type *lhs, Type *rhs, llvm::SMLoc start, llvm::SMLoc end);

    OptValue
    foldBinary (
        BinOp        op,
        const Value &lhs,
        const Value &rhs,
        Type        *resType,
        llvm::SMLoc  start,
        llvm::SMLoc  end);

    OptValue
    foldUnary (
        UnOp op, const Value &rhs, Type *resType, llvm::SMLoc start, llvm::SMLoc end);

    SemanticResult
    implicitlyCast (
        SemanticResult val, Type **expectedType, llvm::SMLoc start, llvm::SMLoc end);

    static bool
    canFit (Value &val, const Type *targetType);
};

}
