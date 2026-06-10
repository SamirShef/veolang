#pragma once
#include <basic/name.h>
#include <basic/types/type.h>

namespace veo::basic {

class AliasType : public Type {
    NameObj _name;
    Type   *_base;

public:
    AliasType (NameObj name, Type *base)
        : _name (std::move (name)), _base (base), Type (TypeKind::Alias) {}

    type_classof (Alias);

    NameObj
    Name () const {
        return _name;
    }

    Type *
    Base () const {
        return _base;
    }

    std::string
    ToString () const override {
        return _name.Val + " (aka " + _base->ToString () + ")";
    }
};

}
