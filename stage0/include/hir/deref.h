#pragma once
#include <basic/types/type.h>
#include <hir/node.h>

namespace veo::hir {

class DerefExpr : public Node {
    Node        *_expr;
    basic::Type *_type;

public:
    DerefExpr (Node *expr, basic::Type *type, llvm::SMLoc start, llvm::SMLoc end)
        : _expr (expr), _type (type), Node (NodeKind::DerefExpr, start, end) {}

    hir_classof (DerefExpr);

    Node *
    Expr () const {
        return _expr;
    }

    basic::Type *
    Type () const {
        return _type;
    }
};

}
