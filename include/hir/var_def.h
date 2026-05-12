#pragma once
#include <basic/name.h>
#include <basic/types/type.h>
#include <hir/node.h>

namespace veo::hir {

class VarDef : public Node {
    basic::NameObj _name;
    basic::Type   *_type;
    Node          *_expr;
    bool           _isConst;

public:
    VarDef (
        basic::NameObj name,
        basic::Type   *type,
        Node          *expr,
        bool           isConst,
        llvm::SMLoc    start,
        llvm::SMLoc    end)
        : _name (std::move (name)),
          _type (type),
          _expr (expr),
          _isConst (isConst),
          Node (NodeKind::VarDef, start, end) {}

    hir_classof (VarDef);

    basic::NameObj
    Name () const {
        return _name;
    }

    basic::Type *
    Type () const {
        return _type;
    }

    Node *
    Init () const {
        return _expr;
    }

    bool
    IsConst () const {
        return _isConst;
    }
};

}
