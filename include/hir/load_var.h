#pragma once
#include <basic/types/type.h>
#include <hir/node.h>
#include <hir/var_def.h>

namespace veo::hir {

class LoadVar : public Node {
    VarDef      *_ptr;
    basic::Type *_type;
    bool         _isGlobal;

public:
    LoadVar (
        VarDef *ptr, basic::Type *type, bool isGlobal, llvm::SMLoc start, llvm::SMLoc end)
        : _ptr (ptr),
          _type (type),
          _isGlobal (isGlobal),
          Node (NodeKind::LoadVar, start, end) {}

    hir_classof (LoadVar);

    VarDef *
    Ptr () const {
        return _ptr;
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
