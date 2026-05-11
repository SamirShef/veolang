#pragma once
#include <basic/name.h>
#include <basic/types/type.h>
#include <basic/value.h>

namespace veo::symbols {

struct Variable {
    basic::NameObj  Name;
    basic::Type    *Type;
    basic::OptValue Val;
    bool            IsConst;
    size_t          Index;

    Variable (
        basic::NameObj  name,
        basic::Type    *type,
        bool            isConst,
        size_t          index,
        basic::OptValue val = std::nullopt)
        : Name (std::move (name)),
          Type (type),
          IsConst (isConst),
          Index (index),
          Val (val) {}
};

}
