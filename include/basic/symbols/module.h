#pragma once
#include <basic/symbols/function.h>
#include <basic/symbols/struct.h>
#include <basic/symbols/variable.h>
#include <sstream>
#include <unordered_map>

namespace veo::symbols {

struct Module {
    std::string                                         Name;
    Module                                             *Parent;
    std::unordered_map<std::string, Variable>           Vars;
    std::unordered_map<std::string, FunctionCandidates> Funcs;
    std::unordered_map<std::string, Struct>             Structs;
    std::unordered_map<std::string, Module *>           Imports;
    std::unordered_map<std::string, Module *>           Submods;

    explicit Module (std::string name, Module *parent = nullptr)
        : Name (std::move (name)), Parent (parent) {}

    bool
    operator== (const Module &other) const {
        bool isParentsEquals
            = Parent != nullptr && other.Parent != nullptr && *Parent == *other.Parent;
        return Name == other.Name && isParentsEquals && Vars == other.Vars
               && Funcs == other.Funcs && Structs == other.Structs
               && Imports == other.Imports && Submods == other.Submods;
    }

    bool
    operator!= (const Module &other) const {
        return !(*this == other);
    }

    std::string
    ToString () const {
        std::ostringstream oss;
        if (Parent != nullptr) {
            oss << Parent->ToString () << '.';
        }
        oss << Name;
        return oss.str ();
    }
};

}
