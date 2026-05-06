#pragma once
#include <cstdint>

namespace veo::ast {

enum class NodeKind : uint8_t {
    StmtStart,

    StmtEnd,

    ExprStart,

    ExprEnd
};

}
