#pragma once
#include <hir/func.h>
#include <hir/node.h>
#include <vector>

namespace veo::hir {

class FuncCall : public Node {
    Function           *_func;
    std::vector<Node *> _args;

public:
    FuncCall (
        Function *func, std::vector<Node *> args, llvm::SMLoc start, llvm::SMLoc end)
        : _func (func), _args (std::move (args)), Node (NodeKind::FuncCall, start, end) {}

    hir_classof (FuncCall);

    Function *
    GetFunction () const {
        return _func;
    }

    std::vector<Node *> &
    Args () {
        return _args;
    }
};

}
