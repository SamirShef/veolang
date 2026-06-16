#pragma once
#include <ast/expr.h>
#include <basic/name.h>
#include <basic/types/type.h>
#include <vector>

namespace veo::ast {

class FuncCall : public Expr {
    basic::NameObj             _name;
    std::vector<Expr *>        _args;
    std::vector<basic::Type *> _genericParams;

public:
    FuncCall (
        basic::NameObj name, std::vector<Expr *> args, llvm::SMLoc start, llvm::SMLoc end)
        : _name (std::move (name)),
          _args (std::move (args)),
          Expr (NodeKind::FuncCall, start, end) {}

    FuncCall (
        basic::NameObj             name,
        std::vector<Expr *>        args,
        std::vector<basic::Type *> genericParams,
        llvm::SMLoc                start,
        llvm::SMLoc                end)
        : _name (std::move (name)),
          _args (std::move (args)),
          _genericParams (std::move (genericParams)),
          Expr (NodeKind::FuncCall, start, end) {}

    ast_classof (FuncCall);

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

    bool
    IsGeneric () const {
        return !_genericParams.empty ();
    }
};

}
