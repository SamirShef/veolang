#pragma once
#include <ast/exprs/bin_expr.h>
#include <ast/exprs/lit_expr.h>
#include <ast/exprs/un_expr.h>
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
        switch (stmt->Kind ()) {
        case NodeKind::VarDef: dumpVarDef (llvm::cast<VarDef> (stmt)); break;
        case NodeKind::FuncDef: dumpFuncDef (llvm::cast<FuncDef> (stmt)); break;
        default: {
        }
        }
    }

    void
    dumpVarDef (VarDef *vd);

    void
    dumpFuncDef (FuncDef *fd);

    void
    dumpExpr (Expr *expr) {
        if (!checkNull (expr)) {
            return;
        }
        switch (expr->Kind ()) {
        case NodeKind::LitExpr: dumpLiteralExpr (llvm::cast<LiteralExpr> (expr)); break;
        case NodeKind::BinExpr: dumpBinaryExpr (llvm::cast<BinaryExpr> (expr)); break;
        case NodeKind::UnExpr: dumpUnaryExpr (llvm::cast<UnaryExpr> (expr)); break;
        default: {
        }
        }
    }

    void
    dumpBinaryExpr (BinaryExpr *be);

    void
    dumpUnaryExpr (UnaryExpr *ue);

    void
    dumpLiteralExpr (LiteralExpr *le);

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
