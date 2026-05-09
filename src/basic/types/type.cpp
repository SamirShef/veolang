#include <basic/types/all.h>

namespace veo::basic {

bool
Type::operator== (const Type &other) {
    if (this == &other) {
        return true;
    }
    if (Kind () != other.Kind ()) {
        return false;
    }

    switch (other.Kind ()) {
    case TypeKind::Integer: {
        const auto *lhs = AsInteger ();
        const auto *rhs = other.AsInteger ();
        return lhs->BitWidth () == rhs->BitWidth ()
               && lhs->IsUnsigned () == rhs->IsUnsigned ();
    }
    case TypeKind::Floating: {
        const auto *lhs = AsFloating ();
        const auto *rhs = AsFloating ();
        return lhs->FloatingKind () == rhs->FloatingKind ();
    }
    default: return true;
    }
}

}
