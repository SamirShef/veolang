#pragma once
#include <ast/expr.h>

namespace veo::ast {

class NilExpr : public Expr {
public:
    NilExpr (llvm::SMLoc start, llvm::SMLoc end) : Expr (NodeKind::NilExpr, start, end) {}

    ast_classof (NilExpr);
};

}
