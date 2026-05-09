#pragma once
#include <basic/types/type.h>

namespace veo::basic {

class CharType : public Type {
public:
    CharType () : Type (TypeKind::Char) {}
};

}
