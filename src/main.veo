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
    let content = main_file.read_all(alloc);
    let mgr = source_mgr.SourceMgr.new(alloc);
    let buffer_id = mgr.add_buffer(alloc, content); // [OWNERSHIP: ACQUIRE]
    let lex = lexer.Lexer.new(mgr, buffer_id);
    let count = 0uz;
    /*
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
    let ty_ctx = types.Context.new(&arena);
    let range = basic.Span.new(smloc.SMLoc.new(nil.(*u8)), smloc.SMLoc.new(nil.(*u8)));
    let name = std.StringView.from("foo", 3uz);
    let var_decl = ast.VarDecl.alloc(arena, range, name, ty_ctx.get_int_ty(32u32, false), nil.(*ast.Expr));

    mgr.destroy(alloc);
    arena.reset();
    return 0;
}
