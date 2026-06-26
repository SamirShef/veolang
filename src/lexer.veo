import basic;
import std;

pub const TOK_ID = 0;

pub struct Token {
    pub kind: i32;
    pub val: std.String;
    pub range: basic.Span;
}

impl Token {
    pub static func new(kind: i32, val: std.String, range: basic.Span): Token {
        return Token { kind: kind, val: val, range: range };
    }

    pub static func new(kind: i32, range: basic.Span): Token {
        return Token { kind: kind, range: range };
    }
}
