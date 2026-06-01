#pragma once
#include <ast/exprs/asgn_expr.h>
#include <ast/exprs/bin_expr.h>
#include <ast/exprs/cast_expr.h>
#include <ast/exprs/deref.h>
#include <ast/exprs/field_expr.h>
#include <ast/exprs/func_call.h>
#include <ast/exprs/lit_expr.h>
#include <ast/exprs/method_call.h>
#include <ast/exprs/nil.h>
#include <ast/exprs/ref.h>
#include <ast/exprs/struct_instance.h>
#include <ast/exprs/ternary_expr.h>
#include <ast/exprs/un_expr.h>
#include <ast/exprs/var_expr.h>
#include <ast/stmts/break_continue.h>
#include <ast/stmts/expr_stmt.h>
#include <ast/stmts/for_loop.h>
#include <ast/stmts/if_else.h>
#include <ast/stmts/impl_stmt.h>
#include <ast/stmts/ret.h>
#include <ast/stmts/var_def.h>
#include <basic/symbols/module.h>
#include <basic/symbols/scope.h>
#include <basic/symbols/struct.h>
#include <basic/types/pool.h>
#include <basic/value.h>
#include <cstddef>
#include <diagnostic/engine.h>
#include <hir/builder.h>
#include <parser/parser.h>
#include <stack>

namespace veo {

using namespace diagnostic;
using namespace basic;

class Sema {
    DiagnosticEngine &_diag; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
    TypePool &_typePool;     // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
    size_t    _localsCount = 0;
    symbols::Module           *_mod;
    hir::Builder               _builder;
    std::stack<symbols::Scope> _vars;
    std::stack<Type *>         _funcRetTypes;

    struct SemanticResult {
        OptValue   Val;
        hir::Node *Node;

        SemanticResult () : Val (Value::Incorrect ()), Node (nullptr) {}
        SemanticResult (OptValue val, hir::Node *node) : Val (val), Node (node) {}
    };

    enum class CastCost : uint16_t { Exact = 0, SafeImplicit = 1, Incompatible = 1000 };

    struct Loop {
        hir::BasicBlock *Break;
        hir::BasicBlock *Continue;
    };
    std::stack<Loop> _loops;

    std::optional<std::pair<symbols::Method *, symbols::Struct *>> _insideMethod;
    std::vector<std::pair<ast::MethodCall *, symbols::Method *>>   _methodCallOnConstBase;

public:
    Sema (
        DiagnosticEngine &diag,
        hir::Context     &ctx,
        symbols::Module  *mod,
        TypePool         &typePool)
        : _diag (diag), _builder (ctx), _mod (mod), _typePool (typePool) {
        _vars.emplace ();
    }

    void
    Analyze (ParseResult &res) {
        for (size_t i = 0; i < res.Count; ++i) {
            if (res.Nodes[i] == nullptr) {
                continue;
            }
            auto *stmt = llvm::cast<ast::Stmt> (res.Nodes[i]);
            if (stmt->Kind () == ast::NodeKind::StructDef) {
                analyzeStructDef (llvm::cast<ast::StructDef> (stmt));
            }
        }
        for (size_t i = 0; i < res.Count; ++i) {
            if (res.Nodes[i] == nullptr) {
                continue;
            }
            auto *stmt = llvm::cast<ast::Stmt> (res.Nodes[i]);
            if (stmt->Kind () == ast::NodeKind::FuncDef) {
                declareFunc (llvm::cast<ast::FuncDef> (stmt));
            } else if (stmt->Kind () == ast::NodeKind::ImplStmt) {
                declareImplMethods (llvm::cast<ast::ImplStmt> (stmt));
            }
        }

        for (size_t i = 0; i < res.Count; ++i) {
            auto *stmt = llvm::cast<ast::Stmt> (res.Nodes[i]);
            if (stmt->Kind () == ast::NodeKind::StructDef) {
                continue;
            }
            analyzeStmt (stmt);
        }

        for (auto &[node, method] : _methodCallOnConstBase) {
            analyzeMethodCallOnConstBase (node, method);
        }
    }

private:
    template <typename T, typename... Args>
    Type *
    createType (Args &&...args) {
        return _typePool.GetOrCreate<T> (std::forward<Args> (args)...);
    }

    void
    analyzeStmt (ast::Stmt *stmt);

    void
    analyzeVarDef (ast::VarDef *vd);

    void
    declareFunc (ast::FuncDef *fd);

    void
    analyzeFuncDef (ast::FuncDef *fd);

    void
    analyzeRet (ast::Return *ret);

    void
    analyzeExprStmt (ast::ExprStmt *es);

    void
    analyzeIfElseStmt (ast::IfElseStmt *ies);

    void
    analyzeForLoop (ast::ForLoopStmt *fls);

    void
    analyzeBreakContinue (ast::BreakContinue *bc);

    void
    analyzeStructDef (ast::StructDef *sd);

    void
    declareImplMethods (ast::ImplStmt *is);

    void
    analyzeImplStmt (ast::ImplStmt *is);

    SemanticResult
    analyzeExpr (ast::Expr *expr, Type *expectedType);

    SemanticResult
    analyzeLiteralExpr (ast::LiteralExpr *le, Type *expectedType);

    SemanticResult
    analyzeBinaryExpr (ast::BinaryExpr *be, Type *expectedType);

    SemanticResult
    analyzeUnaryExpr (ast::UnaryExpr *ue, Type *expectedType);

    SemanticResult
    analyzeVarExpr (ast::VarExpr *ve, Type *expectedType);

    SemanticResult
    analyzeFuncCall (ast::FuncCall *fc, Type *expectedType);

    SemanticResult
    analyzeAsgnExpr (ast::AsgnExpr *ae, Type *expectedType);

    SemanticResult
    analyzeAsgnVar (ast::AsgnExpr *ae, SemanticResult &expr, const SemanticResult &ptr);

    SemanticResult
    analyzeAsgnField (ast::AsgnExpr *ae, SemanticResult &expr, const SemanticResult &ptr);

    SemanticResult
    analyzeAsgnPtr (ast::AsgnExpr *ae, SemanticResult &expr, const SemanticResult &ptr);

    SemanticResult
    analyzeFieldExpr (ast::FieldExpr *fe, Type *expectedType);

    SemanticResult
    analyzeStructInstance (ast::StructInstance *si, Type *expectedType);

    SemanticResult
    analyzeMethodCall (ast::MethodCall *mc, Type *expectedType);

    SemanticResult
    analyzeTernaryExpr (ast::TernaryExpr *te, Type *expectedType);

    SemanticResult
    analyzeCastExpr (ast::CastExpr *ce, Type *expectedType);

    SemanticResult
    analyzeRefExpr (ast::RefExpr *re, Type *expectedType);

    SemanticResult
    analyzeDerefExpr (ast::DerefExpr *de, Type *expectedType);

    SemanticResult
    analyzeNilExpr (ast::NilExpr *ne, Type *expectedType);

    hir::CastKind
    castInts (const IntegerType *src, const IntegerType *dst);

    hir::CastKind
    castFloats (const FloatingType *src, const FloatingType *dst);

    SemanticResult
    cast (
        hir::CastKind  kind,
        Type          *dst,
        SemanticResult val,
        llvm::SMLoc    start,
        llvm::SMLoc    end);

    Type *
    resolveType (Type **type);

    std::optional<symbols::Variable>
    getVariable (const std::string &name);

    symbols::Struct *
    getStruct (const std::string &name, symbols::Module *mod = nullptr);

    std::optional<symbols::Function>
    getFunction (const std::string &name, const std::vector<ast::Argument> &args);

    static symbols::Method *
    getMethod (
        symbols::Struct                  *sym,
        const std::string                &name,
        const std::vector<ast::Argument> &args);

    Type *
    getCommonType (Type *lhs, Type *rhs, llvm::SMLoc start, llvm::SMLoc end);

    static Type *
    getCommonIngeter (Type *lhs, Type *rhs, llvm::SMLoc start, llvm::SMLoc end);

    static Type *
    getCommonFloating (Type *lhs, Type *rhs, llvm::SMLoc start, llvm::SMLoc end);

    Type *
    getCommonFloatingAndInteger (
        Type *lhs, Type *rhs, llvm::SMLoc start, llvm::SMLoc end);

    OptValue
    foldBinary (
        ast::BinOp   op,
        const Value &lhs,
        const Value &rhs,
        Type        *resType,
        llvm::SMLoc  start,
        llvm::SMLoc  end);

    static int64_t
    foldBinaryBitwise (ValueData lhs, ValueData rhs, Type *commonType, ast::BinOp op);

    OptValue
    foldUnary (
        ast::UnOp    op,
        const Value &rhs,
        Type        *resType,
        llvm::SMLoc  start,
        llvm::SMLoc  end);

    bool
    canImplicitlyCast (SemanticResult val, Type **expectedType);

    SemanticResult
    implicitlyCast (
        SemanticResult val, Type **expectedType, llvm::SMLoc start, llvm::SMLoc end);

    static bool
    canFit (Value &val, const Type *targetType);

    symbols::Function *
    resolveBestOverload (
        symbols::FunctionCandidates *candidates,
        const std::vector<Type *>   &argTypes,
        llvm::SMLoc                  start,
        llvm::SMLoc                  end);

    symbols::Method *
    resolveBestOverload (
        symbols::MethodCandidates *candidates,
        const std::vector<Type *> &argTypes,
        llvm::SMLoc                start,
        llvm::SMLoc                end);

    void
    candidatesToStringVector (
        symbols::FunctionCandidates *candidates, std::vector<std::string> &res);

    void
    candidatesToStringVector (
        symbols::MethodCandidates *candidates, std::vector<std::string> &res);

    std::string
    funcCandidateToString (symbols::Function *func);

    static CastCost
    checkCastCost (Type *src, Type *dst);

    static bool
    canApplyAsgnOp (ast::AsgnOp op, Type *type);

    static bool
    viableFuncCandidate (
        symbols::Function *func, const std::vector<Type *> &args, size_t &costSum);

    bool
    inGlobalScope () const;

    bool
    allowInScope (ast::Stmt *stmt, bool allowInGlobal = true);

    bool
    canAccessField (
        const basic::NameObj  &feName,
        const symbols::Field  &field,
        const symbols::Struct *s,
        bool                   canAccessStatic,
        bool                   canAccessPrivate);

    bool
    canAccessMethod (
        const basic::NameObj  &mcName,
        const symbols::Method &method,
        const symbols::Struct *s,
        bool                   canAccessStatic,
        bool                   canAccessPrivate);

    std::string
    typeToString (Type *type);

    void
    analyzeMethodCallOnConstBase (ast::MethodCall *mc, symbols::Method *method);
};

}
