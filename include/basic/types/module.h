#pragma once
#include <basic/symbols/module.h>
#include <basic/types/type.h>

namespace veo::basic {

class ModuleType : public Type {
    symbols::Module *_base;

public:
    explicit ModuleType (symbols::Module *base) : _base (base), Type (TypeKind::Module) {}

    type_classof (Module);

    symbols::Module *
    Base () const {
        return _base;
    }

    std::string
    ToString () const override {
        return _base->ToString ();
    }
};

}
