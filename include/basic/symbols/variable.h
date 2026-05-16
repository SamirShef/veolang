#pragma once
#include <basic/name.h>
#include <basic/types/type.h>
#include <basic/value.h>

namespace veo::symbols {

struct Module;

struct Variable {
    basic::NameObj  Name;
    basic::Type    *Type;
    basic::OptValue Val;
    bool            IsConst;
    bool            IsGlobal;
    size_t          Index;
    Module         *Parent;

    Variable (
        basic::NameObj  name,
        basic::Type    *type,
        bool            isConst,
        bool            isGlobal,
        size_t          index,
        basic::OptValue val    = std::nullopt,
        Module         *parent = nullptr)
        : Name (std::move (name)),
          Type (type),
          IsConst (isConst),
          IsGlobal (isGlobal),
          Index (index),
          Val (val),
          Parent (parent) {}
};

}
