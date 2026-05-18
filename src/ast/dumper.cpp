#include <ast/access_modifier.h>
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
    size_t i = 0;
    for (const auto &arg : fd->Args ()) {
        if (arg.IsValid ()) {
            _os << arg.Name.Val << ": " << arg.Type->ToString ();
        }
        if (i < fd->Args ().size () - 1) {
            _os << ", ";
        }
        ++i;
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
Dumper::dumpRet (Return *ret) {
    indent ();
    _os << "Return:\n";
    if (ret->RetExpr () != nullptr) {
        ++_indentLvl;
        dumpExpr (ret->RetExpr ());
        --_indentLvl;
    }
}

void
Dumper::dumpExprStmt (ExprStmt *es) {
    indent ();
    _os << "ExprStmt:\n";
    ++_indentLvl;
    dumpExpr (es->GetExpr ());
    --_indentLvl;
}

void
Dumper::dumpIfElse (IfElseStmt *ies) {
    indent ();
    _os << "IfElseStmt:\n";

    ++_indentLvl; // common
    dumpExpr (ies->Cond ());

    indent ();
    _os << "Then:\n";
    ++_indentLvl; // then
    for (const auto &stmt : ies->Then ()) {
        dumpStmt (stmt);
    }
    --_indentLvl; // then

    if (!ies->Else ().empty ()) {
        indent ();
        _os << "Else:\n";
        ++_indentLvl; // else
        for (const auto &stmt : ies->Else ()) {
            dumpStmt (stmt);
        }
        --_indentLvl; // else
    }

    --_indentLvl; // common
}

void
Dumper::dumpForLoop (ForLoopStmt *fls) {
    indent ();
    _os << "ForLoopStmt:\n";
    ++_indentLvl;

    if (fls->Cond () != nullptr) {
        indent ();
        _os << "Cond:\n";
        ++_indentLvl;
        dumpExpr (fls->Cond ());
        --_indentLvl;
    }

    if (fls->Indexator () != nullptr) {
        indent ();
        _os << "Indexator:\n";
        ++_indentLvl;
        dumpStmt (fls->Indexator ());
        --_indentLvl;
    }

    if (fls->Iteration () != nullptr) {
        indent ();
        _os << "Iteration:\n";
        ++_indentLvl;
        dumpStmt (fls->Iteration ());
        --_indentLvl;
    }

    if (!fls->Body ().empty ()) {
        indent ();
        _os << "Body:\n";
        ++_indentLvl;
        for (const auto &stmt : fls->Body ()) {
            dumpStmt (stmt);
        }
        --_indentLvl;
    }

    --_indentLvl;
}

void
Dumper::dumpStructDef (StructDef *sd) {
    indent ();
    _os << "StructDef: " << AccessToString (sd->Access ()) << ' ' << sd->Name ().Val
        << '\n';

    ++_indentLvl;
    for (const auto &field : sd->Fields ()) {
        indent ();
        _os << "Field: " << AccessToString (field.Access) << ' '
            << (field.IsStatic ? "static " : "") << (field.IsConst ? "const " : "")
            << field.Name.Val << ": " << field.Type->ToString () << "\n";
    }
    --_indentLvl;
}

void
Dumper::dumpBreakContinue (BreakContinue *bc) {
    indent ();
    _os << "BreakContinue: "
        << (bc->GetKind () == BreakContinue::Kind::Break ? "Break\n" : "Continue\n");
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

void
Dumper::dumpFuncCall (FuncCall *fc) {
    indent ();
    _os << "FuncCall: " << fc->Name ().Val << '\n';
    ++_indentLvl;
    for (auto &a : fc->Args ()) {
        dumpExpr (a);
    }
    --_indentLvl;
}

void
Dumper::dumpAsgnExpr (AsgnExpr *ae) {
    indent ();
    _os << "AsgnExpr: " << AsgnOpToString (ae->Op ()) << '\n';
    ++_indentLvl;
    dumpExpr (ae->Ptr ());
    dumpExpr (ae->Init ());
    --_indentLvl;
}

void
Dumper::dumpFieldExpr (FieldExpr *fe) {
    indent ();
    _os << "FieldExpr: " << fe->Name ().Val << '\n';
    ++_indentLvl;

    indent ();
    _os << "From:\n";
    ++_indentLvl;
    dumpExpr (fe->Base ());
    --_indentLvl;

    --_indentLvl;
}

}
