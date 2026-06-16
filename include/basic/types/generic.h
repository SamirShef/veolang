#pragma once
#include <basic/types/type.h>

namespace veo::basic {

class GenericType : public Type {
    std::string _name;

public:
    explicit GenericType (std::string name)
        : _name (std::move (name)), Type (TypeKind::Generic) {}

    type_classof (Generic);

    const std::string &
    Name () const {
        return _name;
    }

    std::string
    ToString () const override {
        return _name;
    }
};

}
