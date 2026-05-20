#pragma once
#include <basic/symbols/struct.h>

namespace veo::basic {

class StructType : public Type {
    symbols::Struct *_base;

public:
    explicit StructType (symbols::Struct *base) : _base (base), Type (TypeKind::Struct) {}

    type_classof (Struct);

    symbols::Struct *
    BaseSymbol () const {
        return _base;
    }

    std::string
    ToString () const override;
};

}
