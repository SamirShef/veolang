#pragma once
#include <ast/node.h>

namespace veo::ast {

class Expr : public Node {
public:
    Expr (NodeKind kind, llvm::SMLoc start, llvm::SMLoc end) : Node (kind, start, end) {}

    ast_classof_with_range (ExprStart, ExprEnd);
};

}
