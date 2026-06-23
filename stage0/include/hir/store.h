#pragma once
#include <basic/types/type.h>
#include <hir/node.h>

namespace veo::hir {

class Store : public Node {
    Node        *_ptr;
    Node        *_expr;
    basic::Type *_type;

public:
    Store (Node *ptr, Node *expr, basic::Type *type, llvm::SMLoc start, llvm::SMLoc end)
        : _ptr (ptr), _expr (expr), _type (type), Node (NodeKind::Store, start, end) {}

    hir_classof (Store);

    Node *
    Ptr () const {
        return _ptr;
    }

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
