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
    bool               _isGlobal;
    symbols::Variable *_base;

public:
    VarDef (
        basic::NameObj     name,
        basic::Type       *type,
        Node              *expr,
        bool               isConst,
        bool               isGlobal,
        llvm::SMLoc        start,
        llvm::SMLoc        end,
        symbols::Variable *base)
        : _name (std::move (name)),
          _type (type),
          _expr (expr),
          _isConst (isConst),
          _isGlobal (isGlobal),
          _base (base),
          Node (NodeKind::VarDef, start, end) {}

    bool
    operator== (const VarDef *other) const {
        if (this == other) {
            return true;
        }

        return _name == other->_name && _type == other->_type
               && _isConst == other->_isConst && *_base == *other->_base;
    }

    bool
    operator!= (const VarDef *other) const {
        return !(*this == other);
    }

    hir_classof (VarDef);

    basic::NameObj
    Name () const {
        return _name;
    }

    basic::Type *
    Type () const {
        return _type;
    }

    void
    SetType (basic::Type *type) {
        _type = type;
    }

    Node *
    Init () const {
        return _expr;
    }

    bool
    IsConst () const {
        return _isConst;
    }

    bool
    IsGlobal () const {
        return _isGlobal;
    }

    symbols::Variable *
    BaseSymbol () const {
        return _base;
    }
};

}
