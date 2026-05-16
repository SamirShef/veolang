#pragma once
#include <basic/symbols/function.h>
#include <hir/node.h>
#include <vector>

namespace veo::hir {

class FuncCall : public Node {
    symbols::Function  *_func;
    std::vector<Node *> _args;

public:
    FuncCall (
        symbols::Function  *func,
        std::vector<Node *> args,
        llvm::SMLoc         start,
        llvm::SMLoc         end)
        : _func (func), _args (std::move (args)), Node (NodeKind::FuncCall, start, end) {}

    hir_classof (FuncCall);

    symbols::Function *
    Function () const {
        return _func;
    }

    std::vector<Node *> &
    Args () {
        return _args;
    }
};

}
