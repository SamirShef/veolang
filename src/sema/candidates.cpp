#include <sema/sema.h>

namespace veo {

using namespace symbols;

Function *
Sema::resolveBestOverload (
    FunctionCandidates        *candidates,
    const std::vector<Type *> &argTypes,
    llvm::SMLoc                start,
    llvm::SMLoc                end) {
    std::vector<std::pair<Function *, int>> viableCandidates;

    for (auto &cand : candidates->Candidates) {
        if (cand->Args.size () != argTypes.size ()) {
            continue;
        }

        size_t costSum = 0;
        if (viableFuncCandidate (cand.get (), argTypes, costSum)) {
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
    const std::vector<Type *> &argTypes,
    llvm::SMLoc                start,
    llvm::SMLoc                end) {
    std::vector<std::pair<symbols::Method *, int>> viableCandidates;

    for (auto &cand : candidates->Candidates) {
        if (cand->Func->Args.size () != argTypes.size ()) {
            continue;
        }

        size_t costSum = 0;
        if (viableFuncCandidate (cand->Func.get (), argTypes, costSum)) {
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
    oss << "func " << func->Name.Val << "(";
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

}
