#pragma once
#include <ast/expr.h>
#include <ast/stmt.h>
#include <vector>

namespace veo::ast {

class ForLoopStmt : public Stmt {
    Expr               *_cond;
    Stmt               *_indexator;
    Stmt               *_iteration;
    std::vector<Stmt *> _body;

public:
    ForLoopStmt (
        Expr               *cond,
        Stmt               *indexator,
        Stmt               *iteration,
        std::vector<Stmt *> body,
        llvm::SMLoc         start,
        llvm::SMLoc         end)
        : _cond (cond),
          _indexator (indexator),
          _iteration (iteration),
          _body (std::move (body)),
          Stmt (NodeKind::ForLoop, start, end) {}

    ast_classof (ForLoop);

    Expr *
    Cond () const {
        return _cond;
    }

    Stmt *
    Indexator () const {
        return _indexator;
    }

    Stmt *
    Iteration () const {
        return _iteration;
    }

    std::vector<Stmt *> &
    Body () {
        return _body;
    }
};

}
