#include <basic/types/struct.h>
#include <basic/types/trait.h>
#include <ranges>
#include <sema/sema.h>

namespace veo {

using namespace symbols;

Variable *
Sema::getVariable (const std::string &name) {
    for (auto &var : std::views::reverse (_vars)) {
        if (auto varIt = var.Vars.find (name); varIt != var.Vars.end ()) {
            return &varIt->second;
        }
    }
    if (auto it = _mod->Vars.find (name); it != _mod->Vars.end ()) {
        return &it->second;
    }
    return nullptr;
}

Struct *
Sema::getStruct (const std::string &name, Module *mod) {
    if (mod == nullptr) {
        mod = _mod;
    }

    if (auto it = mod->Structs.find (name); it != mod->Structs.end ()) {
        return &it->second;
    }
    return nullptr;
}

std::optional<Function>
Sema::getFunction (const std::string &name, const std::vector<ast::Argument> &args) {
    if (!_mod->Funcs.contains (name)) {
        return std::nullopt;
    }
    const auto &candidates = _mod->Funcs.at (name);
    for (const auto &f : candidates.Candidates) {
        unsigned coincidences = 0;
        if (f->Args.size () == args.size ()) {
            for (size_t i = 0; i < args.size (); ++i) {
                if (*f->Args[i].Type == *args[i].Type) {
                    ++coincidences;
                }
            }
        } else {
            continue;
        }

        if (coincidences == args.size ()) {
            return *f;
        }
    }
    return std::nullopt;
}

Method *
Sema::getMethod (
    basic::Type *base, const std::string &name, const std::vector<ast::Argument> &args) {
    std::unordered_map<std::string, MethodCandidates> *methods = nullptr;
    if (base->IsStruct ()) {
        auto *sym = base->AsStruct ()->BaseSymbol ();
        methods   = &sym->Methods;
        if (!methods->contains (name)) {
            return nullptr;
        }
    } else if (base->IsTrait ()) {
        auto *sym = base->AsTrait ()->BaseSymbol ();
        methods   = &sym->Methods;
        if (!methods->contains (name)) {
            return nullptr;
        }
    } else {
        methods = &_mod->PrimitiveMethods[base];
        if (!methods->contains (name)) {
            return nullptr;
        }
    }
    const auto &candidates = methods->at (name);
    for (const auto &f : candidates.Candidates) {
        unsigned coincidences = 0;
        if (f->Func->Args.size () == args.size ()) {
            for (size_t i = 0; i < args.size (); ++i) {
                if (*f->Func->Args[i].Type == *args[i].Type) {
                    ++coincidences;
                }
            }
        } else {
            continue;
        }

        if (coincidences == args.size ()) {
            return f.get ();
        }
    }
    return nullptr;
}

}
