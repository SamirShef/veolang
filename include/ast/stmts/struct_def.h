#pragma once
#include <ast/stmt.h>
#include <basic/name.h>
#include <basic/types/type.h>
#include <vector>

namespace veo::ast {

struct Field {
    basic::NameObj Name;
    basic::Type   *Type;
    bool           IsStatic;
    bool           IsConst;
    AccessModifier Access;

    Field (
        basic::NameObj name,
        basic::Type   *type,
        bool           isStatic,
        bool           isConst,
        AccessModifier access)
        : Name (std::move (name)),
          Type (type),
          IsStatic (isStatic),
          IsConst (isConst),
          Access (access) {}

    bool
    operator== (const Field &other) const {
        return Name == other.Name && *Type == *other.Type && IsStatic == other.IsStatic
               && IsConst == other.IsConst && Access == other.Access;
    }

    bool
    IsValid () const {
        return !Name.Val.empty () && Type != nullptr;
    }

    static Field
    Invalid () {
        return { basic::NameObj (), nullptr, false, false, AccessModifier::Priv };
    }
};

class StructDef : public Stmt {
    basic::NameObj     _name;
    std::vector<Field> _fields;

public:
    StructDef (
        basic::NameObj     name,
        std::vector<Field> fields,
        AccessModifier     access,
        llvm::SMLoc        start,
        llvm::SMLoc        end)
        : _name (std::move (name)),
          _fields (std::move (fields)),
          Stmt (access, NodeKind::StructDef, start, end) {}

    ast_classof (StructDef);

    basic::NameObj
    Name () const {
        return _name;
    }

    std::vector<Field> &
    Fields () {
        return _fields;
    }
};

}
