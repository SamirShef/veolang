#include <basic/types/all.h>
#include <sema/sema.h>

namespace veo {

using namespace symbols;
using namespace ast;

bool
Sema::canFit (Value &val, const Type *targetType) {
    return std::visit (
        [&] (auto &&arg) -> bool {
            using T = std::decay_t<decltype (arg)>;

            if constexpr (std::is_arithmetic_v<T>) {
                if (targetType->IsIntOrSize ()) {
                    unsigned bits       = targetType->IsInteger ()
                                              ? targetType->AsInteger ()->BitWidth ()
                                              : 32;
                    bool     isUnsigned = targetType->IsInteger ()
                                              ? targetType->AsInteger ()->IsUnsigned ()
                                              : targetType->AsSize ()->IsUnsigned ();

                    if (isUnsigned) {
                        if (arg < 0) {
                            return false;
                        }
                        auto uval = static_cast<uint64_t> (arg);
                        auto umax = (bits == 64) ? ~0ULL : (1ULL << bits) - 1;
                        return uval <= umax;
                    }
                    auto min = -static_cast<int64_t> ((1ULL << (bits - 1)));
                    auto max = static_cast<uint64_t> ((1ULL << (bits - 1))) - 1;
                    return arg >= min && static_cast<uint64_t> (arg) <= max;
                }
            }

            return false;
        },
        val.Data);
}

bool
Sema::canApplyAsgnOp (AsgnOp op, Type *type) {
    if (op == AsgnOp::Eq) {
        return true;
    }
    if (type->IsNumber ()) {
        return true;
    }
    return false;
}

bool
Sema::viableFuncCandidate (
    symbols::Function *func, const std::vector<Type *> &args, size_t &costSum) {
    bool viable = true;
    for (size_t i = 0; i < args.size (); ++i) {
        CastCost cost = checkCastCost (args[i], func->Args[i].Type);
        if (cost == CastCost::Incompatible) {
            viable = false;
            break;
        }
        costSum += static_cast<size_t> (cost);
    }
    return viable;
}

bool
Sema::inGlobalScope () const {
    return _vars.size () == 1;
}

bool
Sema::allowInScope (Stmt *stmt, bool allowInGlobal) {
    if (inGlobalScope () != allowInGlobal) {
        _diag
            .Report (
                DiagCode::ENotAllowedInThisScope,
                "statement not allowed in this scope",
                Severity::Error)
            .AddSpan (
                stmt->Start (),
                stmt->End (),
                "statement found outside of a function");
        return false;
    }
    return true;
}

bool
Sema::canAccessField (
    const basic::NameObj  &feName,
    const symbols::Field  &field,
    const symbols::Struct *s,
    bool                   canAccessStatic,
    bool                   canAccessPrivate) {
    if (field.IsStatic && !canAccessStatic) {
        _diag
            .Report (
                DiagCode::ECannotAccessStaticMemberFromInstance,
                "static field '" + feName.Val
                    + "' cannot be accessed through an instance",
                Severity::Error)
            .AddSpan (feName.Start, feName.End)
            .AddNote (
                "use the type name instead to access this field: '" + s->Name.Val + "."
                + feName.Val + "'");
        return false;
    }
    if (!field.IsStatic && canAccessStatic) {
        _diag
            .Report (
                DiagCode::ECannotAccessNonStaticMemberFromType,
                "instance field '" + feName.Val + "' cannot be accessed through a type",
                Severity::Error)
            .AddSpan (feName.Start, feName.End);
        return false;
    }
    if (field.Access != AccessModifier::Pub && !canAccessPrivate) {
        _diag
            .Report (
                DiagCode::ECannotAccessToPrivMember,
                "field '" + feName.Val + "' of struct '" + s->Name.Val + "' is private",
                Severity::Error)
            .AddSpan (feName.Start, feName.End);
        return false;
    }
    return true;
}

bool
Sema::canAccessMethod (
    const basic::NameObj  &mcName,
    const symbols::Method &method,
    basic::Type           *base,
    bool                   canAccessStatic,
    bool                   canAccessPrivate) {
    if (method.IsStatic && !canAccessStatic) {
        _diag
            .Report (
                DiagCode::ECannotAccessStaticMemberFromInstance,
                "static method '" + mcName.Val
                    + "' cannot be accessed through an instance",
                Severity::Error)
            .AddSpan (mcName.Start, mcName.End);
        return false;
    }
    if (!method.IsStatic && canAccessStatic) {
        _diag
            .Report (
                DiagCode::ECannotAccessNonStaticMemberFromType,
                "instance method '" + mcName.Val + "' cannot be accessed through a type",
                Severity::Error)
            .AddSpan (mcName.Start, mcName.End);
        return false;
    }
    if (method.Access != AccessModifier::Pub && !canAccessPrivate) {
        _diag
            .Report (
                DiagCode::ECannotAccessToPrivMember,
                "method '" + mcName.Val + "' of type '" + typeToString (base)
                    + "' is private",
                Severity::Error)
            .AddSpan (mcName.Start, mcName.End);
        return false;
    }
    return true;
}

std::string
Sema::typeToString (const Type *type) {
    if (type == nullptr) {
        return "";
    }
    if (type->IsPointer ()) {
        return '*' + typeToString (type->AsPointer ()->Base ());
    }
    if (type->IsAlias ()) {
        const auto *alias = type->AsAlias ();
        return alias->Name ().Val + " (aka " + typeToString (alias->Base ()) + ")";
    }
    if (!type->IsStruct () && !type->IsTrait ()) {
        return type->ToString ();
    }
    auto    res    = type->IsStruct () ? type->AsStruct ()->BaseSymbol ()->Name.Val
                                       : type->AsTrait ()->BaseSymbol ()->Name.Val;
    Module *curMod = type->IsStruct () ? type->AsStruct ()->BaseSymbol ()->Parent
                                       : type->AsTrait ()->BaseSymbol ()->Parent;
    while (curMod != nullptr) {
        if (*curMod == *_mod) {
            break;
        }
        res    = curMod->Name + '.' + res; // NOLINT
        curMod = curMod->Parent;
    }
    return res;
}

void
Sema::analyzeMethodCallFromAnotherMethod (
    symbols::Method *method, const std::vector<symbols::Method *> &methods) {
    if (!_methodCallFromAnotherMethod.contains (method)) {
        return;
    }

    bool isConst = true;
    for (const auto &m : methods) {
        if (auto it = _methodCallFromAnotherMethod.find (m);
            it != _methodCallFromAnotherMethod.end ()) {
            analyzeMethodCallFromAnotherMethod (m, it->second);
        }
        if (!m->IsConst) {
            isConst = false;
        }
    }
    method->IsConst = isConst;
}

void
Sema::analyzeMethodCallOnConstBase (MethodCall *mc, symbols::Method *method) {
    if (!method->IsConst) {
        _diag
            .Report (
                DiagCode::ECannotCallMethodOnImmutableReceiver,
                "method '" + mc->Name ().Val
                    + "' cannot be called on an immutable receiver",
                Severity::Error)
            .AddSpan (mc->Name ().Start, mc->Name ().End);
        return;
    }
}

}
