#pragma once
#include <basic/types/type.h>
#include <variant>

namespace veo::basic {

using ValueData = std::variant<int64_t, double>;

struct Value;
using OptValue = std::optional<Value>;

enum class ValueKind : uint8_t { Unknown, Const };

struct Value {
    ValueKind   Kind;
    ValueData   Data;
    class Type *Type;

    Value (ValueKind kind, ValueData data, class Type *type)
        : Kind (kind), Data (data), Type (type) {}
    Value (ValueKind kind, class Type *type) : Kind (kind), Type (type) {}

    bool
    IsUnknown () const {
        return Kind == ValueKind::Unknown;
    }

    static OptValue
    Incorrect () {
        return std::nullopt;
    }
};

}
