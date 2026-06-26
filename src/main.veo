import std.math;
import std;
import llvm.smloc;
import basic;
import lexer;
import std.fs;
import std.mem;
import std.io;
import std.sys;

let alloc: mem.MallocAllocator;

func main(): i32 {
    let tok = lexer.Token.new(lexer.TOK_ID, std.String.from(alloc, "foo"), basic.Span{});
    io.println(tok.to_string(alloc));
    return 0;
}
