#include <basic/types/generic.h>
#include <basic/types/pointer.h>
#include <sema/sema.h>

namespace veo {

using namespace symbols;

Function *
Sema::resolveBestOverload (
    FunctionCandidates        *candidates,
    const std::vector<Type *> &explicitGenericArgs,
    const std::vector<Type *> &argTypes,
    llvm::SMLoc                start,
    llvm::SMLoc                end) {
    std::vector<std::pair<Function *, int>> viableCandidates;

    for (auto &cand : candidates->Candidates) {
        if (cand->Args.size () != argTypes.size ()) {
            continue;
        }

        ast::FuncDef *funcDef = nullptr;
        if (auto it = _genericFuncs.find (cand.get ()); it != _genericFuncs.end ()) {
            funcDef = it->second;
        }

        size_t costSum = 0;
        if (viableFuncCandidate (
                cand.get (),
                funcDef,
                explicitGenericArgs,
                argTypes,
                costSum)) {
            viableCandidates.emplace_back (cand.get (), costSum);
        }
    }

    if (viableCandidates.empty ()) {
        std::vector<std::string> foundCandidates;
        candidatesToStringVector (candidates, foundCandidates);
        _diag
            .Report (
                DiagCode::ENoMatchingFunction,
                "no matching function for call to '" + candidates->Candidates[0]->Name.Val
                    + "'",
                Severity::Error)
            .AddSpan (start, end)
            .AddNote ("candidate functions found:", std::move (foundCandidates));
        return nullptr;
    }

    Function *bestCand    = viableCandidates[0].first;
    size_t    minCost     = viableCandidates[0].second;
    bool      isAmbiguous = false;

    for (size_t i = 1; i < viableCandidates.size (); ++i) {
        if (viableCandidates[i].second < minCost) {
            minCost     = viableCandidates[i].second;
            bestCand    = viableCandidates[i].first;
            isAmbiguous = false;
        } else if (viableCandidates[i].second == minCost) {
            isAmbiguous = true;
        }
    }

    if (isAmbiguous) {
        auto  &err = _diag
                         .Report (
                             DiagCode::ECallIsAmbiguous,
                             "call to '" + bestCand->Name.Val + "' is ambiguous",
                             Severity::Error)
                         .AddSpan (start, end, "ambiguously call");
        size_t i   = 1;
        for (const auto &[c, _] : viableCandidates) {
            err.AddSpan (
                c->Name.Start,
                c->Name.End,
                "candidate " + std::to_string (i),
                false);
            ++i;
        }
        return nullptr;
    }

    return bestCand;
}

symbols::Method *
Sema::resolveBestOverload (
    symbols::MethodCandidates *candidates,
    const std::vector<Type *> &explicitGenericArgs,
    const std::vector<Type *> &argTypes,
    llvm::SMLoc                start,
    llvm::SMLoc                end) {
    std::vector<std::pair<symbols::Method *, int>> viableCandidates;

    for (auto &cand : candidates->Candidates) {
        if (cand->Func->Args.size () != argTypes.size ()) {
            continue;
        }

        ast::FuncDef *funcDef = nullptr;
        if (auto it = _genericMethods.find (cand.get ()); it != _genericMethods.end ()) {
            funcDef = it->second;
        }

        size_t costSum = 0;
        if (viableFuncCandidate (
                cand->Func.get (),
                funcDef,
                explicitGenericArgs,
                argTypes,
                costSum)) {
            viableCandidates.emplace_back (cand.get (), costSum);
        }
    }

    if (viableCandidates.empty ()) {
        std::vector<std::string> foundCandidates;
        candidatesToStringVector (candidates, foundCandidates);
        _diag
            .Report (
                DiagCode::ENoMatchingFunction,
                "no matching function for call to '"
                    + candidates->Candidates[0]->Func->Name.Val + "'",
                Severity::Error)
            .AddSpan (start, end)
            .AddNote ("candidate functions found:", std::move (foundCandidates));
        return nullptr;
    }

    symbols::Method *bestCand    = viableCandidates[0].first;
    size_t           minCost     = viableCandidates[0].second;
    bool             isAmbiguous = false;

    for (size_t i = 1; i < viableCandidates.size (); ++i) {
        if (viableCandidates[i].second < minCost) {
            minCost     = viableCandidates[i].second;
            bestCand    = viableCandidates[i].first;
            isAmbiguous = false;
        } else if (viableCandidates[i].second == minCost) {
            isAmbiguous = true;
        }
    }

    if (isAmbiguous) {
        auto  &err = _diag
                         .Report (
                             DiagCode::ECallIsAmbiguous,
                             "call to '" + bestCand->Func->Name.Val + "' is ambiguous",
                             Severity::Error)
                         .AddSpan (start, end, "ambiguously call");
        size_t i   = 1;
        for (const auto &[c, _] : viableCandidates) {
            err.AddSpan (
                c->Func->Name.Start,
                c->Func->Name.End,
                "candidate " + std::to_string (i),
                false);
            ++i;
        }
        return nullptr;
    }

    return bestCand;
}

void
Sema::candidatesToStringVector (
    symbols::FunctionCandidates *candidates, std::vector<std::string> &res) {
    for (auto &func : candidates->Candidates) {
        res.emplace_back (funcCandidateToString (func.get ()));
    }
}

void
Sema::candidatesToStringVector (
    symbols::MethodCandidates *candidates, std::vector<std::string> &res) {
    for (auto &method : candidates->Candidates) {
        res.emplace_back (funcCandidateToString (method->Func.get ()));
    }
}

std::string
Sema::funcCandidateToString (symbols::Function *func) {
    std::ostringstream oss;
    oss << "func " << func->Name.Val << ' ';
    if (func->FuncDef != nullptr && func->FuncDef->IsGeneric ()) {
        oss << "<";
        size_t i = 0;
        for (const auto &param : func->FuncDef->GenericParams ()) {
            if (i != 0) {
                oss << ", ";
            }
            oss << param.Name.Val;
            ++i;
        }
        oss << "> ";
    }
    oss << "(";
    size_t i = 0;
    for (const auto &a : func->Args) {
        oss << typeToString (a.Type);
        if (i < func->Args.size () - 1) {
            oss << ", ";
        }
        ++i;
    }
    oss << "): " << typeToString (func->RetType);
    return oss.str ();
}

bool
Sema::viableFuncCandidate (
    symbols::Function         *func,
    ast::FuncDef              *funcDef,
    const std::vector<Type *> &explicitGenericArgs,
    const std::vector<Type *> &args,
    size_t                    &costSum) {
    bool isGeneric = funcDef != nullptr && funcDef->IsGeneric ();
    if (!isGeneric && !explicitGenericArgs.empty ()) {
        // TODO: report error
        return false;
    }

    std::unordered_map<std::string, Type *> substMap;

    if (isGeneric) {
        const auto &genericParams = funcDef->GenericParams ();

        if (!explicitGenericArgs.empty ()) {
            if (explicitGenericArgs.size () != genericParams.size ()) {
                // TODO: report error
                return false;
            }
            for (size_t i = 0; i < genericParams.size (); ++i) {
                substMap[genericParams[i].Name.Val] = explicitGenericArgs[i];
            }
        } else {
            std::unordered_map<std::string, Type *> inferredMap;
            for (size_t i = 0; i < args.size (); ++i) {
                if (!deduceGenericTypes (func->Args[i].Type, args[i], inferredMap)) {
                    return false;
                }
            }
            for (const auto &param : genericParams) {
                if (!inferredMap.contains (param.Name.Val)) {
                    return false;
                }
            }
            substMap = std::move (inferredMap);
        }
    }

    bool viable = true;
    for (size_t i = 0; i < args.size (); ++i) {
        Type *expectedType = func->Args[i].Type;

        if (isGeneric) {
            expectedType = substituteGenericTypes (expectedType, substMap);
            if (expectedType == nullptr) {
                return false;
            }
        }

        CastCost cost = checkCastCost (args[i], expectedType);
        if (cost == CastCost::Incompatible) {
            viable = false;
            break;
        }
        costSum += static_cast<size_t> (cost);
    }
    return viable;
}

bool
Sema::deduceGenericTypes (
    Type                                    *paramType,
    Type                                    *argType,
    std::unordered_map<std::string, Type *> &inferredMap) {
    if (paramType == nullptr || argType == nullptr) {
        return false;
    }

    if (auto *generic = llvm::dyn_cast<GenericType> (paramType)) {
        std::string name = generic->Name ();
        auto        it   = inferredMap.find (name);
        if (it != inferredMap.end ()) {
            return canImplicitCast (
                { Value (ValueKind::Unknown, argType), nullptr },
                &it->second);
        }
        inferredMap[name] = argType;
        return true;
    }

    if (auto *paramPtr = llvm::dyn_cast<PointerType> (paramType)) {
        if (auto *argPtr = llvm::dyn_cast<PointerType> (argType)) {
            return deduceGenericTypes (paramPtr->Base (), argPtr->Base (), inferredMap);
        }
        return false;
    }

    return true;
}

Type *
Sema::substituteGenericTypes (
    Type *type, const std::unordered_map<std::string, Type *> &substMap) {
    if (type == nullptr) {
        return nullptr;
    }

    if (auto *generic = llvm::dyn_cast<GenericType> (type)) {
        auto it = substMap.find (generic->Name ());
        if (it != substMap.end ()) {
            return it->second;
        }
        return type;
    }

    if (auto *ptr = llvm::dyn_cast<PointerType> (type)) {
        Type *baseSubst = substituteGenericTypes (ptr->Base (), substMap);
        return createType<PointerType> (baseSubst);
    }

    return type;
}

}
