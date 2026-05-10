#pragma once
#include <basic/types/type.h>
#include <cstdint>

namespace veo::basic {

enum class FloatingKind : uint8_t { Float, Double };

class FloatingType : public Type {
    FloatingKind _floatingKind;

public:
    explicit FloatingType (FloatingKind kind)
        : _floatingKind (kind), Type (TypeKind::Floating) {}

    type_classof (Floating);

    FloatingKind
    GetFloatingKind () const {
        return _floatingKind;
    }

    bool
    IsFloat () const {
        return _floatingKind == FloatingKind::Float;
    }

    bool
    IsDouble () const {
        return _floatingKind == FloatingKind::Double;
    }
};

}
