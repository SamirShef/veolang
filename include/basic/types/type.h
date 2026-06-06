#pragma once
#include <cstdint>
#include <llvm/Support/Casting.h>

namespace veo::basic {

class IntegerType;
class SizeType;
class FloatingType;
class BoolType;
class CharType;
class StructType;
class NamedType;
class PointerType;
class NothType;

#define type_classof(kind)                                                               \
    static bool classof (const veo::basic::Type *type) {                                 \
        return type->Kind () == (veo::basic::TypeKind::kind);                            \
    }

enum class TypeKind : uint8_t {
    Integer,
    Size,
    Floating,
    Bool,
    Char,
    Struct,
    Named,
    Pointer,
    Noth
};

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
    is (Size);
    as (Size);
    is (Floating);
    as (Floating);
    is (Bool);
    as (Bool);
    is (Char);
    as (Char);
    is (Struct);
    as (Struct);
    is (Named);
    as (Named);
    is (Pointer);
    as (Pointer);
    is (Noth);
    as (Noth);

    bool
    IsIntOrSize () const {
        return IsInteger () || IsSize ();
    }

    bool
    IsNumber () const {
        return IsInteger () || IsFloating () || IsSize ();
    }

#undef as
#undef is
};

bool
operator== (const Type &lhs, const Type &rhs);

}
