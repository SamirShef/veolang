#pragma once
#include <basic/name.h>
#include <basic/symbols/struct.h>
#include <basic/types/type.h>
#include <hir/node.h>
#include <vector>

namespace veo::hir {

class StructDef : public Node {
    basic::NameObj             _name;
    std::vector<basic::Type *> _fields;
    symbols::Struct           *_base;

public:
    StructDef (
        basic::NameObj             name,
        std::vector<basic::Type *> fields,
        symbols::Struct           *base,
        llvm::SMLoc                start,
        llvm::SMLoc                end)
        : _name (std::move (name)),
          _fields (std::move (fields)),
          _base (base),
          Node (NodeKind::StructDef, start, end) {}

    hir_classof (StructDef);

    basic::NameObj
    Name () const {
        return _name;
    }

    std::vector<basic::Type *> &
    Fields () {
        return _fields;
    }

    symbols::Struct *
    BaseSymbol () const {
        return _base;
    }
};

}
