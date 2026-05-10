#include <basic/types/all.h>

namespace veo::basic {

#define as(kind)                                                                         \
    const kind##Type *Type::As##kind () const { return llvm::cast<kind##Type> (this); }

as (Integer);
as (Floating);
as (Bool);
as (Char);

#undef as

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
        return lhs->GetFloatingKind () == rhs->GetFloatingKind ();
    }
    default: return true;
    }
}

}
