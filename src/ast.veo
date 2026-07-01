import std.sys;
import std.math;
import std.mem;
import std;
import std.io;
import llvm.source_mgr;
import llvm.smloc;
import basic;
import types;
import lexer;

pub const ACCESS_PRIV = 0;
pub const ACCESS_PUB  = 1;

pub const NODE_STMT_START =   0;
pub const NODE_VAR_DECL   =   0;
pub const NODE_STMT_END   = 100;

pub const NODE_EXPR_START = 101;
pub const NODE_LIT_EXPR   = 101;
pub const NODE_BIN_EXPR   = 102;
pub const NODE_EXPR_END   = 200;

pub struct Node {
    kind: i32;
    range: basic.Span;
}

impl Node {
    pub static func new(kind: i32, range: basic.Span): Node {
        return Node {
            kind: kind,
            range: range
        };
    }

    pub func kind(): i32 {
        return this.kind;
    }

    pub func range(): basic.Span {
        return this.range;
    }
}

pub struct Stmt {
    base: Node;
    access: i32;
}

impl Stmt {
    pub static func new(kind: i32, range: basic.Span): Stmt {
        return Stmt {
            base: Node.new(kind, range),
            access: ACCESS_PRIV
        };
    }

    pub static func new(access: i32, kind: i32, range: basic.Span): Stmt {
        return Stmt {
            base: Node.new(kind, range),
            access: access
        };
    }

    pub static func isa(node: *Node): bool {
        if node == nil {
            return false;
        }
        return node.kind() >= NODE_STMT_START && node.kind() <= NODE_STMT_END;
    }

    pub static func cast(node: *Node): *Stmt {
        if !Stmt.isa(node) {
            std.panic("RTTI Error: Failed cast to *Stmt");
        }
        return node.(*Stmt);
    }
}

pub struct Expr {
    base: Node;
}

impl Expr {
    pub static func new(kind: i32, range: basic.Span): Expr {
        return Expr {
            base: Node.new(kind, range)
        };
    }

    pub static func isa(node: *Node): bool {
        if node == nil {
            return false;
        }
        return node.kind() >= NODE_EXPR_START && node.kind() <= NODE_EXPR_END;
    }

    pub static func cast(node: *Node): *Expr {
        if !Expr.isa(node) {
            std.panic("RTTI Error: Failed cast to *Expr");
        }
        return node.(*Expr);
    }
}

pub struct Context {
    alloc: *mem.ArenaAllocator;
}

impl Context {
    pub static func new(alloc: *mem.ArenaAllocator): Context {
        return Context { alloc: alloc };
    }

    pub func alloc_node_array(count: usize): **Node {
        if count == 0uz {
            return nil;
        }
        let ptr: *Node;
        return this.alloc.alloc(count * @size_of(ptr)).(**Node);
    }

    pub func alloc_var_decl(range: basic.Span, name: std.StringView, type: *types.Type, init: *Expr): *VarDecl {
        let mem_ptr = this.alloc.alloc(@size_of(VarDecl));
        let node = mem_ptr.(*VarDecl);

        node.base = Stmt.new(NODE_VAR_DECL, range);
        node.name = name;
        node.ty   = type;
        node.init = init;

        return node;
    }
}

pub struct VarDecl {
    pub base: Stmt;
    pub name: std.StringView;
    pub ty: *types.Type;
    pub init: *Expr;
}

impl VarDecl {
    pub static func isa(node: *Node): bool {
        if node == nil {
            return false;
        }
        return node.kind() == NODE_VAR_DECL;
    }

    pub static func cast(node: *Node): *VarDecl {
        if !VarDecl.isa(node) {
            std.panic("RTTI Error: Failed cast to *VarDecl");
        }
        return node.(*VarDecl);
    }
}

pub struct ParseResult {
    pub nodes: **Node;
    pub count: usize;
    pub has_errs: bool;
}

pub struct Parser {
    lex: *lexer.Lexer;
    ty_ctx: *types.Context;
    ast_ctx: *Context;
    last_tok: lexer.Token;
    cur_tok: lexer.Token;
    next_tok: lexer.Token;
}

impl Parser {
    pub static func new(lex: *lexer.Lexer, ty_ctx: *types.Context, ast_ctx: *Context): Parser {
        let p = Parser {
            lex: lex,
            ty_ctx: ty_ctx,
            ast_ctx: ast_ctx
        };
        p.advance();
        p.advance();
        return p;
    }

    pub func parse(): ParseResult {
        let ptr: *Node;
        let cap      = 128uz;
        let count    = 0uz;
        let has_errs = false;
        let nodes    = sys.malloc(cap * @size_of(ptr)).(**Node);

        for !this.is_at_end() {
            let node = this.parse_stmt();
            if node == nil {
                has_errs = true;
                // TODO: invoke synchronize function
                continue;
            }
            if count >= cap {
                cap *= 2;
                nodes = sys.realloc(nodes.(*u8), cap * @size_of(ptr)).(**Node);
            }
            *(nodes + count) = node.(*Node);
            count += 1;
        }
        let final_nodes = this.ast_ctx.alloc_node_array(count);
        if count > 0uz {
            sys.memcpy(final_nodes.(*u8), nodes.(*u8), count * @size_of(ptr));
        }
        sys.free(nodes.(*u8));
        return ParseResult {
            nodes: final_nodes,
            count: count,
            has_errs: has_errs
        };
    }

    func parse_stmt(): *Stmt {
        this.advance();
        return nil;
    }

    pub func advance(): lexer.Token {
        this.last_tok = this.cur_tok;
        this.cur_tok = this.next_tok;
        this.next_tok = this.lex.next_token().unwrap();
        return this.last_tok;
    }

    func is_at_end(): bool {
        return this.cur_tok.kind == lexer.TOK_EOF;
    }
}
