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
        auto *base = AsAlias ()->Base ();
        if (base == nullptr) {
            return nullptr;
        }
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
    if (lAnon == nullptr || rAnon == nullptr) {
        return lAnon == rAnon;
    }
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
        if (slhs->BaseSymbol () == nullptr || srhs->BaseSymbol () == nullptr) {
            return slhs->BaseSymbol () == srhs->BaseSymbol ();
        }
        return *slhs->BaseSymbol () == *srhs->BaseSymbol ();
    }
    case TypeKind::Trait: {
        const auto *tlhs = lAnon->AsTrait ();
        const auto *trhs = rAnon->AsTrait ();
        if (tlhs->BaseSymbol () == nullptr || trhs->BaseSymbol () == nullptr) {
            return tlhs->BaseSymbol () == trhs->BaseSymbol ();
        }
        return *tlhs->BaseSymbol () == *trhs->BaseSymbol ();
    }
    case TypeKind::Named: {
        const auto *nlhs = lAnon->AsNamed ();
        const auto *nrhs = rAnon->AsNamed ();
        return nlhs->Path () == nrhs->Path ();
    }
    case TypeKind::Pointer: {
        const auto *plhs = lAnon->AsPointer ();
        const auto *prhs = rAnon->AsPointer ();
        if (plhs->Base () == nullptr || prhs->Base () == nullptr) {
            return plhs->Base () == prhs->Base ();
        }
        return *plhs->Base () == *prhs->Base ();
    }
    case TypeKind::Alias: {
        const auto *alhs = lAnon->AsAlias ();
        const auto *arhs = rAnon->AsAlias ();
        if (alhs->Name ().Val != arhs->Name ().Val) {
            return false;
        }
        if (alhs->Base () == nullptr || arhs->Base () == nullptr) {
            return alhs->Base () == arhs->Base ();
        }
        return *alhs->Base () == *arhs->Base ();
    }
    case TypeKind::TraitThis: {
        const auto *tlhs = lAnon->AsTraitThis ();
        const auto *trhs = rAnon->AsTraitThis ();
        if (tlhs->Trait () == nullptr || trhs->Trait () == nullptr) {
            return tlhs->Trait () == trhs->Trait ();
        }
        return *tlhs->Trait () == *trhs->Trait ();
    }
    case TypeKind::Module: {
        const auto *mlhs = lAnon->AsModule ();
        const auto *mrhs = rAnon->AsModule ();
        if (mlhs->Base () == nullptr || mrhs->Base () == nullptr) {
            return mlhs->Base () == mrhs->Base ();
        }
        return *mlhs->Base () == *mrhs->Base ();
    }
    default: return true; // BoolType, CharType, NothType and SizeType
    }
}

}
