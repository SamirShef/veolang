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
as (Alias);
as (TraitThis);
as (Module);

#undef as

const Type *
Type::CanonicalType () const {
    if (IsAlias ()) {
        return AsAlias ()->Base ()->CanonicalType ();
    }
    return this;
}

Type *
Type::CanonicalType () {
    if (IsAlias ()) {
        return AsAlias ()->Base ()->CanonicalType ();
    }
    return this;
}

bool
operator== (const Type &lhs, const Type &rhs) {
    const auto *lAnon = lhs.CanonicalType ();
    const auto *rAnon = rhs.CanonicalType ();
    if (lAnon == rAnon) {
        return true;
    }
    if (lAnon->Kind () != rAnon->Kind ()) {
        return false;
    }

    switch (lAnon->Kind ()) {
    case TypeKind::Integer: {
        const auto *ilhs = lAnon->AsInteger ();
        const auto *irhs = rAnon->AsInteger ();
        return ilhs->BitWidth () == irhs->BitWidth ()
               && ilhs->IsUnsigned () == irhs->IsUnsigned ();
    }
    case TypeKind::Floating: {
        const auto *flhs = lAnon->AsFloating ();
        const auto *frhs = rAnon->AsFloating ();
        return flhs->GetFloatingKind () == frhs->GetFloatingKind ();
    }
    case TypeKind::Struct: {
        const auto *slhs = lAnon->AsStruct ();
        const auto *srhs = rAnon->AsStruct ();
        return *slhs->BaseSymbol () == *srhs->BaseSymbol ();
    }
    case TypeKind::Named: {
        const auto *nlhs = lAnon->AsNamed ();
        const auto *nrhs = rAnon->AsNamed ();
        return nlhs->Path () == nrhs->Path ();
    }
    case TypeKind::Pointer: {
        const auto *plhs = lAnon->AsPointer ();
        const auto *prhs = rAnon->AsPointer ();
        return *plhs->Base () == *prhs->Base ();
    }
    case TypeKind::Alias: {
        const auto *alhs = lAnon->AsAlias ();
        const auto *arhs = rAnon->AsAlias ();
        return alhs->Name ().Val == arhs->Name ().Val && *alhs->Base () == *arhs->Base ();
    }
    case TypeKind::TraitThis: {
        const auto *tlhs = lAnon->AsTraitThis ();
        const auto *trhs = rAnon->AsTraitThis ();
        return *tlhs->Trait () == *trhs->Trait ();
    }
    case TypeKind::Module: {
        const auto *mlhs = lAnon->AsModule ();
        const auto *mrhs = rAnon->AsModule ();
        return *mlhs->Base () == *mrhs->Base ();
    }
    default: return true; // BoolType, CharType, NothType and SizeType
    }
}

}
