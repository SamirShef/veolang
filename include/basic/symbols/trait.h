#pragma once
#include <ast/access_modifier.h>
#include <basic/name.h>
#include <basic/symbols/function.h>
#include <basic/types/type.h>
#include <unordered_map>

namespace veo::symbols {

struct Module;
struct MethodCandidates;

struct Trait {
    basic::NameObj                                    Name;
    std::unordered_map<std::string, MethodCandidates> Methods;
    Module                                           *Parent;

    Trait (basic::NameObj name, Module *parent)
        : Name (std::move (name)), Parent (parent) {}

    bool
    operator== (const Trait &other) const;

    bool
    operator!= (const Trait &other) const {
        return !(*this == other);
    }
};

}
