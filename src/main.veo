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
import types;
import ast;

let alloc: mem.MallocAllocator;
let arena = mem.ArenaAllocator.init(alloc, 64uz * mem.KB);

func main(): i32 {
    let main_file = fs.File.open("src/main.veo", "r");
    if !main_file.is_open() {
        std.panic("Cannot open file src/main.veo");
    }
    let content   = main_file.read_all(alloc);
    let mgr       = source_mgr.SourceMgr.new(alloc);
    let buffer_id = mgr.add_buffer(alloc, content); // [OWNERSHIP: ACQUIRE]
    let lex       = lexer.Lexer.new(mgr, buffer_id);
    let ty_ctx    = types.Context.new(&arena);
    let ast_ctx   = ast.Context.new(&arena);
    let parser    = ast.Parser.new(&lex, &ty_ctx, &ast_ctx);
    let parse_res = parser.parse();
    /*
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
    */
    mgr.destroy(alloc);
    arena.reset();
    return 0;
}
