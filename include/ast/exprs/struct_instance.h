#pragma once
#include <ast/expr.h>
#include <basic/name.h>
#include <basic/types/type.h>
#include <tuple>
#include <vector>

namespace veo::ast {

class StructInstance : public Expr {
    basic::Type                                    *_structType;
    std::vector<std::tuple<basic::NameObj, Expr *>> _fields;

public:
    StructInstance (
        basic::Type                                    *structType,
        std::vector<std::tuple<basic::NameObj, Expr *>> fields,
        llvm::SMLoc                                     start,
        llvm::SMLoc                                     end)
        : _structType (structType),
          _fields (std::move (fields)),
          Expr (NodeKind::StructInstance, start, end) {}

    ast_classof (StructInstance);

    basic::Type *&
    StructType () {
        return _structType;
    }

    std::vector<std::tuple<basic::NameObj, Expr *>> &
    Fields () {
        return _fields;
    }
};

}
