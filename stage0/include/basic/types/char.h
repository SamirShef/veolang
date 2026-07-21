#pragma once
#include <basic/types/type.h>

namespace veo::basic {

class CharType : public Type {
public:
    CharType () : Type (TypeKind::Char) {}

    type_classof (Char);

    std::string
    ToString () const override {
        return "char";
    }
};

}
