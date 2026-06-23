#pragma once
#include <basic/types/type.h>
#include <hir/node.h>

namespace veo::hir {

enum class CastKind : uint8_t {
    UIToFP,
    SIToFP,
    FPToUI,
    FPToSI,
    SExt,
    ZExt,
    FPTrunc,
    Trunc,
    BitCast,
    FPExt,
    Invalid
};

class Cast : public Node {
    CastKind     _kind;
    basic::Type *_type;
    Node        *_expr;

public:
    Cast (
        CastKind kind, basic::Type *type, Node *expr, llvm::SMLoc start, llvm::SMLoc end)
        : _kind (kind), _type (type), _expr (expr), Node (NodeKind::Cast, start, end) {}

    hir_classof (Cast);

    CastKind
    GetCastKind () const {
        return _kind;
    }

    basic::Type *
    Type () const {
        return _type;
    }

    Node *
    Expr () const {
        return _expr;
    }
};

}
