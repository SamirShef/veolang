#pragma once
#include <basic/symbols/struct.h>
#include <hir/node.h>
#include <vector>

namespace veo::hir {

class StructInstance : public Node {
    std::vector<std::pair<size_t, Node *>> _fields;
    symbols::Struct                       *_base;

public:
    StructInstance (
        std::vector<std::pair<size_t, Node *>> fields,
        symbols::Struct                       *base,
        llvm::SMLoc                            start,
        llvm::SMLoc                            end)
        : _fields (std::move (fields)),
          _base (base),
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
};

}
