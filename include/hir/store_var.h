#pragma once
#include <hir/node.h>

namespace veo::hir {

class StoreVar : public Node {
    Node *_ptr;
    Node *_expr;

public:
    StoreVar (Node *ptr, Node *expr, llvm::SMLoc start, llvm::SMLoc end)
        : _ptr (ptr), _expr (expr), Node (NodeKind::StoreVar, start, end) {}

    hir_classof (StoreVar);

    Node *
    Ptr () const {
        return _ptr;
    }

    Node *
    Expr () const {
        return _expr;
    }
};

}
