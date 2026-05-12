#pragma once
#include <basic/symbols/function.h>
#include <basic/symbols/variable.h>
#include <unordered_map>

namespace veo::symbols {

struct Module {
    std::string                                         Name;
    Module                                             *Parent;
    std::unordered_map<std::string, Variable>           Vars;
    std::unordered_map<std::string, FunctionCandidates> Funcs;
    std::unordered_map<std::string, Module *>           Imports;
    std::unordered_map<std::string, Module *>           Submods;

    explicit Module (std::string name, Module *parent = nullptr)
        : Name (std::move (name)), Parent (parent) {}
};

}
