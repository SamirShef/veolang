import std.math;
import std;
import llvm.smloc;
import basic;
import lexer;
import std.fs;
import std.mem;
import std.io;
import std.sys;
import llvm.source_mgr;

let alloc: mem.MallocAllocator;

func main(): i32 {
    let main_file = fs.File.open("src/main.veo", "r");
    if !main_file.is_open() {
        std.panic("Cannot open file src/main.veo");
    }
    let content = main_file.read_all(alloc);
    let mgr = source_mgr.SourceMgr.new(alloc);
    let buffer_id = mgr.add_buffer(alloc, content); // [OWNERSHIP: ACQUIRE]
    let lex = lexer.Lexer.new(mgr, buffer_id);
    let count = 0uz;
    for {
        let tok = lex.next_token();
        if !tok.has_val() {
            break;
        }
        io.println(tok.unwrap().to_string(alloc));
        if tok.unwrap().kind == lexer.TOK_EOF {
            break;
        }
        count += 1;
    }
    io.println(count.(i32));
    return 0;
}
