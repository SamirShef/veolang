import basic;
import lexer;
import std;
import std.fs;
import std.mem;
import std.io;
import std.sys;

let alloc: mem.MallocAllocator;

func main(): i32 {
    let tok = lexer.Token.new(lexer.TOK_ID, std.String.from("foo"), basic.Span{});
    return 0;
}
