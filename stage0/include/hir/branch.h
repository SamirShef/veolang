#pragma once
#include <hir/basic_block.h>
#include <hir/node.h>

namespace veo::hir {

class Branch : public Node {
    Node       *_cond; // may be null
    BasicBlock *_thenBB;
    BasicBlock *_elseBB;

public:
    Branch (
        Node       *cond,
        BasicBlock *thenBB,
        BasicBlock *elseBB,
        llvm::SMLoc start,
        llvm::SMLoc end)
        : _cond (cond),
          _thenBB (thenBB),
          _elseBB (elseBB),
          Node (NodeKind::Branch, start, end) {}

    hir_classof (Branch);

    Node *
    Cond () const {
        return _cond;
    }

    BasicBlock *
    Then () const {
        return _thenBB;
    }

    BasicBlock *
    Else () const {
        return _elseBB;
    }
};

}
