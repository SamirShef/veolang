#pragma once
#include <ast/node_kind.h>
#include <llvm/Support/SMLoc.h>

namespace veo::ast {

#define ast_classof_with_range(kind1, kind2)                                             \
    static bool classof (const veo::ast::Node *node) {                                   \
        return node->Kind () >= (veo::ast::NodeKind::kind1)                              \
               && node->Kind () <= (veo::ast::NodeKind::kind2);                          \
    }
#define ast_classof(kind)                                                                \
    static bool classof (const veo::ast::Node *node) {                                   \
        return node->Kind () == (veo::ast::NodeKind::kind);                              \
    }

class Node {
    NodeKind    _kind;
    llvm::SMLoc _start;
    llvm::SMLoc _end;

public:
    Node (NodeKind kind, llvm::SMLoc start, llvm::SMLoc end)
        : _kind (kind), _start (start), _end (end) {}

    NodeKind
    Kind () const {
        return _kind;
    }

    llvm::SMLoc
    Start () const {
        return _start;
    }

    llvm::SMLoc
    End () const {
        return _end;
    }
};

}
