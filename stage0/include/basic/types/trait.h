#pragma once
#include <basic/symbols/trait.h>
#include <bitcode/deserializer.h>

namespace veo::basic {

class TraitType : public Type {
    friend class bitcode::Deserializer;
    symbols::Trait *_base;

public:
    explicit TraitType (symbols::Trait *base) : _base (base), Type (TypeKind::Trait) {}

    type_classof (Trait);

    symbols::Trait *
    BaseSymbol () const {
        return _base;
    }

    std::string
    ToString () const override;
};

}
