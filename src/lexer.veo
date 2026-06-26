import std.math;
import llvm.smloc;
import basic;
import std.mem;
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

impl std.ToString for Token {
    pub func to_string(alloc: mem.Allocator): std.String {
        let s: std.String;
        s.append(alloc, this.kind.to_string(alloc));
        s.append(alloc, '('.(u8));
        s.append(alloc, this.val);
        s.append(alloc, ')'.(u8));
        return s;
    }
}
