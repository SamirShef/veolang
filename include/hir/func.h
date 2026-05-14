#pragma once
#include <ast/stmts/func_def.h>
#include <basic/name.h>
#include <basic/symbols/function.h>
#include <basic/types/type.h>
#include <hir/basic_block.h>
#include <hir/node.h>

namespace veo::hir {

class Function : public Node {
    basic::NameObj             _name;
    basic::Type               *_retType;
    std::vector<ast::Argument> _args;
    std::vector<BasicBlock *>  _body;
    symbols::Function         *_base;

public:
    Function (
        basic::NameObj             name,
        basic::Type               *retType,
        std::vector<ast::Argument> args,
        llvm::SMLoc                start,
        llvm::SMLoc                end,
        symbols::Function         *base)
        : _name (std::move (name)),
          _retType (retType),
          _args (std::move (args)),
          _base (base),
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

    std::vector<ast::Argument> &
    Args () {
        return _args;
    }

    std::vector<BasicBlock *> &
    Body () {
        return _body;
    }

    symbols::Function *
    BaseSymbol () const {
        return _base;
    }
};

}
