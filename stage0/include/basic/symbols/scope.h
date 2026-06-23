#pragma once
#include <basic/symbols/variable.h>
#include <unordered_map>

namespace veo::symbols {

struct Scope {
    std::unordered_map<std::string, Variable> Vars;
};

}
