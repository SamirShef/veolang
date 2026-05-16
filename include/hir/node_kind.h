#pragma once
#include <cstdint>

namespace veo::hir {

enum class NodeKind : uint8_t {
    VarDef,
    LoadVar,
    Func,
    FuncCall,
    Ret,
    LitExpr,
    BinExpr,
    UnExpr,
    ExprStmt
};

}
