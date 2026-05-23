#pragma once
#include <cstddef>
#include <hir/node.h>

namespace veo::hir {

class FieldExpr : public Node {
    Node  *_base;
    size_t _index;

public:
    FieldExpr (Node *base, size_t index, llvm::SMLoc start, llvm::SMLoc end)
        : _base (base), _index (index), Node (NodeKind::FieldExpr, start, end) {}

    hir_classof (FieldExpr);

    Node *
    Base () const {
        return _base;
    }

    size_t
    Index () const {
        return _index;
    }
};

}
