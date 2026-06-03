#pragma once
#include <basic/symbols/struct.h>
#include <basic/types/type.h>
#include <hir/node.h>
#include <vector>

namespace veo::hir {

class StructInstance : public Node {
    std::vector<std::pair<size_t, Node *>> _fields;
    symbols::Struct                       *_base;
    basic::Type                           *_type;

public:
    StructInstance (
        std::vector<std::pair<size_t, Node *>> fields,
        symbols::Struct                       *base,
        basic::Type                           *type,
        llvm::SMLoc                            start,
        llvm::SMLoc                            end)
        : _fields (std::move (fields)),
          _base (base),
          _type (type),
          Node (NodeKind::StructInstance, start, end) {}

    hir_classof (StructInstance);

    std::vector<std::pair<size_t, Node *>> &
    Fields () {
        return _fields;
    }

    symbols::Struct *
    BaseSymbol () const {
        return _base;
    }

    basic::Type *
    Type () const {
        return _type;
    }
};

}
