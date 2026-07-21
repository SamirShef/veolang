#pragma once
#include <ast/expr.h>
#include <ast/stmt.h>
#include <vector>

namespace veo::ast {

class IfElseStmt : public Stmt {
    Expr               *_cond;
    std::vector<Stmt *> _then;
    std::vector<Stmt *> _else;

public:
    IfElseStmt (
        Expr               *cond,
        std::vector<Stmt *> thenBranch,
        std::vector<Stmt *> elseBranch,
        llvm::SMLoc         start,
        llvm::SMLoc         end)
        : _cond (cond),
          _then (std::move (thenBranch)),
          _else (std::move (elseBranch)),
          Stmt (NodeKind::IfElse, start, end) {}

    ast_classof (IfElse);

    Expr *
    Cond () const {
        return _cond;
    }

    std::vector<Stmt *> &
    Then () {
        return _then;
    }

    std::vector<Stmt *> &
    Else () {
        return _else;
    }
};

}
