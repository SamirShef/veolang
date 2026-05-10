#pragma once
#include <hir/node_kind.h>
#include <llvm/Support/SMLoc.h>

namespace veo::hir {

#define hir_classof(kind)                                                                \
    static bool classof (const veo::hir::Node *node) {                                   \
        return node->Kind () == (veo::hir::NodeKind::kind);                              \
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

    bool
    IsTerminator () const {
        return Kind () == NodeKind::Ret; // TODO: add 'br' inst
    }
};

}
