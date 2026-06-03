#pragma once
#include <basic/types/type.h>
#include <cstddef>
#include <hir/node.h>

namespace veo::hir {

class FieldExpr : public Node {
    Node        *_base;
    size_t       _index;
    basic::Type *_type;

public:
    FieldExpr (
        Node *base, size_t index, basic::Type *type, llvm::SMLoc start, llvm::SMLoc end)
        : _base (base),
          _index (index),
          _type (type),
          Node (NodeKind::FieldExpr, start, end) {}

    hir_classof (FieldExpr);

    Node *
    Base () const {
        return _base;
    }

    size_t
    Index () const {
        return _index;
    }

    basic::Type *
    Type () const {
        return _type;
    }
};

}
