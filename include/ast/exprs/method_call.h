#pragma once
#include <ast/expr.h>
#include <basic/name.h>
#include <vector>

namespace veo::ast {

class MethodCall : public Expr {
    Expr               *_base;
    basic::NameObj      _name;
    std::vector<Expr *> _args;

public:
    MethodCall (
        Expr               *base,
        basic::NameObj      name,
        std::vector<Expr *> args,
        llvm::SMLoc         start,
        llvm::SMLoc         end)
        : _base (base),
          _name (std::move (name)),
          _args (std::move (args)),
          Expr (NodeKind::MethodCall, start, end) {}

    ast_classof (MethodCall);

    Expr *
    Base () const {
        return _base;
    }

    basic::NameObj
    Name () const {
        return _name;
    }

    std::vector<Expr *> &
    Args () {
        return _args;
    }
};

}
