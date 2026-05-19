#pragma once
#include <ast/expr.h>
#include <basic/name.h>
#include <tuple>
#include <vector>

namespace veo::ast {

class StructInstance : public Expr {
    basic::NameObj                                  _path;
    std::vector<std::tuple<basic::NameObj, Expr *>> _fields;

public:
    StructInstance (
        basic::NameObj                                  path,
        std::vector<std::tuple<basic::NameObj, Expr *>> fields,
        llvm::SMLoc                                     start,
        llvm::SMLoc                                     end)
        : _path (std::move (path)),
          _fields (std::move (fields)),
          Expr (NodeKind::StructInstance, start, end) {}

    ast_classof (StructInstance);

    basic::NameObj
    Path () const {
        return _path;
    }

    std::vector<std::tuple<basic::NameObj, Expr *>> &
    Fields () {
        return _fields;
    }
};

}
