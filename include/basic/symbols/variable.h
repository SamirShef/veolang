#pragma once
#include <basic/name.h>
#include <basic/types/type.h>
#include <basic/value.h>

namespace veo::hir {

class VarDef;

}

namespace veo::symbols {

struct Module;

struct Variable {
    basic::NameObj  Name;
    basic::Type    *Type;
    basic::OptValue Val;
    bool            IsConst;
    bool            IsGlobal;
    hir::VarDef    *HIR = nullptr;
    Module         *Parent;

    Variable (
        basic::NameObj  name,
        basic::Type    *type,
        bool            isConst,
        bool            isGlobal,
        Module         *parent,
        basic::OptValue val = std::nullopt,
        hir::VarDef    *hir = nullptr)
        : Name (std::move (name)),
          Type (type),
          IsConst (isConst),
          IsGlobal (isGlobal),
          Val (val),
          HIR (hir),
          Parent (parent) {}

    bool
    operator== (const Variable &other) const;

    bool
    operator!= (const Variable &other) const {
        return !(*this == other);
    }
};

}
