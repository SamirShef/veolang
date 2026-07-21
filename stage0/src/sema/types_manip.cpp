#include <basic/types/alias.h>
#include <basic/types/floating.h>
#include <basic/types/integer.h>
#include <basic/types/named.h>
#include <basic/types/pointer.h>
#include <basic/types/struct.h>
#include <basic/types/trait.h>
#include <llvm/Support/raw_ostream.h>
#include <sema/sema.h>

namespace veo {

using namespace symbols;

Type *
Sema::resolveType (Type **type) {
    if (*type == nullptr) {
        return *type;
    }
    if ((*type)->IsPointer ()) {
        auto *ptrBase = (*type)->AsPointer ()->Base ();
        resolveType (&ptrBase);
        *type = createType<PointerType> (ptrBase);
        return *type;
    }
    if (!(*type)->IsNamed ()) {
        return *type;
    }

    const auto *named  = (*type)->AsNamed ();
    Module     *curMod = _mod;
    const auto &path   = named->Path ();
    if (path.size () == 1) {
        if (Type *localType = lookupLocalType (path[0].Val)) {
            *type = localType;
            return *type;
        }
    }
    for (size_t i = 0; i < path.size () - 1; ++i) {
        const auto &name = path[i];
        if (auto it = curMod->Submods.find (name.Val); it != curMod->Submods.end ()) {
            curMod = it->second;
        } else if (auto it = curMod->Imports.find (name.Val);
                   it != curMod->Imports.end ()) {
            curMod = it->second;
        } else {
            _diag
                .Report (
                    DiagCode::ECannotFindMod,
                    "cannot find module '" + name.Val + "'",
                    Severity::Error)
                .AddSpan (name.Start, name.End, "no such module found");
            return nullptr;
        }
    }
    const auto &typeName = path.back ();
    if (auto it = curMod->Structs.find (typeName.Val); it != curMod->Structs.end ()) {
        if (curMod != _mod && it->second.Access != ast::AccessModifier::Pub) {
            _diag
                .Report (
                    DiagCode::ECannotAccessToPrivMember,
                    "cannot use private type '" + typeName.Val + "' from module '"
                        + curMod->ToString () + "'",
                    Severity::Error)
                .AddSpan (path.front ().Start, path.back ().End);
        }
        *type = createType<StructType> (&it->second);
        return *type;
    }
    if (auto it = curMod->Traits.find (typeName.Val); it != curMod->Traits.end ()) {
        if (curMod != _mod && it->second.Access != ast::AccessModifier::Pub) {
            _diag
                .Report (
                    DiagCode::ECannotAccessToPrivMember,
                    "cannot use private type '" + typeName.Val + "' from module '"
                        + curMod->ToString () + "'",
                    Severity::Error)
                .AddSpan (path.front ().Start, path.back ().End);
        }
        *type = createType<TraitType> (&it->second);
        return *type;
    }

    _diag
        .Report (
            DiagCode::ECannotFindType,
            "cannot find type '" + typeName.Val + "' in "
                + (path.size () == 1 ? "this scope" : curMod->ToString ()),
            Severity::Error)
        .AddSpan (typeName.Start, typeName.End, "not found");
    return nullptr;
}

Type *
Sema::getCommonType (Type *lhs, Type *rhs, llvm::SMLoc start, llvm::SMLoc end) {
    if (lhs == nullptr || rhs == nullptr) {
        return nullptr;
    }
    if (*lhs == *rhs) {
        return lhs;
    }
    if (lhs->IsInteger () && rhs->IsInteger ()) {
        return getCommonIngeter (lhs, rhs, start, end);
    }
    if (lhs->IsFloating () && rhs->IsFloating ()) {
        return getCommonFloating (lhs, rhs, start, end);
    }
    if (lhs->IsFloating () && rhs->IsInteger ()
        || lhs->IsInteger () && rhs->IsFloating ()) {
        return getCommonFloatingAndInteger (lhs, rhs, start, end);
    }
    if (lhs->IsPointer () && rhs->IsIntOrSize ()
        || lhs->IsIntOrSize () && rhs->IsPointer ()) {
        return lhs->IsPointer () ? lhs : rhs;
    }
    _diag
        .Report (
            DiagCode::ECannotFindCommonType,
            "cannot find common type between '" + typeToString (lhs) + "' and '"
                + typeToString (rhs) + "'",
            Severity::Error)
        .AddSpan (start, end)
        .AddNote ("сonsider using an explicit cast");
    return nullptr;
}

Type *
Sema::getCommonIngeter (Type *lhs, Type *rhs, llvm::SMLoc start, llvm::SMLoc end) {
    const auto *lhsT = lhs->AsInteger ();
    const auto *rhsT = rhs->AsInteger ();
    if (lhsT->IsUnsigned () == rhsT->IsUnsigned ()) {
        if (lhsT->BitWidth () > rhsT->BitWidth ()) {
            return lhs;
        }
        return rhs;
    }
    const auto *unsignedType    = lhsT->IsUnsigned () ? lhsT : rhsT;
    auto       *unsignedTypeSrc = lhsT->IsUnsigned () ? lhs : rhs;
    const auto *signedType      = lhsT->IsUnsigned () ? rhsT : lhsT;
    auto       *signedTypeSrc   = lhsT->IsUnsigned () ? rhs : lhs;
    if (unsignedType->BitWidth () >= signedType->BitWidth ()) {
        return unsignedTypeSrc;
    }
    return signedTypeSrc;
}

Type *
Sema::getCommonFloating (Type *lhs, Type *rhs, llvm::SMLoc start, llvm::SMLoc end) {
    const auto *lhsT = lhs->AsFloating ();
    const auto *rhsT = rhs->AsFloating ();
    if (lhsT->IsDouble ()) {
        return lhs;
    }
    return rhs;
}

Type *
Sema::getCommonFloatingAndInteger (
    Type *lhs, Type *rhs, llvm::SMLoc start, llvm::SMLoc end) {
    const auto *floatingType
        = lhs->IsFloating () ? lhs->AsFloating () : rhs->AsFloating ();
    const auto *integerType = lhs->IsFloating () ? rhs->AsInteger () : lhs->AsInteger ();
    // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
    const unsigned maxWidth
        = 2U
          << (4U
              + static_cast<unsigned> (
                  floatingType->IsDouble ())); /* 2^(5 + IsDouble()) */
    // NOLINTEND(cppcoreguidelines-avoid-magic-numbers)
    if (integerType->BitWidth () >= maxWidth) {
        _diag
            .Report (
                DiagCode::WLossPrecision,
                "casting types can lead to loss of precision",
                Severity::Warning)
            .AddSpan (start, end);
    }
    return lhs->IsFloating () ? lhs : rhs;
}

bool
Sema::compareTypesWithThis (Type *traitTy, Type *implTy, Type *concreteTarget) {
    if (traitTy == nullptr || implTy == nullptr) {
        return traitTy == implTy;
    }

    if (traitTy->IsTraitThis ()) {
        Type *unwrappedImpl = implTy->CanonicalType ();
        return unwrappedImpl == concreteTarget;
    }

    if (traitTy->IsPointer () && implTy->IsPointer ()) {
        return compareTypesWithThis (
            traitTy->AsPointer ()->Base (),
            implTy->AsPointer ()->Base (),
            concreteTarget);
    }
    return *traitTy == *implTy;
}

}
