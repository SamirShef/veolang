#pragma once
#include "ast/exprs/asgn_expr.h"

#include <ast/exprs/bin_expr.h>
#include <ast/exprs/func_call.h>
#include <ast/exprs/lit_expr.h>
#include <ast/exprs/un_expr.h>
#include <ast/exprs/var_expr.h>
#include <ast/stmts/expr_stmt.h>
#include <ast/stmts/ret.h>
#include <ast/stmts/var_def.h>
#include <llvm/Support/raw_ostream.h>
#include <parser/parser.h>

namespace veo::ast {

class Dumper {
    unsigned           _indentLvl = 0;
    llvm::raw_ostream &_os; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)

public:
    explicit Dumper (llvm::raw_ostream &os) : _os (os) {}

    void
    Dump (ParseResult res, unsigned startIndentLevel = 0);

private:
    void
    dumpStmt (Stmt *stmt) {
        if (!checkNull (stmt)) {
            return;
        }
#define variant(kind, func, type)                                                        \
    case NodeKind::kind: func (llvm::cast<type> (stmt)); break;
        switch (stmt->Kind ()) {
            variant (VarDef, dumpVarDef, VarDef);
            variant (FuncDef, dumpFuncDef, FuncDef);
            variant (Ret, dumpRet, Return);
            variant (ExprStmt, dumpExprStmt, ExprStmt);
        default: {
        }
        }
#undef variant
    }

    void
    dumpVarDef (VarDef *vd);

    void
    dumpFuncDef (FuncDef *fd);

    void
    dumpRet (Return *ret);

    void
    dumpExprStmt (ExprStmt *es);

    void
    dumpExpr (Expr *expr) {
        if (!checkNull (expr)) {
            return;
        }
#define variant(kind, func, type)                                                        \
    case NodeKind::kind: func (llvm::cast<type> (expr)); break;
        switch (expr->Kind ()) {
            variant (LitExpr, dumpLiteralExpr, LiteralExpr);
            variant (BinExpr, dumpBinaryExpr, BinaryExpr);
            variant (UnExpr, dumpUnaryExpr, UnaryExpr);
            variant (VarExpr, dumpVarExpr, VarExpr);
            variant (FuncCall, dumpFuncCall, FuncCall);
            variant (AsgnExpr, dumpAsgnExpr, AsgnExpr);
        default: {
        }
        }
#undef variant
    }

    void
    dumpLiteralExpr (LiteralExpr *le);

    void
    dumpBinaryExpr (BinaryExpr *be);

    void
    dumpUnaryExpr (UnaryExpr *ue);

    void
    dumpVarExpr (VarExpr *ve);

    void
    dumpFuncCall (FuncCall *fc);

    void
    dumpAsgnExpr (AsgnExpr *ae);

    void
    indent () {
        _os.indent (_indentLvl * 2);
    }

    bool
    checkNull (const Node *node) {
        if (node == nullptr) {
            _os << "null\n";
            return false;
        }
        return true;
    }
};

}
