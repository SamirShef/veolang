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
import hir;
import sema;

let alloc: mem.MallocAllocator;
let arena = mem.ArenaAllocator.init(alloc, 64uz * mem.KB);

func main(): i32 {
    let main_file = fs.File.open("src/tests/var_decl.veo", "r");
    if !main_file.is_open() {
        std.panic("Cannot open file src/main.veo");
    }
    let content   = main_file.read_all(alloc);
    let mgr       = source_mgr.SourceMgr.new(alloc);
    let buffer_id = mgr.add_buffer(alloc, content); // [OWNERSHIP: ACQUIRE]
    let mod_id    = basic.hash64("main", 4uz);
    let lex       = lexer.Lexer.new(mgr, buffer_id);
    let ty_ctx    = types.Context.new(&arena);
    let ast_ctx   = ast.Context.new(&arena);
    let parser    = ast.Parser.new(&lex, &ty_ctx, &ast_ctx);
    let parse_res = parser.parse();
    let collector = ast.DefIdCollector.new(mod_id);
    collector.collect(parse_res);
    let dumper: ast.Dumper;
    dumper.dump(parse_res);
    let hir_ctx     = hir.Context.new(&arena);
    let hir_builder = hir.Builder.new(&hir_ctx);
    let semantic    = sema.Sema.new(alloc, &hir_builder, &ty_ctx);
    semantic.analyze(alloc, parse_res);
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
