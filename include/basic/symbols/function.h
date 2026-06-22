#pragma once
#include <ast/stmts/func_def.h>
#include <basic/name.h>
#include <basic/types/type.h>
#include <hir/mangle_kind.h>
#include <vector>

namespace veo::symbols {

struct Module;

struct Function {
    basic::NameObj             Name;
    basic::Type               *RetType;
    std::vector<ast::Argument> Args;
    bool                       IsGeneric;
    Module                    *Parent;
    ast::AccessModifier        Access;
    hir::MangleKind            MangleKind;

    Function (
        basic::NameObj             name,
        basic::Type               *retType,
        std::vector<ast::Argument> args,
        bool                       isGeneric,
        Module                    *parent,
        ast::AccessModifier        access,
        hir::MangleKind            mangleKind = hir::MangleKind::Veo)
        : Name (std::move (name)),
          RetType (retType),
          Args (std::move (args)),
          IsGeneric (isGeneric),
          Parent (parent),
          Access (access),
          MangleKind (mangleKind) {}

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
        if (this == &other) {
            return true;
        }

        return Candidates == other.Candidates;
    }

    bool
    operator!= (const FunctionCandidates &other) const {
        return !(*this == other);
    }
};

}
