#pragma once
#include <ast/access_modifier.h>
#include <basic/name.h>
#include <basic/symbols/function.h>
#include <basic/symbols/trait.h>
#include <basic/types/type.h>
#include <hir/mangle_kind.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace veo::symbols {

struct Module;
struct Trait;

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
        if (this == &other) {
            return true;
        }

        return Name.Val == other.Name.Val && *Type == *other.Type
               && IsStatic == other.IsStatic && IsConst == other.IsConst
               && Access == other.Access && Index == other.Index;
    }

    bool
    operator!= (const Field &other) const {
        return !(*this == other);
    }
};

struct Method {
    std::unique_ptr<Function> Func;
    ast::AccessModifier       Access;
    bool                      IsStatic;
    bool                      IsGeneric;
    bool                      IsConst = true;

    Method (
        std::unique_ptr<Function> func,
        ast::AccessModifier       access,
        bool                      isStatic,
        bool                      isGeneric)
        : Func (std::move (func)),
          Access (access),
          IsStatic (isStatic),
          IsGeneric (isGeneric) {}

    bool
    operator== (const Method &other) const {
        if (this == &other) {
            return true;
        }

        return *Func == *other.Func && Access == other.Access
               && IsStatic == other.IsStatic && IsGeneric == other.IsGeneric
               && IsConst == other.IsConst;
    }

    bool
    operator!= (const Method &other) const {
        return !(*this == other);
    }
};

struct MethodCandidates {
    std::vector<std::unique_ptr<Method>> Candidates;

    bool
    operator== (const MethodCandidates &other) const {
        if (this == &other) {
            return true;
        }

        return Candidates == other.Candidates;
    }

    bool
    operator!= (const MethodCandidates &other) const {
        return !(*this == other);
    }
};

struct Struct {
    basic::NameObj                                    Name;
    std::vector<Field>                                Fields;
    std::unordered_map<std::string, MethodCandidates> Methods;
    std::unordered_set<Trait *>                       TraitsImplements;
    Module                                           *Parent;
    hir::MangleKind                                   MangleKind;

    Struct (
        basic::NameObj     name,
        std::vector<Field> fields,
        Module            *parent,
        hir::MangleKind    mangleKind = hir::MangleKind::Veo)
        : Name (std::move (name)),
          Fields (std::move (fields)),
          Parent (parent),
          MangleKind (mangleKind) {}

    bool
    operator== (const Struct &other) const;

    bool
    operator!= (const Struct &other) const {
        return !(*this == other);
    }
};

}
