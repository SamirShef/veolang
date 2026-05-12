#pragma once
#include <ast/stmts/func_def.h>
#include <basic/name.h>
#include <basic/types/type.h>
#include <vector>

namespace veo::symbols {

struct Function {
    basic::NameObj             Name;
    basic::Type               *RetType;
    std::vector<ast::Argument> Args;

    Function (basic::NameObj name, basic::Type *retType, std::vector<ast::Argument> args)
        : Name (std::move (name)), RetType (retType), Args (std::move (args)) {}
};

struct FunctionCandidates {
    std::vector<Function> Candidates;
};

}
