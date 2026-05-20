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
    size_t              Index;

    Field (
        basic::NameObj      name,
        basic::Type        *type,
        bool                isStatic,
        bool                isConst,
        ast::AccessModifier access,
        size_t              index)
        : Name (std::move (name)),
          Type (type),
          IsStatic (isStatic),
          IsConst (isConst),
          Access (access),
          Index (index) {}

    bool
    operator== (const Field &other) const {
        return Name.Val == other.Name.Val && *Type == *other.Type
               && IsStatic == other.IsStatic && IsConst == other.IsConst
               && Access == other.Access && Index == other.Index;
    }

    bool
    operator!= (const Field &other) const {
        return !(*this == other);
    }
};

struct Struct {
    basic::NameObj     Name;
    std::vector<Field> Fields;
    Module            *Parent;

    Struct (basic::NameObj name, std::vector<Field> fields, Module *parent)
        : Name (std::move (name)), Fields (std::move (fields)), Parent (parent) {}

    bool
    operator== (const Struct &other) const;

    bool
    operator!= (const Struct &other) const {
        return !(*this == other);
    }
};

}
