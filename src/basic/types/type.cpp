#include <basic/types/all.h>

namespace veo::basic {

#define as(kind)                                                                         \
    const kind##Type *Type::As##kind () const { return llvm::cast<kind##Type> (this); }

as (Integer);
as (Size);
as (Floating);
as (Bool);
as (Char);
as (Struct);
as (Trait);
as (Named);
as (Pointer);
as (Noth);

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
    case TypeKind::Struct: {
        const auto *slhs = lhs.AsStruct ();
        const auto *srhs = rhs.AsStruct ();
        return *slhs->BaseSymbol () == *srhs->BaseSymbol ();
    }
    case TypeKind::Named: {
        const auto *nlhs = lhs.AsNamed ();
        const auto *nrhs = rhs.AsNamed ();
        return nlhs->Path () == nrhs->Path ();
    }
    case TypeKind::Pointer: {
        const auto *plhs = lhs.AsPointer ();
        const auto *prhs = rhs.AsPointer ();
        return *plhs->Base () == *prhs->Base ();
    }
    default: return true; // BoolType, CharType, NothType and SizeType
    }
}

}
