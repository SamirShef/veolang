#pragma once
#include <basic/name.h>
#include <basic/symbols/variable.h>
#include <basic/types/type.h>
#include <hir/node.h>

namespace veo::hir {

class VarDef : public Node {
    basic::NameObj     _name;
    basic::Type       *_type;
    Node              *_expr;
    bool               _isConst;
    symbols::Variable *_base;

public:
    VarDef (
        basic::NameObj     name,
        basic::Type       *type,
        Node              *expr,
        bool               isConst,
        llvm::SMLoc        start,
        llvm::SMLoc        end,
        symbols::Variable *base)
        : _name (std::move (name)),
          _type (type),
          _expr (expr),
          _isConst (isConst),
          _base (base),
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

    symbols::Variable *
    BaseSymbol () const {
        return _base;
    }
};

}
