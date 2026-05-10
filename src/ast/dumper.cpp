#include "ast/access_modifier.h"

#include <ast/dumper.h>

namespace veo::ast {

inline const char *
AccessToString (AccessModifier access) {
#define variant(kind, res)                                                               \
    case AccessModifier::kind: return res;
    switch (access) {
        variant (Pub, "pub");
        variant (Priv, "priv");
    }
#undef variant
    return ""; // to shut up the fucking warning
}

inline const char *
BinOpToString (BinOp op) {
#define variant(kind, res)                                                               \
    case BinOp::kind: return res;
    switch (op) {
        variant (Plus, "+");
        variant (Minus, "-");
        variant (Mul, "*");
        variant (Div, "/");
        variant (Rem, "%");
        variant (Eq, "==");
        variant (NEq, "!=");
        variant (Lt, "<");
        variant (LtEq, "<=");
        variant (Gt, ">");
        variant (GtEq, ">=");
        variant (LogAnd, "&&");
        variant (LogOr, "||");
        variant (Invalid, "<invalid>");
    }
#undef variant
    return ""; // to shut up the fucking warning
}

inline const char *
UnOpToString (UnOp op) {
#define variant(kind, res)                                                               \
    case UnOp::kind: return res;
    switch (op) {
        variant (Minus, "-");
        variant (Not, "!");
        variant (Invalid, "<invalid>");
    }
#undef variant
    return ""; // to shut up the fucking warning
}

void
Dumper::Dump (ParseResult res, unsigned startIndentLevel) {
    _indentLvl = startIndentLevel;
    for (size_t i = 0; i < res.Count; ++i) {
        Node *node = res.Nodes[i];
        if (node->Kind () > NodeKind::StmtStart && node->Kind () < NodeKind::StmtEnd) {
            if (i != 0) {
                _os << '\n';
            }
            dumpStmt (llvm::cast<Stmt> (node));
        }
    }
}

void
Dumper::dumpVarDef (VarDef *vd) {
    indent ();
    _os << "VarDef: " << AccessToString (vd->Access ()) << ' '
        << (vd->IsConst () ? "const" : "let") << ' ' << vd->Name ().Val;
    if (vd->Type () != nullptr) {
        _os << ": " << vd->Type ()->ToString ();
    }
    _os << '\n';
    if (vd->Init () != nullptr) {
        ++_indentLvl;
        dumpExpr (vd->Init ());
        --_indentLvl;
    }
}

void
Dumper::dumpFuncDef (FuncDef *fd) {
    indent ();
    _os << "FuncDef: " << AccessToString (fd->Access ()) << ' ' << fd->Name ().Val
        << " (";
    for (const auto &arg : fd->Args ()) {
        if (arg.IsValid ()) {
            _os << arg.Name.Val << ": " << arg.Type->ToString ();
        }
    }
    _os << ')';
    if (fd->RetType () != nullptr) {
        _os << ": " << fd->RetType ()->ToString ();
    }
    _os << '\n';
    ++_indentLvl;
    for (const auto &stmt : fd->Body ()) {
        dumpStmt (stmt);
    }
    --_indentLvl;
}

void
Dumper::dumpLiteralExpr (LiteralExpr *le) {
    indent ();
    _os << "LiteralExpr: " << le->Value () << '\n';
}

void
Dumper::dumpBinaryExpr (BinaryExpr *be) {
    indent ();
    _os << "BinaryExpr: " << BinOpToString (be->Op ()) << '\n';
    ++_indentLvl;
    dumpExpr (be->Lhs ());
    dumpExpr (be->Rhs ());
    --_indentLvl;
}

void
Dumper::dumpUnaryExpr (UnaryExpr *ue) {
    indent ();
    _os << "UnaryExpr: " << UnOpToString (ue->Op ()) << '\n';
    ++_indentLvl;
    dumpExpr (ue->Rhs ());
    --_indentLvl;
}

void
Dumper::dumpVarExpr (VarExpr *ve) {
    indent ();
    _os << "VarExpr: " << ve->Name ().Val << '\n';
}

}
