#pragma once
#include <basic/types/type.h>
#include <hir/node.h>

namespace veo::hir {

class LoadGlobalVarByName : public Node {
    std::string  _name;
    basic::Type *_type;

public:
    LoadGlobalVarByName (
        std::string name, basic::Type *type, llvm::SMLoc start, llvm::SMLoc end)
        : _name (std::move (name)),
          _type (type),
          Node (NodeKind::LoadGlobalVarByName, start, end) {}

    hir_classof (LoadGlobalVarByName);

    const std::string &
    Name () const {
        return _name;
    }

    basic::Type *
    Type () const {
        return _type;
    }
};

}
