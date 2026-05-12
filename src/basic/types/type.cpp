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
operator== (const Type &lhs, const Type &rhs) {
    if (&lhs == &rhs) {
        return true;
    }
    if (lhs.Kind () != rhs.Kind ()) {
        return false;
    }

    switch (rhs.Kind ()) {
    case TypeKind::Integer: {
        const auto *ilhs = lhs.AsInteger ();
        const auto *irhs = rhs.AsInteger ();
        return ilhs->BitWidth () == irhs->BitWidth ()
               && ilhs->IsUnsigned () == irhs->IsUnsigned ();
    }
    case TypeKind::Floating: {
        const auto *flhs = lhs.AsFloating ();
        const auto *frhs = rhs.AsFloating ();
        return flhs->GetFloatingKind () == frhs->GetFloatingKind ();
    }
    default: return true;
    }
}

}
