#include <basic/types/floating.h>
#include <basic/types/integer.h>
#include <basic/types/struct.h>
#include <basic/types/trait.h>
#include <sema/sema.h>

namespace veo {

hir::CastKind
Sema::castInts (const IntegerType *src, const IntegerType *dst) {
    if (src->BitWidth () < dst->BitWidth ()) {
        return src->IsUnsigned () ? hir::CastKind::ZExt : hir::CastKind::SExt;
    }
    if (src->BitWidth () > dst->BitWidth ()) {
        return hir::CastKind::Trunc;
    }
    return hir::CastKind::BitCast;
}

hir::CastKind
Sema::castFloats (const FloatingType *src, const FloatingType *dst) {
    if (src->IsFloat () && dst->IsDouble ()) {
        return hir::CastKind::FPExt;
    }
    return hir::CastKind::FPTrunc;
}

Sema::SemanticResult
Sema::cast (
    hir::CastKind  kind,
    Type          *dst,
    SemanticResult val,
    llvm::SMLoc    start,
    llvm::SMLoc    end) {
    auto *castNode = _builder.CreateCast (kind, dst, val.Node, start, end);

    Value newValue = val.Val.value ();
    if (newValue.Kind == ValueKind::Const) {
        ValueData newData;
        std::visit (
            [&] (auto &val) {
                using T = std::decay_t<decltype (val)>;
                if constexpr (std::is_arithmetic_v<T>) {
                    if (dst->IsIntOrSize ()) {
                        newData = ValueData (static_cast<int64_t> (val));
                    } else if (dst->IsFloating ()) {
                        newData = ValueData (static_cast<double> (val));
                    }
                }
            },
            val.Val->Data);

        newValue = Value (ValueKind::Const, newData, dst);
    } else {
        newValue.Type = dst;
    }

    auto res = SemanticResult (newValue, castNode);
    return implicitlyCast (res, &dst, start, end);
}

bool
Sema::canImplicitCast (Sema::SemanticResult val, Type **expectedType) {
    if (!val.Val.has_value ()) {
        return false;
    }
    resolveType (&val.Val->Type);
    resolveType (expectedType);
    if (val.Val->Type == nullptr || *expectedType == nullptr) {
        return false;
    }

    auto *src = val.Val->Type->CanonicalType ();
    auto *dst = (*expectedType)->CanonicalType ();

    if (*src == *dst) {
        return true;
    }
    if (src->IsInteger () && dst->IsInteger ()) {
        const auto *srcI = src->AsInteger ();
        const auto *dstI = dst->AsInteger ();
        if (dstI->BitWidth () >= srcI->BitWidth ()) {
            if (srcI->IsUnsigned () == dstI->IsUnsigned ()
                || dstI->BitWidth () > srcI->BitWidth ()) {
                return true;
            }
        }
    }
    if (src->IsFloating () && dst->IsFloating ()) {
        const auto *srcF = src->AsFloating ();
        const auto *dstF = dst->AsFloating ();
        return (int) dstF->GetFloatingKind () >= (int) srcF->GetFloatingKind ();
    }
    if (src->IsInteger () && dst->IsFloating ()) {
        return true;
    }
    if (dst->IsTrait ()) {
        auto *tSym = dst->AsTrait ()->BaseSymbol ();
        if (src->IsStruct ()) {
            auto *sSym = src->AsStruct ()->BaseSymbol ();
            return sSym->TraitsImplements.contains (tSym);
        }
        return std::ranges::find (_mod->PrimitiveTraitsImplement[src], tSym)
               != _mod->PrimitiveTraitsImplement[src].end ();
    }
    return false;
}

bool
Sema::canExplicitCast (Type *src, Type *dst) {
    if (*src == *dst) {
        return true;
    }
    auto isScalar = [] (Type *t) {
        return t->IsInteger () || t->IsSize () || t->IsFloating () || t->IsChar ()
               || t->IsBool ();
    };
    if (isScalar (src) && isScalar (dst)) {
        return true;
    }
    if (src->IsPointer () && dst->IsSize ()) {
        return true;
    }
    if (src->IsSize () && dst->IsPointer ()) {
        return true;
    }
    if (src->IsPointer () && dst->IsPointer ()) {
        return true;
    }

    return false;
}

Sema::SemanticResult
Sema::implicitlyCast (
    SemanticResult val, Type **expectedType, llvm::SMLoc start, llvm::SMLoc end) {
    if (!val.Val.has_value ()) {
        return val;
    }
    resolveType (&val.Val->Type);
    resolveType (expectedType);
    if (val.Val->Type == nullptr || *expectedType == nullptr) {
        return val;
    }

    auto *src = val.Val->Type->CanonicalType ();
    auto *dst = (*expectedType)->CanonicalType ();

    if (*src == *dst) {
        val.Val->Type = src;
        return val;
    }
    if (!canImplicitCast (val, expectedType)) {
        _diag
            .Report (
                DiagCode::ECannotCast,
                "cannot implicitly cast '" + typeToString (val.Val->Type) + "' to '"
                    + typeToString (*expectedType) + "'",
                Severity::Error)
            .AddSpan (start, end);

        return {};
    }

    auto kind = hir::CastKind::BitCast;
    if (src->IsInteger () && dst->IsInteger ()) {
        kind = castInts (src->AsInteger (), dst->AsInteger ());
    } else if (src->IsFloating () && dst->IsFloating ()) {
        kind = castFloats (src->AsFloating (), dst->AsFloating ());
    } else if (src->IsInteger () && dst->IsFloating ()) {
        kind = src->AsInteger ()->IsUnsigned () ? hir::CastKind::UIToFP
                                                : hir::CastKind::SIToFP;
    } else if (src->IsFloating () && dst->IsInteger ()) {
        kind = dst->AsInteger ()->IsUnsigned () ? hir::CastKind::FPToUI
                                                : hir::CastKind::FPToSI;
    }
    return cast (kind, dst, val, start, end);
}

Sema::CastCost
Sema::checkCastCost (Type *src, Type *dst) {
    if (src == nullptr || dst == nullptr) {
        return CastCost::Incompatible;
    }

    src = src->CanonicalType ();
    dst = dst->CanonicalType ();

    if (*src == *dst) {
        return CastCost::Exact;
    }

    if (src->IsInteger () && dst->IsInteger ()) {
        return checkCastCostIntegers (src, dst);
    }

    if (src->IsInteger () && dst->IsFloating ()) {
        return CastCost::SafeImplicit;
    }

    if (src->IsFloating () && dst->IsFloating ()) {
        return checkCastCostFloatings (src, dst);
    }
    if (dst->IsTrait ()) {
        return checkCastCostTraitMatch (src, dst);
    }

    return CastCost::Incompatible;
}

Sema::CastCost
Sema::checkCastCostIntegers (Type *src, Type *dst) {
    const auto *srcI = src->AsInteger ();
    const auto *dstI = dst->AsInteger ();
    if (dstI->BitWidth () >= srcI->BitWidth ()) {
        if (srcI->IsUnsigned () == dstI->IsUnsigned ()
            && dstI->BitWidth () == srcI->BitWidth ()) {
            return CastCost::Exact;
        }
        if (srcI->IsUnsigned () == dstI->IsUnsigned ()
            || dstI->BitWidth () > srcI->BitWidth ()) {
            return CastCost::SafeImplicit;
        }
    }
    return CastCost::Incompatible;
}

Sema::CastCost
Sema::checkCastCostFloatings (Type *src, Type *dst) {
    const auto *srcF = src->AsFloating ();
    const auto *dstF = dst->AsFloating ();

    if (dstF->IsDouble () && srcF->IsFloat ()) {
        return CastCost::SafeImplicit;
    }
    if (dstF->IsFloat () == srcF->IsFloat () || dstF->IsDouble () == srcF->IsDouble ()) {
        return CastCost::Exact;
    }
    return CastCost::Incompatible;
}

Sema::CastCost
Sema::checkCastCostTraitMatch (Type *src, Type *dst) {
    auto *tSym = dst->AsTrait ()->BaseSymbol ();
    if (src->IsStruct ()) {
        auto *sSym = src->AsStruct ()->BaseSymbol ();
        return sSym->TraitsImplements.contains (tSym) ? CastCost::TraitMatch
                                                      : CastCost::Incompatible;
    }
    return std::ranges::find (_mod->PrimitiveTraitsImplement[src], tSym)
                   != _mod->PrimitiveTraitsImplement[src].end ()
               ? CastCost::TraitMatch
               : CastCost::Incompatible;
}

}
