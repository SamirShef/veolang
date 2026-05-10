#pragma once
#include <basic/types/type.h>

namespace veo::basic {

class BoolType : public Type {
public:
    BoolType () : Type (TypeKind::Bool) {}

    type_classof (Bool);
};

}
