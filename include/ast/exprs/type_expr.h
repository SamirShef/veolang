#pragma once
#include <ast/expr.h>
#include <basic/types/type.h>

namespace veo::ast {

class TypeExpr : public Expr {
    basic::Type *_type;

public:
    TypeExpr (basic::Type *type, llvm::SMLoc start, llvm::SMLoc end)
        : _type (type), Expr (NodeKind::TypeExpr, start, end) {}

    ast_classof (TypeExpr);

    basic::Type *
    Type () const {
        return _type;
    }
};

}
