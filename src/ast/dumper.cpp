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
    print (
        "VarDef: " + std::string (AccessToString (vd->Access ())) + ' '
        + (vd->IsConst () ? "const" : "let") + ' ' + vd->Name ().Val);
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
    print (
        "FuncDef: " + std::string (AccessToString (fd->Access ()))
        + (fd->IsDeclaration () ? " decl" : "") + ' ' + fd->Name ().Val + " (");
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
    print ("Return:\n");
    if (ret->RetExpr () != nullptr) {
        ++_indentLvl;
        dumpExpr (ret->RetExpr ());
        --_indentLvl;
    }
}

void
Dumper::dumpExprStmt (ExprStmt *es) {
    print ("ExprStmt:\n");
    ++_indentLvl;
    dumpExpr (es->GetExpr ());
    --_indentLvl;
}

void
Dumper::dumpIfElse (IfElseStmt *ies) {
    print ("IfElseStmt:\n");

    ++_indentLvl; // common
    dumpExpr (ies->Cond ());

    print ("Then:\n");
    ++_indentLvl; // then
    for (const auto &stmt : ies->Then ()) {
        dumpStmt (stmt);
    }
    --_indentLvl; // then

    if (!ies->Else ().empty ()) {
        print ("Else:\n");
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
    print ("ForLoopStmt:\n");
    ++_indentLvl;

    if (fls->Cond () != nullptr) {
        print ("Cond:\n");
        ++_indentLvl;
        dumpExpr (fls->Cond ());
        --_indentLvl;
    }

    if (fls->Indexator () != nullptr) {
        print ("Indexator:\n");
        ++_indentLvl;
        dumpStmt (fls->Indexator ());
        --_indentLvl;
    }

    if (fls->Iteration () != nullptr) {
        print ("Iteration:\n");
        ++_indentLvl;
        dumpStmt (fls->Iteration ());
        --_indentLvl;
    }

    if (!fls->Body ().empty ()) {
        print ("Body:\n");
        ++_indentLvl;
        for (const auto &stmt : fls->Body ()) {
            dumpStmt (stmt);
        }
        --_indentLvl;
    }

    --_indentLvl;
}

void
Dumper::dumpBreakContinue (BreakContinue *bc) {
    print (
        std::string ("BreakContinue: ")
        + (bc->GetKind () == BreakContinue::Kind::Break ? "Break\n" : "Continue\n"));
}

void
Dumper::dumpStructDef (StructDef *sd) {
    print (
        "StructDef: " + std::string (AccessToString (sd->Access ())) + ' '
        + sd->Name ().Val + '\n');

    ++_indentLvl;
    for (const auto &field : sd->Fields ()) {
        print (
            "Field: " + std::string (AccessToString (field.Access)) + ' '
            + (field.IsStatic ? "static " : "") + (field.IsConst ? "const " : "")
            + field.Name.Val + ": " + field.Type->ToString () + "\n");
    }
    --_indentLvl;
}

void
Dumper::dumpImplStmt (ImplStmt *is) {
    print (
        std::string ("ImplStmt: ")
        + (is->TraitType () != nullptr ? is->TraitType ()->ToString () + " for " : "")
        + is->StructType ()->ToString () + '\n');
    ++_indentLvl;

    print ("Methods:\n");
    ++_indentLvl;
    for (const auto &method : is->Methods ()) {
        auto *func = method.Func;
        print (
            std::string (AccessToString (func->Access ())) + ' '
            + (method.IsStatic ? "static " : "") + func->Name ().Val + " (");

        size_t i = 0;
        for (const auto &arg : func->Args ()) {
            if (arg.IsValid ()) {
                _os << arg.Name.Val << ": " << arg.Type->ToString ();
            }
            if (i < func->Args ().size () - 1) {
                _os << ", ";
            }
            ++i;
        }
        _os << ')';
        if (func->RetType () != nullptr) {
            _os << ": " << func->RetType ()->ToString ();
        }
        _os << '\n';
        ++_indentLvl;
        for (const auto &stmt : func->Body ()) {
            dumpStmt (stmt);
        }
        --_indentLvl;
    }
    --_indentLvl;
    --_indentLvl;
}

void
Dumper::dumpLiteralExpr (LiteralExpr *le) {
    print ("LiteralExpr: " + le->Value () + '\n');
}

void
Dumper::dumpBinaryExpr (BinaryExpr *be) {
    print ("BinaryExpr: " + std::string (BinOpToString (be->Op ())) + '\n');
    ++_indentLvl;
    dumpExpr (be->Lhs ());
    dumpExpr (be->Rhs ());
    --_indentLvl;
}

void
Dumper::dumpUnaryExpr (UnaryExpr *ue) {
    print ("UnaryExpr: " + std::string (UnOpToString (ue->Op ())) + '\n');
    ++_indentLvl;
    dumpExpr (ue->Rhs ());
    --_indentLvl;
}

void
Dumper::dumpVarExpr (VarExpr *ve) {
    print ("VarExpr: " + ve->Name ().Val + '\n');
}

void
Dumper::dumpFuncCall (FuncCall *fc) {
    print ("FuncCall: " + fc->Name ().Val + '\n');
    ++_indentLvl;
    for (auto &a : fc->Args ()) {
        dumpExpr (a);
    }
    --_indentLvl;
}

void
Dumper::dumpAsgnExpr (AsgnExpr *ae) {
    print ("AsgnExpr: " + std::string (AsgnOpToString (ae->Op ())) + '\n');
    ++_indentLvl;
    dumpExpr (ae->Ptr ());
    dumpExpr (ae->Init ());
    --_indentLvl;
}

void
Dumper::dumpFieldExpr (FieldExpr *fe) {
    print ("FieldExpr: " + fe->Name ().Val + '\n');
    ++_indentLvl;

    print ("From:\n");
    ++_indentLvl;
    dumpExpr (fe->Base ());
    --_indentLvl;

    --_indentLvl;
}

void
Dumper::dumpStructInstance (StructInstance *si) {
    print ("StructInstance: " + si->Path ().Val + '\n');
    ++_indentLvl;

    print ("Fields:\n");
    ++_indentLvl;
    for (const auto &[name, expr] : si->Fields ()) {
        print (name.Val + ":\n");
        ++_indentLvl;
        dumpExpr (expr);
        --_indentLvl;
    }
    --_indentLvl;

    --_indentLvl;
}

void
Dumper::dumpMethodCall (MethodCall *mc) {
    print ("MethodCall: " + mc->Name ().Val + '\n');
    ++_indentLvl;
    for (auto &a : mc->Args ()) {
        dumpExpr (a);
    }

    print ("From:\n");
    ++_indentLvl;
    dumpExpr (mc->Base ());
    --_indentLvl;

    --_indentLvl;
}

void
Dumper::dumpTernaryExpr (TernaryExpr *te) {
    print ("TernaryExpr:\n");
    ++_indentLvl;

    print ("Cond:\n");
    ++_indentLvl;
    dumpExpr (te->Cond ());
    --_indentLvl;

    print ("TrueVal:\n");
    ++_indentLvl;
    dumpExpr (te->TrueVal ());
    --_indentLvl;

    print ("FalseVal:\n");
    ++_indentLvl;
    dumpExpr (te->FalseVal ());
    --_indentLvl;

    --_indentLvl;
}

void
Dumper::dumpCastExpr (CastExpr *ce) {
    print ("CastExpr:\n");
    ++_indentLvl;

    print ("CastTo: " + ce->Type ()->ToString () + '\n');

    print ("Expr:\n");
    ++_indentLvl;
    dumpExpr (ce->GetExpr ());
    --_indentLvl;

    --_indentLvl;
}

void
Dumper::dumpRefExpr (RefExpr *re) {
    print ("RefExpr:\n");
    ++_indentLvl;
    dumpExpr (re->GetExpr ());
    --_indentLvl;
}

void
Dumper::dumpDerefExpr (DerefExpr *de) {
    print ("DerefExpr:\n");
    ++_indentLvl;
    dumpExpr (de->GetExpr ());
    --_indentLvl;
}

void
Dumper::dumpNilExpr (NilExpr *ne) {
    print ("NilExpr\n");
}

void
Dumper::dumpTypeExpr (TypeExpr *te) {
    print ("TypeExpr: " + te->Type ()->ToString () + '\n');
}

}
