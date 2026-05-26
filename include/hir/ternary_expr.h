#pragma once
#include <basic/types/type.h>
#include <hir/basic_block.h>
#include <hir/node.h>

namespace veo::hir {

class TernaryExpr : public Node {
    basic::Type *_type;
    Node        *_trueVal;
    BasicBlock  *_trueBB;
    Node        *_falseVal;
    BasicBlock  *_falseBB;

public:
    TernaryExpr (
        basic::Type *type,
        Node        *trueVal,
        BasicBlock  *trueBB,
        Node        *falseVal,
        BasicBlock  *falseBB,
        llvm::SMLoc  start,
        llvm::SMLoc  end)
        : _type (type),
          _trueVal (trueVal),
          _trueBB (trueBB),
          _falseVal (falseVal),
          _falseBB (falseBB),
          Node (NodeKind::TernaryExpr, start, end) {}

    hir_classof (TernaryExpr);

    basic::Type *
    Type () const {
        return _type;
    }

    Node *
    TrueVal () const {
        return _trueVal;
    }

    BasicBlock *
    TrueBB () const {
        return _trueBB;
    }

    Node *
    FalseVal () const {
        return _falseVal;
    }

    BasicBlock *
    FalseBB () const {
        return _falseBB;
    }
};

}
