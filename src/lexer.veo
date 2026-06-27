import std.math;
import llvm.smloc;
import basic;
import std.mem;
import std;
import std.sys;
import llvm.source_mgr;

pub const TOK_ID = 0;

pub struct Token {
    pub kind: i32;
    pub val: std.StringView;
    pub range: basic.Span;
}

impl Token {
    pub static func new(kind: i32, val: std.StringView, range: basic.Span): Token {
        return Token { kind: kind, val: val, range: range };
    }

    pub static func new(kind: i32, range: basic.Span): Token {
        return Token { kind: kind, range: range };
    }
}

impl std.ToString for Token {
    pub func to_string(alloc: mem.Allocator): std.String {
        let s: std.String;
        s.append(alloc, std.i32_to_string(alloc, this.kind));
        s.append(alloc, '('.(u8));
        s.append(alloc, this.val);
        s.append(alloc, ')'.(u8));
        return s;
    }
}

pub struct Lexer {
    buf_start: *u8;
    buf_end: *u8;
    cur: *u8;
}

impl Lexer {
    pub static func new(mgr: source_mgr.SourceMgr, id: usize): Lexer {
        let raw_buf = mgr.get_buffer(id);
        if !raw_buf.has_val() {
            sys.write(2, "veo panic: ", 11uz);
            sys.write(2, "buffer with id ", 15uz);
            sys.__veo_print_u64(2, id.(u64));
            sys.write(2, "does not loaded\n", 16uz);
            sys.write(2, "\naborting execution...\n", 23uz);
            sys.exit(1);
        }
        let buf = raw_buf.unwrap();
        return Lexer {
            buf_start: buf.data(),
            buf_end: buf.data() + buf.len(),
            cur: buf.data()
        };
    }
}
