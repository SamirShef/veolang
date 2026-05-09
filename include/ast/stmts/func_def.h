#pragma once
#include <ast/stmt.h>
#include <basic/name.h>
#include <basic/types/type.h>
#include <utility>
#include <vector>

namespace veo::ast {

struct Argument {
    basic::NameObj Name;
    basic::Type   *Type;

    Argument (basic::NameObj name, basic::Type *type)
        : Name (std::move (name)), Type (type) {}

    bool
    IsValid () const {
        return !Name.Val.empty () && Type != nullptr;
    }

    static Argument
    Invalid () {
        return { basic::NameObj (), nullptr };
    }
};

class FuncDef : public Stmt {
    basic::NameObj        _name;
    basic::Type          *_retType;
    std::vector<Argument> _args;
    std::vector<Stmt *>   _body;

public:
    FuncDef (
        basic::NameObj        name,
        basic::Type          *retType,
        std::vector<Argument> args,
        std::vector<Stmt *>   body,
        AccessModifier        access,
        llvm::SMLoc           start,
        llvm::SMLoc           end)
        : _name (std::move (name)),
          _retType (retType),
          _args (std::move (args)),
          _body (std::move (body)),
          Stmt (access, NodeKind::FuncDef, start, end) {}

    basic::NameObj
    Name () const {
        return _name;
    }

    basic::Type *
    RetType () const {
        return _retType;
    }

    const std::vector<Argument> &
    Args () {
        return _args;
    }

    const std::vector<Stmt *> &
    Body () {
        return _body;
    }
};

}
