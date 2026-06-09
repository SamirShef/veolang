#pragma once
#include <ast/context.h>
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
#include <ast/exprs/type_expr.h>
#include <ast/exprs/un_expr.h>
#include <ast/exprs/var_expr.h>
#include <ast/stmts/break_continue.h>
#include <ast/stmts/expr_stmt.h>
#include <ast/stmts/for_loop.h>
#include <ast/stmts/if_else.h>
#include <ast/stmts/impl_stmt.h>
#include <ast/stmts/ret.h>
#include <ast/stmts/trait_stmt.h>
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
#include <ranges>
#include <stack>
#include <vector>

namespace veo {

using namespace diagnostic;
using namespace basic;

class Sema {
    DiagnosticEngine &_diag; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
    TypePool &_typePool;     // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
    size_t    _localsCount = 0;
    symbols::Module *_mod;
    hir::Builder &_builder; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
    ast::Context
        &_astContext; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
    std::stack<symbols::Scope> _vars;
    std::stack<Type *>         _funcRetTypes;

    struct SemanticResult {
        OptValue   Val;
        hir::Node *Node;

        SemanticResult () : Val (Value::Incorrect ()), Node (nullptr) {}
        SemanticResult (OptValue val, hir::Node *node) : Val (val), Node (node) {}
    };

    enum class CastCost : uint16_t {
        Exact        = 0,
        TraitMatch   = 10,
        SafeImplicit = 20,
        Incompatible = 1000
    };

    struct Loop {
        hir::BasicBlock *Break;
        hir::BasicBlock *Continue;
    };
    std::stack<Loop> _loops;

    std::optional<std::pair<symbols::Method *, basic::Type *>>   _insideMethod;
    std::vector<std::pair<ast::MethodCall *, symbols::Method *>> _methodCallOnConstBase;
    std::unordered_map<symbols::Method *, std::vector<symbols::Method *>>
                                                             _methodCallFromAnotherMethod;
    std::unordered_map<symbols::Function *, hir::Function *> _funcs;
    std::unordered_map<symbols::Method *, hir::Function *>   _methods;
    std::unordered_map<symbols::Function *, ast::FuncDef *>  _genericFuncs;
    std::unordered_map<symbols::Method *, ast::FuncDef *>    _genericMethods;
    std::vector<std::unordered_map<std::string, Type *>>     _localTypes;

    unsigned _ptrBitWidth;

    enum class SignatureMatchResult : uint8_t {
        Success,
        StaticMismatch,
        ArgCountMismatch,
        ArgTypeMismatch,
        RetTypeMismatch
    };

    struct MatchResult {
        SignatureMatchResult Status;
        size_t               ErrorArgIndex = 0;

        explicit MatchResult (SignatureMatchResult status, size_t index = 0)
            : Status (status), ErrorArgIndex (index) {}

        MatchResult () : MatchResult (SignatureMatchResult::Success, 0) {}
    };

public:
    Sema (
        DiagnosticEngine &diag,
        hir::Builder     &builder,
        ast::Context     &astContext,
        symbols::Module  *mod,
        TypePool         &typePool,
        unsigned          ptrBitWidth)
        : _diag (diag),
          _builder (builder),
          _astContext (astContext),
          _mod (mod),
          _typePool (typePool),
          _ptrBitWidth (ptrBitWidth) {
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
            } else if (stmt->Kind () == ast::NodeKind::TraitStmt) {
                analyzeTraitStmt (llvm::cast<ast::TraitStmt> (stmt));
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
            if (stmt->Kind () == ast::NodeKind::StructDef
                || stmt->Kind () == ast::NodeKind::TraitStmt) {
                continue;
            }
            analyzeStmt (stmt);
        }

        for (auto &[method, methods] : _methodCallFromAnotherMethod) {
            analyzeMethodCallFromAnotherMethod (method, methods);
        }
        for (auto &[node, method] : _methodCallOnConstBase) {
            analyzeMethodCallOnConstBase (node, method);
        }
    }

private:
    template <typename T, typename... Args>
    T *
    createNode (Args &&...args) {
        return _astContext.CreateNode<T> (std::forward<Args> (args)...);
    }

    template <typename T, typename... Args>
    Type *
    createType (Args &&...args) {
        return _typePool.GetOrCreate<T> (std::forward<Args> (args)...);
    }

    void
    pushTypeScope () {
        _localTypes.emplace_back ();
    }
    void
    popTypeScope () {
        _localTypes.pop_back ();
    }

    void
    registerLocalType (const std::string &name, Type *type) {
        if (!_localTypes.empty ()) {
            _localTypes.back ()[name] = type;
        }
    }

    Type *
    lookupLocalType (const std::string &name) {
        for (auto &types : std::views::reverse (_localTypes)) {
            if (auto f = types.find (name); f != types.end ()) {
                return f->second;
            }
        }
        return nullptr;
    }

    void
    analyzeStmt (ast::Stmt *stmt);

    void
    analyzeVarDef (ast::VarDef *vd);

    void
    declareFunc (ast::FuncDef *fd);

    void
    analyzeFuncDef (ast::FuncDef *fd, bool generatingGeneric = false);

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
    declareImplMethod (ast::Method method, symbols::Struct *sym, basic::Type *targetType);

    MatchResult
    checkMethodSignature (
        symbols::Method *traitMethod, symbols::Method *implMethod, Type *concreteTarget);

    void
    analyzeImplStmt (ast::ImplStmt *is);

    void
    analyzeImplMethodDef (
        ast::Method method, symbols::Struct *sym, basic::Type *targetType);

    void
    analyzeTraitStmt (ast::TraitStmt *ts);

    SemanticResult
    analyzeExpr (ast::Expr *expr, Type *expectedType);

    SemanticResult
    analyzeLiteralExpr (ast::LiteralExpr *le, Type *expectedType);

    SemanticResult
    analyzeBinaryExpr (ast::BinaryExpr *be, Type *expectedType);

    std::pair<SemanticResult, SemanticResult>
    analyzeBinaryExprOperands (ast::BinaryExpr *be);

    SemanticResult
    analyzeUnaryExpr (ast::UnaryExpr *ue, Type *expectedType);

    SemanticResult
    analyzeVarExpr (ast::VarExpr *ve, Type *expectedType);

    SemanticResult
    analyzeFuncCall (ast::FuncCall *fc, Type *expectedType);

    bool
    generateGenericFunc (
        symbols::Function          **func,
        std::vector<Type *>         &argTypes,
        symbols::FunctionCandidates *candidates,
        ast::FuncCall               *fc);

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

    bool
    generateGenericMethod (
        symbols::Method          **method,
        std::vector<Type *>       &argTypes,
        symbols::Struct           *s,
        basic::Type               *targetType,
        symbols::MethodCandidates *candidates,
        ast::MethodCall           *mc);

    SemanticResult
    analyzeTernaryExpr (ast::TernaryExpr *te, Type *expectedType);

    SemanticResult
    analyzeCastExpr (ast::CastExpr *ce, Type *expectedType);

    void
    getIntegralProps (Type *t, bool &isUnsigned, unsigned &width) const;

    hir::CastKind
    castIntegrals (Type *src, Type *dst);

    SemanticResult
    analyzeRefExpr (ast::RefExpr *re, Type *expectedType);

    SemanticResult
    analyzeDerefExpr (ast::DerefExpr *de, Type *expectedType);

    SemanticResult
    analyzeNilExpr (ast::NilExpr *ne, Type *expectedType);

    SemanticResult
    analyzeTypeExpr (ast::TypeExpr *te, Type *expectedType);

    static hir::CastKind
    castInts (const IntegerType *src, const IntegerType *dst);

    static hir::CastKind
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

    symbols::Variable *
    getVariable (const std::string &name);

    symbols::Struct *
    getStruct (const std::string &name, symbols::Module *mod = nullptr);

    std::optional<symbols::Function>
    getFunction (const std::string &name, const std::vector<ast::Argument> &args);

    symbols::Method *
    getMethod (
        basic::Type                      *base,
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

    bool
    compareTypesWithThis (Type *traitTy, Type *implTy, Type *concreteTarget);

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
    canImplicitCast (SemanticResult val, Type **expectedType);

    static bool
    canExplicitCast (Type *src, Type *dst);

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

    static CastCost
    checkCastCostIntegers (Type *src, Type *dst);

    static CastCost
    checkCastCostFloatings (Type *src, Type *dst);

    static CastCost
    checkCastCostTraitMatch (Type *src, Type *dst);

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
        basic::Type           *base,
        bool                   canAccessStatic,
        bool                   canAccessPrivate);

    std::string
    typeToString (const Type *type);

    void
    analyzeMethodCallFromAnotherMethod (
        symbols::Method *method, const std::vector<symbols::Method *> &methods);

    void
    analyzeMethodCallOnConstBase (ast::MethodCall *mc, symbols::Method *method);
};

}
