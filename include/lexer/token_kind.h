#pragma once
#include <cstdint>

namespace veo {

enum class TokenKind : uint8_t {
    Id, // identifier

    Bool,  // type `bool`
    Char,  // type `char`
    I8,    // type `i8`
    I16,   // type `i16`
    I32,   // type `i32`
    I64,   // type `i64`
    ISize, // type `isize`
    U8,    // type `u8`
    U16,   // type `u16`
    U32,   // type `u32`
    U64,   // type `u64`
    USize, // type `usize`
    F32,   // type `f32`
    F64,   // type `f64`

    Let,      // keyword `let`
    Const,    // keyword `const`
    Func,     // keyword `func`
    Ret,      // keyword `return`
    If,       // keyword `if`
    Else,     // keyword `else`
    For,      // keyword `for`
    Break,    // keyword `break`
    Continue, // keyword `continue`
    Struct,   // keyword `struct`
    Pub,      // keyword `pub`
    Impl,     // keyword `impl`
    Trait,    // keyword `trait`
    Nil,      // keyword `nil`
    New,      // keyword `new`
    Del,      // keyword `del`
    Mod,      // keyword `mod`
    Import,   // keyword `import`
    Static,   // keyword `static`

    BoolLit,  // bool literal
    CharLit,  // character literal
    I8Lit,    // i8 literal
    I16Lit,   // i16 literal
    I32Lit,   // i32 literal
    I64Lit,   // i64 literal
    I128Lit,  // i128 literal
    ISizeLit, // isize literal
    U8Lit,    // u8 literal
    U16Lit,   // u16 literal
    U32Lit,   // u32 literal
    U64Lit,   // u64 literal
    U128Lit,  // u128 literal
    USizeLit, // usize literal
    F32Lit,   // f32 literal
    F64Lit,   // f64 literal
    IntLit,   // integer literal (unresolved width)
    StrLit,   // string literal

    Semi,      // `;`
    Comma,     // `,`
    Dot,       // `.`
    LParen,    // `(`
    RParen,    // `)`
    LBrace,    // `}`
    RBrace,    // `{`
    LBracket,  // `[`
    RBracket,  // `]`
    At,        // `@`
    Tilde,     // `~`
    Question,  // `?`
    Colon,     // `:`
    Dollar,    // `$`
    Eq,        // `=`
    BitAnd,    // '&`
    BitOr,     // `|`
    LogAnd,    // '&&`
    LogOr,     // `||`
    Plus,      // `+`
    Minus,     // `-`
    Star,      // `*`
    Slash,     // `/`
    Percent,   // `%`
    PlusEq,    // `+=`
    MinusEq,   // `-=`
    StarEq,    // `*=`
    SlashEq,   // `/=`
    PercentEq, // `%=`
    Bang,      // `!`
    BangEq,    // `!=`
    EqEq,      // `==`
    Lt,        // `<`
    Gt,        // `>`
    LtEq,      // `<=`
    GtEq,      // `>=`
    Carret,    // `^`

    Unknown,
    Eof
};

}
