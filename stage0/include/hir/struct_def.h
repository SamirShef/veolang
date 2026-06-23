#pragma once
#include <basic/name.h>
#include <basic/symbols/struct.h>
#include <basic/types/type.h>
#include <hir/mangle_kind.h>
#include <hir/node.h>
#include <vector>

namespace veo::hir {

struct Field {
    basic::NameObj Name;
    basic::Type   *Type;
    bool           IsStatic;
    bool           IsConst;

    Field (basic::NameObj name, basic::Type *type, bool isStatic, bool isConst)
        : Name (std::move (name)), Type (type), IsStatic (isStatic), IsConst (isConst) {}

    bool
    operator== (const Field &other) const {
        if (this == &other) {
            return true;
        }

        return Name.Val == other.Name.Val && *Type == *other.Type
               && IsStatic == other.IsStatic && IsConst == other.IsConst;
    }

    bool
    operator!= (const Field &other) const {
        return !(*this == other);
    }
};

class StructDef : public Node {
    basic::NameObj     _name;
    std::vector<Field> _fields;
    symbols::Struct   *_base;
    MangleKind         _mangleKind;

public:
    StructDef (
        basic::NameObj     name,
        std::vector<Field> fields,
        symbols::Struct   *base,
        llvm::SMLoc        start,
        llvm::SMLoc        end,
        MangleKind         mangleKind = MangleKind::Veo)
        : _name (std::move (name)),
          _fields (std::move (fields)),
          _base (base),
          _mangleKind (mangleKind),
          Node (NodeKind::StructDef, start, end) {}

    hir_classof (StructDef);

    basic::NameObj
    Name () const {
        return _name;
    }

    std::vector<Field> &
    Fields () {
        return _fields;
    }

    symbols::Struct *
    BaseSymbol () const {
        return _base;
    }

    MangleKind
    GetMangleKind () const {
        return _mangleKind;
    }
};

}
