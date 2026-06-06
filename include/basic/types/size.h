#pragma once
#include <basic/types/type.h>

namespace veo::basic {

class SizeType : public Type {
    bool _isUnsigned;

public:
    explicit SizeType (bool isUnsigned)
        : _isUnsigned (isUnsigned), Type (TypeKind::Size) {}

    type_classof (Size);

    bool
    IsUnsigned () const {
        return _isUnsigned;
    }

    std::string
    ToString () const override {
        return (_isUnsigned ? "u" : "i") + std::string ("size");
    }
};

}
