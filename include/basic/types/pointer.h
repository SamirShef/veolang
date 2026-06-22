#pragma once
#include <basic/types/type.h>
#include <bitcode/deserializer.h>

namespace veo::basic {

class PointerType : public Type {
    friend class bitcode::Deserializer;
    Type *_base;

public:
    explicit PointerType (Type *base) : _base (base), Type (TypeKind::Pointer) {}

    type_classof (Pointer);

    Type *
    Base () const {
        return _base;
    }

    std::string
    ToString () const override {
        return '*' + _base->ToString ();
    }
};

}
