#pragma once
#include <cstdint>
#include <llvm/Support/Casting.h>

namespace veo::basic {

class IntegerType;
class FloatingType;
class BoolType;
class CharType;

#define type_classof(kind)                                                               \
    static bool classof (const veo::basic::Type *type) {                                 \
        return type->Kind () == (veo::basic::TypeKind::kind);                            \
    }

enum class TypeKind : uint8_t { Integer, Floating, Bool, Char };

class Type {
    TypeKind _kind;

public:
    explicit Type (TypeKind kind) : _kind (kind) {}
    virtual ~Type ()    = default;
    Type (const Type &) = default;
    Type (Type &&)      = delete;

    Type &
    operator= (const Type &) = default;

    Type &
    operator= (Type &&) = delete;

    bool
    operator== (const Type &other);

    TypeKind
    Kind () const {
        return _kind;
    }

    virtual std::string
    ToString () const = 0;

#define is(kind)                                                                         \
    bool Is##kind () const { return _kind == TypeKind::kind; }
#define as(kind) const kind##Type *As##kind () const;

    is (Integer);
    as (Integer);
    is (Floating);
    as (Floating);
    is (Bool);
    as (Bool);
    is (Char);
    as (Char);

#undef as
#undef is
};

}
