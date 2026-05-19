#pragma once
#include <ast/access_modifier.h>
#include <basic/name.h>
#include <basic/types/type.h>
#include <vector>

namespace veo::symbols {

struct Module;

struct Field {
    basic::NameObj      Name;
    basic::Type        *Type;
    bool                IsStatic;
    bool                IsConst;
    ast::AccessModifier Access;

    Field (
        basic::NameObj      name,
        basic::Type        *type,
        bool                isStatic,
        bool                isConst,
        ast::AccessModifier access)
        : Name (std::move (name)),
          Type (type),
          IsStatic (isStatic),
          IsConst (isConst),
          Access (access) {}
};

struct Struct {
    basic::NameObj     Name;
    std::vector<Field> Fields;
    Module            *Parent;

    Struct (basic::NameObj name, std::vector<Field> fields, Module *parent)
        : Name (std::move (name)), Fields (std::move (fields)), Parent (parent) {}
};

}
