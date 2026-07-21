#pragma once
#include <basic/types/type.h>

namespace veo::basic {

class NothType : public Type {
public:
    explicit NothType () : Type (TypeKind::Noth) {}

    type_classof (Noth);

    std::string
    ToString () const override {
        return "noth";
    }
};

}
