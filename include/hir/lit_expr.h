#pragma once
#include <basic/value.h>
#include <hir/node.h>

namespace veo::hir {

class LiteralExpr : public Node {
    basic::Value _val;

public:
    LiteralExpr (basic::Value val, llvm::SMLoc start, llvm::SMLoc end)
        : _val (val), Node (NodeKind::LitExpr, start, end) {}

    hir_classof (LitExpr);

    basic::Value
    Value () const {
        return _val;
    }
};

}
