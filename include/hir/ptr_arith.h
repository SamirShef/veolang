#pragma once
#include <ast/exprs/bin_expr.h>
#include <basic/types/type.h>
#include <hir/node.h>

namespace veo::hir {

class PtrArith : public Node {
    ast::BinOp   _op;
    Node        *_ptr;
    Node        *_offset;
    basic::Type *_resType;

public:
    PtrArith (
        ast::BinOp   op,
        Node        *ptr,
        Node        *offset,
        basic::Type *resType,
        llvm::SMLoc  start,
        llvm::SMLoc  end)
        : Node (NodeKind::PtrArith, start, end),
          _op (op),
          _ptr (ptr),
          _offset (offset),
          _resType (resType) {}

    hir_classof (PtrArith);

    ast::BinOp
    Op () const {
        return _op;
    }

    Node *
    Ptr () const {
        return _ptr;
    }

    Node *
    Offset () const {
        return _offset;
    }

    basic::Type *
    Type () const {
        return _resType;
    }
};

}
