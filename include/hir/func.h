#pragma once
#include "hir/node.h"

#include <basic/name.h>
#include <basic/types/type.h>
#include <hir/basic_block.h>

namespace veo::hir {

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

class Function : public Node {
    basic::NameObj            _name;
    basic::Type              *_retType;
    std::vector<Argument>     _args;
    std::vector<BasicBlock *> _body;

public:
    Function (
        basic::NameObj        name,
        basic::Type          *retType,
        std::vector<Argument> args,
        llvm::SMLoc           start,
        llvm::SMLoc           end)
        : _name (std::move (name)),
          _retType (retType),
          _args (std::move (args)),
          Node (NodeKind::Func, start, end) {}

    hir_classof (Func);

    basic::NameObj
    Name () const {
        return _name;
    }

    basic::Type *
    RetType () const {
        return _retType;
    }

    std::vector<Argument> &
    Args () {
        return _args;
    }

    std::vector<BasicBlock *> &
    Body () {
        return _body;
    }
};

}
