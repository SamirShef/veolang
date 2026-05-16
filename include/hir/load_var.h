#pragma once
#include <basic/types/type.h>
#include <cstddef>
#include <hir/node.h>

namespace veo::hir {

class LoadVar : public Node {
    size_t       _id;
    basic::Type *_type;
    bool         _isGlobal;

public:
    LoadVar (
        size_t id, basic::Type *type, bool isGlobal, llvm::SMLoc start, llvm::SMLoc end)
        : _id (id),
          _type (type),
          _isGlobal (isGlobal),
          Node (NodeKind::LoadVar, start, end) {}

    hir_classof (LoadVar);

    size_t
    Id () const {
        return _id;
    }

    basic::Type *
    Type () const {
        return _type;
    }

    bool
    IsGlobal () const {
        return _isGlobal;
    }
};

}
