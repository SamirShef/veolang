#pragma once
#include <ast/stmts/func_def.h>
#include <basic/name.h>
#include <basic/types/type.h>
#include <vector>

namespace veo::symbols {

struct Module;

struct Function {
    basic::NameObj             Name;
    basic::Type               *RetType;
    std::vector<ast::Argument> Args;
    Module                    *Parent;

    Function (
        basic::NameObj             name,
        basic::Type               *retType,
        std::vector<ast::Argument> args,
        Module                    *parent)
        : Name (std::move (name)),
          RetType (retType),
          Args (std::move (args)),
          Parent (parent) {}

    bool
    operator== (const Function &other) const;

    bool
    operator!= (const Function &other) const {
        return !(*this == other);
    }
};

struct FunctionCandidates {
    std::vector<std::unique_ptr<Function>> Candidates;

    bool
    operator== (const FunctionCandidates &other) const {
        return Candidates == other.Candidates;
    }

    bool
    operator!= (const FunctionCandidates &other) const {
        return !(*this == other);
    }
};

}
