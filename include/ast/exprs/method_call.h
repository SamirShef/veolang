#pragma once
#include <ast/expr.h>
#include <basic/name.h>
#include <basic/types/type.h>
#include <vector>

namespace veo::ast {

class MethodCall : public Expr {
    Expr                      *_base;
    basic::NameObj             _name;
    std::vector<Expr *>        _args;
    std::vector<basic::Type *> _genericParams;

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

    MethodCall (
        Expr                      *base,
        basic::NameObj             name,
        std::vector<Expr *>        args,
        std::vector<basic::Type *> genericParams,
        llvm::SMLoc                start,
        llvm::SMLoc                end)
        : _base (base),
          _name (std::move (name)),
          _args (std::move (args)),
          _genericParams (std::move (genericParams)),
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

    std::vector<basic::Type *> &
    GenericParams () {
        return _genericParams;
    }
};

}
