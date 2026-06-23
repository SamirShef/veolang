#pragma once
#include <basic/types/type.h>

namespace veo::basic {

class IntegerType : public Type {
    unsigned _bitWidth;
    bool     _isUnsigned;

public:
    explicit IntegerType (unsigned bitWidth, bool isUnsigned = false)
        : _bitWidth (bitWidth), _isUnsigned (isUnsigned), Type (TypeKind::Integer) {}

    type_classof (Integer);

    unsigned
    BitWidth () const {
        return _bitWidth;
    }

    bool
    IsUnsigned () const {
        return _isUnsigned;
    }

    std::string
    ToString () const override {
        return (IsUnsigned () ? "u" : "i") + std::to_string (BitWidth ());
    }
};

}
