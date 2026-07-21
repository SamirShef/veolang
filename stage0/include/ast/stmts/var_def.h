#pragma once
#include <ast/expr.h>
#include <ast/stmt.h>
#include <basic/name.h>
#include <basic/types/type.h>

namespace veo::ast {

class VarDef : public Stmt {
    basic::NameObj _name;
    basic::Type   *_type;
    Expr          *_expr;
    bool           _isConst;

public:
    VarDef (
        basic::NameObj name,
        basic::Type   *type,
        Expr          *expr,
        bool           isConst,
        AccessModifier access,
        llvm::SMLoc    start,
        llvm::SMLoc    end)
        : _name (std::move (name)),
          _type (type),
          _expr (expr),
          _isConst (isConst),
          Stmt (access, NodeKind::VarDef, start, end) {}

    ast_classof (VarDef);

    basic::NameObj
    Name () const {
        return _name;
    }

    basic::Type *&
    Type () {
        return _type;
    }

    Expr *
    Init () const {
        return _expr;
    }

    bool
    IsConst () const {
        return _isConst;
    }
};

}
