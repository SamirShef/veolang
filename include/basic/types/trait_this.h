#pragma once
#include <basic/symbols/trait.h>
#include <basic/types/type.h>

namespace veo::basic {

class TraitThisType : public Type {
    symbols::Trait *_trait;

public:
    explicit TraitThisType (symbols::Trait *trait)
        : Type (TypeKind::TraitThis), _trait (trait) {}

    type_classof (TraitThis);

    symbols::Trait *
    Trait () const {
        return _trait;
    }

    std::string
    ToString () const override {
        return "This";
    }
};

}
