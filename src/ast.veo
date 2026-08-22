import std.sys;
import std.math;
import std.mem;
import std;
import std.io;
import basic;
import types;
import lexer;
import diag;

pub const ACCESS_PRIV = 0;
pub const ACCESS_PUB  = 1;

pub const NODE_STMT_START =   0;
pub const NODE_VAR_DECL   =   0;
pub const NODE_FUNC_DECL  =   1;
pub const NODE_BLOCK_STMT =   2;
pub const NODE_RET_STMT   =   3;
pub const NODE_STMT_END   = 100;

pub const NODE_EXPR_START = 101;
pub const NODE_LIT_EXPR   = 101;
pub const NODE_BIN_EXPR   = 102;
pub const NODE_UN_EXPR    = 103;
pub const NODE_VAR_EXPR   = 104;
pub const NODE_CALL_EXPR  = 105;
pub const NODE_EXPR_END   = 200;

pub struct Node {
    kind: i32;
    id: u32;
    range: basic.Span;
}

impl Node {
    pub static func new(kind: i32, id: u32, range: basic.Span): Node {
        return Node {
            kind: kind,
            id: id,
            range: range
        };
    }

    pub func kind(): i32 {
        return this.kind;
    }

    pub func id(): u32 {
        return this.id;
    }

    pub func range(): basic.Span {
        return this.range;
    }

    pub func set_range(range: basic.Span) {
        this.range = range;
    }

    pub func set_range(start: basic.Pos, end: basic.Pos) {
        this.set_range(basic.Span.new(start, end));
    }
}

pub struct Stmt {
    base: Node;
    access: i32;
}

impl Stmt {
    pub static func new(kind: i32, id: u32, range: basic.Span): Stmt {
        return Stmt {
            base: Node.new(kind, id, range),
            access: ACCESS_PRIV
        };
    }

    pub static func new(access: i32, kind: i32, id: u32, range: basic.Span): Stmt {
        return Stmt {
            base: Node.new(kind, id, range),
            access: access
        };
    }

    pub func kind(): i32 {
        return this.base.kind();
    }

    pub func id(): u32 {
        return this.base.id();
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
    pub static func new(kind: i32, id: u32, range: basic.Span): Expr {
        return Expr {
            base: Node.new(kind, id, range)
        };
    }

    pub func kind(): i32 {
        return this.base.kind();
    }

    pub func id(): u32 {
        return this.base.id();
    }

    pub func range(): basic.Span {
        return this.base.range();
    }

    pub func set_range(range: basic.Span) {
        this.base.set_range(range);
    }

    pub func set_range(start: basic.Pos, end: basic.Pos) {
        this.set_range(basic.Span.new(start, end));
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
    pub alloc: mem.ArenaAllocator;
    cur_id: u32;
    nodes: **Node;
    nodes_count: usize;
    nodes_cap: usize;
}

impl Context {
    pub static func new(): Context {
        return Context {
            alloc: mem.ArenaAllocator.init(64uz * mem.KB),
            cur_id: 0,
            nodes: nil,
            nodes_count: 0,
            nodes_cap: 0
        };
    }

    func register_node(node: *Node) {
        if this.nodes_cap < this.nodes_count + 1uz {
            let old_cap = this.nodes_cap;
            this.nodes_cap = math.max(this.nodes_cap * 2uz, this.nodes_count + 1uz);
            this.nodes = sys.realloc(
                            this.nodes.(*u8),
                            this.nodes_cap * @size_of(Node)
                        ).(*Node);
        }
        *(this.nodes + node.id()) = node;
        this.nodes_count += 1;
    }

    pub func get(id: u32): *Node {
        if id.(usize) < this.nodes_count {
            return *(this.nodes + id.(usize));
        }
        return nil;
    }

    pub func alloc_node_array(count: usize): **Node {
        if count == 0uz {
            return nil;
        }
        return this.alloc.alloc(count * @size_of(*Node)).(**Node);
    }

    pub func alloc_var_decl(range: basic.Span, name: std.StringView, is_const: bool,
                            type: *types.Type, init: *Expr): *VarDecl {
        let mem_ptr = this.alloc.alloc(@size_of(VarDecl));
        let node    = mem_ptr.(*VarDecl);

        node.base     = Stmt.new(NODE_VAR_DECL, this.cur_id, range);
        this.cur_id   += 1;
        node.name     = name;
        node.is_const = is_const;
        node.ty       = type;
        node.init     = init;
        this.register_node(node.(*Node));

        return node;
    }

    pub func alloc_func_decl(range: basic.Span, name: std.StringView, ret_ty: *types.Type,
                             args: *Argument, args_count: usize, body: *BlockStmt): *FuncDecl {
        let mem_ptr = this.alloc.alloc(@size_of(FuncDecl));
        let node    = mem_ptr.(*FuncDecl);

        node.base       = Stmt.new(NODE_FUNC_DECL, this.cur_id, range);
        this.cur_id     += 1;
        node.name       = name;
        node.ret_ty     = ret_ty;
        node.args       = args;
        node.args_count = args_count;
        node.body       = body;
        this.register_node(node.(*Node));

        return node;
    }

    pub func alloc_block_stmt(range: basic.Span, stmts: **Stmt,
                              stmts_count: usize): *BlockStmt {
        let mem_ptr = this.alloc.alloc(@size_of(BlockStmt));
        let node    = mem_ptr.(*BlockStmt);

        node.base        = Stmt.new(NODE_BLOCK_STMT, this.cur_id, range);
        this.cur_id      += 1;
        node.stmts       = stmts;
        node.stmts_count = stmts_count;
        this.register_node(node.(*Node));

        return node;
    }

    pub func alloc_ret_stmt(range: basic.Span, expr: *Expr): *RetStmt {
        let mem_ptr = this.alloc.alloc(@size_of(RetStmt));
        let node    = mem_ptr.(*RetStmt);

        node.base   = Stmt.new(NODE_RET_STMT, this.cur_id, range);
        this.cur_id += 1;
        node.expr   = expr;
        this.register_node(node.(*Node));

        return node;
    }

    pub func alloc_lit_expr(range: basic.Span, val: std.StringView, tok_kind: i32): *LitExpr {
        let mem_ptr = this.alloc.alloc(@size_of(LitExpr));
        let node    = mem_ptr.(*LitExpr);

        node.base     = Expr.new(NODE_LIT_EXPR, this.cur_id, range);
        this.cur_id  += 1;
        node.val      = val;
        node.tok_kind = tok_kind;
        this.register_node(node.(*Node));

        return node;
    }

    pub func alloc_bin_expr(range: basic.Span, op: i32, left: *Expr, right: *Expr): *BinExpr {
        let mem_ptr = this.alloc.alloc(@size_of(BinExpr));
        let node    = mem_ptr.(*BinExpr);

        node.base   = Expr.new(NODE_BIN_EXPR, this.cur_id, range);
        this.cur_id += 1;
        node.op     = op;
        node.left   = left;
        node.right  = right;
        this.register_node(node.(*Node));

        return node;
    }

    pub func alloc_un_expr(range: basic.Span, op: i32, right: *Expr): *UnExpr {
        let mem_ptr = this.alloc.alloc(@size_of(UnExpr));
        let node    = mem_ptr.(*UnExpr);

        node.base   = Expr.new(NODE_UN_EXPR, this.cur_id, range);
        this.cur_id += 1;
        node.op     = op;
        node.right  = right;
        this.register_node(node.(*Node));

        return node;
    }

    pub func alloc_var_expr(range: basic.Span, name: std.StringView): *VarExpr {
        let mem_ptr = this.alloc.alloc(@size_of(VarExpr));
        let node    = mem_ptr.(*VarExpr);

        node.base   = Expr.new(NODE_VAR_EXPR, this.cur_id, range);
        this.cur_id += 1;
        node.name   = name;
        this.register_node(node.(*Node));

        return node;
    }

    pub func alloc_call_expr(range: basic.Span, callee: *Expr, args: **Expr,
                             args_count: usize): *CallExpr {
        let mem_ptr = this.alloc.alloc(@size_of(CallExpr));
        let node    = mem_ptr.(*CallExpr);

        node.base       = Expr.new(NODE_CALL_EXPR, this.cur_id, range);
        this.cur_id     += 1;
        node.callee     = callee;
        node.args       = args;
        node.args_count = args_count;
        this.register_node(node.(*Node));

        return node;
    }

    pub func destroy() {
        this.alloc.reset();
        sys.free(this.nodes.(*u8));
        this.nodes_cap   = 0;
        this.nodes_count = 0;
    }
}

pub struct VarDecl {
    pub base: Stmt;
    pub name: std.StringView;
    pub is_const: bool;
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

pub struct BlockStmt {
    pub base: Stmt;
    pub stmts: **Stmt;
    pub stmts_count: usize;
}

impl BlockStmt {
    pub static func isa(node: *Node): bool {
        if node == nil {
            return false;
        }
        return node.kind() == NODE_BLOCK_STMT;
    }

    pub static func cast(node: *Node): *BlockStmt {
        if !BlockStmt.isa(node) {
            std.panic("RTTI Error: Failed cast to *BlockStmt");
        }
        return node.(*BlockStmt);
    }
}

pub struct Argument {
    pub name: std.StringView;
    pub ty: *types.Type;
}

pub struct FuncDecl {
    pub base: Stmt;
    pub name: std.StringView;
    pub ret_ty: *types.Type;
    pub args: *Argument;
    pub args_count: usize;
    pub body: *BlockStmt;
}

impl FuncDecl {
    pub static func isa(node: *Node): bool {
        if node == nil {
            return false;
        }
        return node.kind() == NODE_FUNC_DECL;
    }

    pub static func cast(node: *Node): *FuncDecl {
        if !FuncDecl.isa(node) {
            std.panic("RTTI Error: Failed cast to *FuncDecl");
        }
        return node.(*FuncDecl);
    }
}

pub struct RetStmt {
    pub base: Stmt;
    pub expr: *Expr;
}

impl RetStmt {
    pub static func isa(node: *Node): bool {
        if node == nil {
            return false;
        }
        return node.kind() == NODE_RET_STMT;
    }

    pub static func cast(node: *Node): *RetStmt {
        if !RetStmt.isa(node) {
            std.panic("RTTI Error: Failed cast to *RetStmt");
        }
        return node.(*RetStmt);
    }
}

pub struct LitExpr {
    pub base: Expr;
    pub val: std.StringView;
    pub tok_kind: i32;
}

impl LitExpr {
    pub static func isa(node: *Node): bool {
        if node == nil {
            return false;
        }
        return node.kind() == NODE_LIT_EXPR;
    }

    pub static func cast(node: *Node): *LitExpr {
        if !LitExpr.isa(node) {
            std.panic("RTTI Error: Failed cast to *LitExpr");
        }
        return node.(*LitExpr);
    }
}

pub const BIN_OP_INVALID = -1;
pub const BIN_OP_PLUS    =  0;
pub const BIN_OP_MINUS   =  1;
pub const BIN_OP_MUL     =  2;
pub const BIN_OP_DIV     =  3;
pub const BIN_OP_REM     =  4;
pub const BIN_OP_EQ      =  5;
pub const BIN_OP_NOT_EQ  =  6;
pub const BIN_OP_LT      =  7;
pub const BIN_OP_LT_EQ   =  8;
pub const BIN_OP_GT      =  9;
pub const BIN_OP_GT_EQ   = 10;
pub const BIN_OP_LOG_AND = 11;
pub const BIN_OP_LOG_OR  = 12;
pub const BIN_OP_BIT_AND = 13;
pub const BIN_OP_BIT_OR  = 14;
pub const BIN_OP_BIT_XOR = 15;

func tok_to_bin_op(kind: i32): i32 {
    if kind == lexer.TOK_PLUS {
        return BIN_OP_PLUS;
    } else if kind == lexer.TOK_MINUS {
        return BIN_OP_MINUS;
    } else if kind == lexer.TOK_STAR {
        return BIN_OP_MUL;
    } else if kind == lexer.TOK_SLASH {
        return BIN_OP_DIV;
    } else if kind == lexer.TOK_PERCENT {
        return BIN_OP_REM;
    } else if kind == lexer.TOK_EQ_EQ {
        return BIN_OP_EQ;
    } else if kind == lexer.TOK_BANG_EQ {
        return BIN_OP_NOT_EQ;
    } else if kind == lexer.TOK_LT {
        return BIN_OP_LT;
    } else if kind == lexer.TOK_LT_EQ {
        return BIN_OP_LT_EQ;
    } else if kind == lexer.TOK_GT {
        return BIN_OP_GT;
    } else if kind == lexer.TOK_GT_EQ {
        return BIN_OP_GT_EQ;
    } else if kind == lexer.TOK_AMPAMP {
        return BIN_OP_LOG_AND;
    } else if kind == lexer.TOK_PIPEPIPE {
        return BIN_OP_LOG_OR;
    } else if kind == lexer.TOK_AMP {
        return BIN_OP_BIT_AND;
    } else if kind == lexer.TOK_PIPE {
        return BIN_OP_BIT_OR;
    } else if kind == lexer.TOK_CARET {
        return BIN_OP_BIT_XOR;
    }
    return BIN_OP_INVALID;
}

func bin_op_to_str(op: i32): *u8 {
    if op == BIN_OP_PLUS {
        return "+";
    } else if op == BIN_OP_MINUS {
        return "-";
    } else if op == BIN_OP_MUL {
        return "*";
    } else if op == BIN_OP_DIV {
        return "/";
    } else if op == BIN_OP_REM {
        return "%";
    } else if op == BIN_OP_EQ {
        return "==";
    } else if op == BIN_OP_NOT_EQ {
        return "!=";
    } else if op == BIN_OP_LT {
        return "<";
    } else if op == BIN_OP_LT_EQ {
        return "<=";
    } else if op == BIN_OP_GT {
        return ">";
    } else if op == BIN_OP_GT_EQ {
        return ">=";
    } else if op == BIN_OP_LOG_AND {
        return "&&";
    } else if op == BIN_OP_LOG_OR{
        return "||";
    } else if op == BIN_OP_BIT_AND {
        return "&";
    } else if op == BIN_OP_BIT_OR {
        return "|";
    } else if op == BIN_OP_BIT_XOR {
        return "^";
    }
    return "<invalid>";
}

pub struct BinExpr {
    pub base: Expr;
    pub op: i32;
    pub left: *Expr;
    pub right: *Expr;
}

impl BinExpr {
    pub static func isa(node: *Node): bool {
        if node == nil {
            return false;
        }
        return node.kind() == NODE_BIN_EXPR;
    }

    pub static func cast(node: *Node): *BinExpr {
        if !BinExpr.isa(node) {
            std.panic("RTTI Error: Failed cast to *BinExpr");
        }
        return node.(*BinExpr);
    }
}

pub const UN_OP_INVALID = -1;
pub const UN_OP_MINUS   =  0;
pub const UN_OP_NOT     =  1;
pub const UN_OP_INVERSE =  2;

func tok_to_un_op(kind: i32): i32 {
    if kind == lexer.TOK_MINUS {
        return UN_OP_MINUS;
    } else if kind == lexer.TOK_BANG {
        return UN_OP_NOT;
    } else if kind == lexer.TOK_TILDE {
        return UN_OP_INVERSE;
    }
    return UN_OP_INVALID;
}

func un_op_to_str(op: i32): *u8 {
    if op == UN_OP_MINUS {
        return "-";
    } else if op == UN_OP_NOT {
        return "!";
    } else if op == UN_OP_INVERSE {
        return "~";
    }
    return "<invalid>";
}

pub struct UnExpr {
    pub base: Expr;
    pub op: i32;
    pub right: *Expr;
}

impl UnExpr {
    pub static func isa(node: *Node): bool {
        if node == nil {
            return false;
        }
        return node.kind() == NODE_UN_EXPR;
    }

    pub static func cast(node: *Node): *UnExpr {
        if !UnExpr.isa(node) {
            std.panic("RTTI Error: Failed cast to *UnExpr");
        }
        return node.(*UnExpr);
    }
}

pub struct VarExpr {
    pub base: Expr;
    pub name: std.StringView;
}

impl VarExpr {
    pub static func isa(node: *Node): bool {
        if node == nil {
            return false;
        }
        return node.kind() == NODE_VAR_EXPR;
    }

    pub static func cast(node: *Node): *VarExpr {
        if !VarExpr.isa(node) {
            std.panic("RTTI Error: Failed cast to *VarExpr");
        }
        return node.(*VarExpr);
    }
}

pub struct CallExpr {
    pub base: Expr;
    pub callee: *Expr;
    pub args: **Expr;
    pub args_count: usize;
}

impl CallExpr {
    pub static func isa(node: *Node): bool {
        if node == nil {
            return false;
        }
        return node.kind() == NODE_CALL_EXPR;
    }

    pub static func cast(node: *Node): *CallExpr {
        if !CallExpr.isa(node) {
            std.panic("RTTI Error: Failed cast to *CallExpr");
        }
        return node.(*CallExpr);
    }
}

pub struct ParseResult {
    pub nodes: **Node;
    pub count: usize;
    pub has_errs: bool;
}

const PREC_LOWEST     =  0;
const PREC_ASSIGNMENT =  1;
const PREC_TERNARY    =  2;
const PREC_LOG_OR     =  3;
const PREC_LOG_AND    =  4;
const PREC_BIT_OR     =  5;
const PREC_BIT_XOR    =  6;
const PREC_BIT_AND    =  7;
const PREC_EQUALITY   =  8;
const PREC_COMPARISON =  9;
const PREC_SUM        = 10;
const PREC_PRODUCT    = 11;
const PREC_UNARY      = 12;
const PREC_MEMBER     = 13;
const PREC_CALL       = 14;

func tok_is_assignment(kind: i32): bool {
    return kind == lexer.TOK_EQ || kind == lexer.TOK_PLUS_EQ || kind == lexer.TOK_MINUS_EQ
        || kind == lexer.TOK_STAR_EQ || kind == lexer.TOK_SLASH_EQ || kind == lexer.TOK_PERCENT_EQ
        || kind == lexer.TOK_AMP_EQ || kind == lexer.TOK_PIPE_EQ || kind == lexer.TOK_CARET_EQ;
}

func tok_precedence(kind: i32): i32 {
    if kind == lexer.TOK_LPAREN {
        return PREC_CALL;
    } else if kind == lexer.TOK_DOT {
        return PREC_MEMBER;
    } else if tok_is_assignment(kind) {
        return PREC_ASSIGNMENT;
    } else if kind == lexer.TOK_QUESTION {
        return PREC_TERNARY;
    } else if kind == lexer.TOK_AMPAMP {
        return PREC_LOG_AND;
    } else if kind == lexer.TOK_PIPEPIPE {
        return PREC_LOG_OR;
    } else if kind == lexer.TOK_AMP {
        return PREC_BIT_AND;
    } else if kind == lexer.TOK_PIPE {
        return PREC_BIT_OR;
    } else if kind == lexer.TOK_CARET {
        return PREC_BIT_XOR;
    } else if kind == lexer.TOK_EQ_EQ || kind == lexer.TOK_BANG_EQ {
        return PREC_EQUALITY;
    } else if kind == lexer.TOK_LT || kind == lexer.TOK_LT_EQ
        || kind == lexer.TOK_GT || kind == lexer.TOK_GT_EQ {
        return PREC_COMPARISON;
    } else if kind == lexer.TOK_PLUS || kind == lexer.TOK_MINUS {
        return PREC_SUM;
    } else if kind == lexer.TOK_STAR || kind == lexer.TOK_SLASH || kind == lexer.TOK_PERCENT {
        return PREC_PRODUCT;
    }
    return PREC_LOWEST;
}

pub struct Parser {
    lex: *lexer.Lexer;
    engine: *diag.DiagEngine;
    ty_ctx: *types.Context;
    ast_ctx: *Context;
    prev_tok: lexer.Token;
    cur_tok: lexer.Token;
    next_tok: lexer.Token;
}

impl Parser {
    pub static func new(lex: *lexer.Lexer, engine: *diag.DiagEngine, ty_ctx: *types.Context,
                        ast_ctx: *Context): Parser {
        let p = Parser {
            lex: lex,
            engine: engine,
            ty_ctx: ty_ctx,
            ast_ctx: ast_ctx
        };
        p.advance();
        p.advance();
        return p;
    }

    pub func parse(): ParseResult {
        let cap      = 128uz;
        let count    = 0uz;
        let has_errs = false;
        let nodes    = sys.malloc(cap * @size_of(*Node)).(**Node);

        for !this.is_at_end() {
            let node = this.parse_stmt();
            if node == nil {
                has_errs = true;
                this.synchronize();
                continue;
            }
            if count >= cap {
                cap *= 2;
                nodes = sys.realloc(nodes.(*u8), cap * @size_of(*Node)).(**Node);
            }
            *(nodes + count) = node.(*Node);
            count += 1;
        }
        let final_nodes = this.ast_ctx.alloc_node_array(count);
        if count > 0uz {
            sys.memcpy(final_nodes.(*u8), nodes.(*u8), count * @size_of(*Node));
        }
        sys.free(nodes.(*u8));
        return ParseResult {
            nodes: final_nodes,
            count: count,
            has_errs: has_errs
        };
    }

    func parse_stmt(): *Stmt {
        return this.parse_stmt(true);
    }

    func parse_stmt(expect_semi: bool): *Stmt {
        let kind = this.cur_tok.kind;
        if kind == lexer.TOK_LET || kind == lexer.TOK_CONST {
            return this.check_trailing_semi(this.parse_var_decl(), expect_semi);
        } else if kind == lexer.TOK_FUNC {
            return this.parse_func_decl();
        } else if kind == lexer.TOK_LBRACE {
            return this.parse_block_stmt();
        } else if kind == lexer.TOK_RET {
            return this.check_trailing_semi(this.parse_ret_stmt(), expect_semi);
        }
        this.engine.report(diag.E_UNEXPECTED_TOKEN, "unexpected token", diag.SEV_ERROR).
            span(this.cur_tok.range);
        this.advance();
        return nil;
    }

    func parse_var_decl(): *Stmt {
        let first_tok = this.advance();
        let is_const = first_tok.kind == lexer.TOK_CONST;
        let name_tok = this.advance();
        if name_tok.kind != lexer.TOK_ID {
            return nil;
        }
        let name = name_tok.val;
        let ty: *types.Type;
        if this.match(lexer.TOK_COLON) {
            ty = this.consume_type();
        }
        let expr: *Expr;
        if this.match(lexer.TOK_EQ) {
            expr = this.parse_expr();
        }
        let range = basic.Span.new(
            first_tok.range.start,
            this.cur_tok.range.end
        );
        return this.ast_ctx.alloc_var_decl(range, name, is_const, ty, expr).(*Stmt);
    }

    func parse_func_decl(): *Stmt {
        let first_tok   = this.advance();
        let name_tok = this.advance();
        if name_tok.kind != lexer.TOK_ID {
            return nil;
        }
        let name = name_tok.val;
        if !this.expect_tok(lexer.TOK_LPAREN) {
            this.engine.report(diag.E_UNEXPECTED_TOKEN, "expected '('", diag.SEV_ERROR)
                .span(this.cur_tok.range);
            this.synchronize();
            return nil;
        }
        let args_count = 0uz;
        let args_cap   = 4uz;
        let args       = sys.malloc(args_cap * @size_of(Argument)).(*Argument);
        for !this.match(lexer.TOK_RPAREN) {
            if args_count != 0uz {
                if !this.expect_tok(lexer.TOK_COMMA) {
                    this.engine.report(diag.E_UNEXPECTED_TOKEN, "expected ','", diag.SEV_ERROR)
                        .span(this.cur_tok.range);
                    this.synchronize();
                }
            }
            let name_tok = this.advance();
            if name_tok.kind != lexer.TOK_ID {
                this.engine.report(diag.E_UNEXPECTED_TOKEN, "expected identifier", diag.SEV_ERROR)
                    .span(this.cur_tok.range);
                this.synchronize();
                continue;
            }
            let name = name_tok.val;
            if !this.expect_tok(lexer.TOK_COLON) {
                this.engine.report(diag.E_UNEXPECTED_TOKEN, "expected ':'", diag.SEV_ERROR)
                    .span(this.cur_tok.range);
                this.synchronize();
            }
            let ty = this.consume_type();
            let arg = Argument { name: name, ty: ty };
            if args_count >= args_cap {
                args_cap = math.max(args_cap * 2uz, args_cap + 1uz);
                args     = sys.realloc(args.(*u8), args_cap * @size_of(Argument)).(*Argument);
            }
            *(args + args_count) = arg;
            args_count += 1;
        }
        let final_args = this.ast_ctx.alloc.alloc(args_count * @size_of(Argument)).(*Argument);
        sys.memcpy(final_args.(*u8), args.(*u8), args_count * @size_of(Argument));
        sys.free(args.(*u8));
        let ret_ty: *types.Type;
        if this.match(lexer.TOK_COLON) {
            ret_ty = this.consume_type();
        } else {
            ret_ty = this.ty_ctx.alloc_noth_ty();
        }
        let body: *BlockStmt;
        if this.check(lexer.TOK_LBRACE) {
            body = this.parse_block_stmt().(*BlockStmt);
        } else {
            this.engine.report(diag.E_UNEXPECTED_TOKEN, "expected '{'", diag.SEV_ERROR)
                .span(this.cur_tok.range);
        }
        let range       = basic.Span.new(
            first_tok.range.start,
            this.prev_tok.range.end
        );
        return this.ast_ctx.alloc_func_decl(range, name, ret_ty, final_args, args_count, body).(*Stmt);
    }

    func parse_block_stmt(): *Stmt {
        let first_tok   = this.advance();
        let stmts_count = 0uz;
        let stmts_cap   = 4uz;
        let stmts       = sys.malloc(stmts_cap * @size_of(*Stmt)).(**Stmt);
        for !this.match(lexer.TOK_RBRACE) {
            let stmt = this.parse_stmt();
            if stmts_count >= stmts_cap {
                stmts_cap = math.max(stmts_cap * 2uz, stmts_cap + 1uz);
                stmts     = sys.realloc(stmts.(*u8), stmts_cap * @size_of(*Stmt)).(**Stmt);
            }
            *(stmts + stmts_count) = stmt;
            stmts_count            += 1;
        }
        let final_nodes = this.ast_ctx.alloc_node_array(stmts_count).(**Stmt);
        sys.memcpy(final_nodes.(*u8), stmts.(*u8), stmts_count * @size_of(*Stmt));
        let range       = basic.Span.new(
            first_tok.range.start,
            this.cur_tok.range.end
        );
        sys.free(stmts.(*u8));
        return this.ast_ctx.alloc_block_stmt(range, final_nodes, stmts_count).(*Stmt);
    }

    func parse_ret_stmt(): *Stmt {
        let first_tok   = this.advance();
        let expr: *Expr = this.check(lexer.TOK_SEMI) ? nil : this.parse_expr();
        let range       = basic.Span.new(
            first_tok.range.start,
            this.cur_tok.range.end
        );
        return this.ast_ctx.alloc_ret_stmt(range, expr).(*Stmt);
    }

    func parse_expr(): *Expr {
        return this.parse_expr(PREC_LOWEST, true);
    }

    func parse_expr(min_prec: i32): *Expr {
        return this.parse_expr(min_prec, true);
    }

    func parse_expr(allow_struct: bool): *Expr {
        return this.parse_expr(PREC_LOWEST, allow_struct);
    }

    pub func parse_expr(min_prec: i32, allow_struct: bool): *Expr {
        let start = this.cur_tok.range.start;
        let left  = this.parse_primary_expr(allow_struct);
        let prec  = 0;
        for !this.is_at_end() && min_prec < (prec = tok_precedence(this.cur_tok.kind)) {
            if this.check(lexer.TOK_LPAREN) {
                left = this.parse_call_expr(left);
            } else {
                let op    = tok_to_bin_op(this.advance().kind);
                let right = this.parse_expr(prec, allow_struct);
                let end   = this.prev_tok.range.end;
                let range = basic.Span.new(start, end);
                left      = this.ast_ctx.alloc_bin_expr(range, op, left, right).(*Expr);
            }
        }
        return left;
    }

    func parse_primary_expr(): *Expr {
        return this.parse_primary_expr(true);
    }

    func parse_primary_expr(allow_struct: bool): *Expr {
        let tok  = this.advance();
        let kind = tok.kind;
        if kind == lexer.TOK_BOOL_LIT || kind == lexer.TOK_CHAR_LIT
            || kind == lexer.TOK_NUM_LIT || kind == lexer.TOK_STR_LIT {
            return this.ast_ctx.alloc_lit_expr(tok.range, tok.val, kind).(*Expr);
        }
        if kind == lexer.TOK_ID {
            return this.ast_ctx.alloc_var_expr(tok.range, tok.val).(*Expr);
        }
        if kind == lexer.TOK_LPAREN {
            let expr = this.parse_expr();
            if expr != nil {
                expr.set_range(tok.range.start, expr.range().end);
            }
            if !this.expect_tok(lexer.TOK_RPAREN) {
                this.engine.report(diag.E_UNEXPECTED_TOKEN, "expected ')'", diag.SEV_ERROR)
                    .span(this.cur_tok.range);
                this.synchronize();
            }
            if expr != nil {
                expr.set_range(expr.range().start, this.prev_tok.range.end);
            }
            return expr;
        }
        if kind == lexer.TOK_MINUS || kind == lexer.TOK_BANG || kind == lexer.TOK_TILDE {
            let expr = this.parse_expr(PREC_UNARY, allow_struct);
            let range = basic.Span.new(tok.range.start, this.prev_tok.range.end);
            return this.ast_ctx.alloc_un_expr(range, tok_to_un_op(kind), expr).(*Expr);
        }
        this.engine.report(diag.E_EXPECTED_EXPR, "expected expression", diag.SEV_ERROR)
            .span(tok.range);
        this.synchronize();
        return nil;
    }

    func parse_call_expr(callee: *Expr): *Expr {
        let lparen_tok = this.advance();
        let args_count = 0uz;
        let args_cap   = 4uz;
        let args       = sys.malloc(args_cap * @size_of(*Expr)).(**Expr);

        for !this.check(lexer.TOK_RPAREN) && !this.is_at_end() {
            if args_count != 0uz {
                if !this.expect_tok(lexer.TOK_COMMA) {
                    this.engine.report(diag.E_UNEXPECTED_TOKEN, "expected ','", diag.SEV_ERROR)
                        .span(this.cur_tok.range);
                    this.synchronize();
                }
            }
            let arg = this.parse_expr();
            if arg == nil {
                break;
            }
            if args_count >= args_cap {
                args_cap *= 2uz;
                args     = sys.realloc(args.(*u8), args_cap * @size_of(*Expr)).(**Expr);
            }
            *(args + args_count) = arg;
            args_count += 1;
        }

        if !this.expect_tok(lexer.TOK_RPAREN) {
            this.engine.report(diag.E_UNEXPECTED_TOKEN, "expected ')'", diag.SEV_ERROR)
                .span(this.cur_tok.range);
        }

        let final_args = this.ast_ctx.alloc.alloc(args_count * @size_of(*Expr)).(**Expr);
        sys.memcpy(final_args.(*u8), args.(*u8), args_count * @size_of(*Expr));
        sys.free(args.(*u8));

        let range = basic.Span.new(callee.range().start, this.prev_tok.range.end);
        return this.ast_ctx.alloc_call_expr(range, callee, final_args, args_count).(*Expr);
    }

    func consume_type(): *types.Type {
        let tok  = this.advance();
        let kind = tok.kind;
        if kind == lexer.TOK_I8 {
            return this.ty_ctx.get_int_ty(8u32, false);
        } else if kind == lexer.TOK_I16 {
            return this.ty_ctx.get_int_ty(16u32, false);
        } else if kind == lexer.TOK_I32 {
            return this.ty_ctx.get_int_ty(32u32, false);
        } else if kind == lexer.TOK_I64 {
            return this.ty_ctx.get_int_ty(64u32, false);
        } else if kind == lexer.TOK_ISIZE {
            return this.ty_ctx.get_size_ty(false);
        } else if kind == lexer.TOK_U8 {
            return this.ty_ctx.get_int_ty(8u32, true);
        } else if kind == lexer.TOK_U16 {
            return this.ty_ctx.get_int_ty(16u32, true);
        } else if kind == lexer.TOK_U32 {
            return this.ty_ctx.get_int_ty(32u32, true);
        } else if kind == lexer.TOK_U64 {
            return this.ty_ctx.get_int_ty(64u32, true);
        } else if kind == lexer.TOK_USIZE {
            return this.ty_ctx.get_size_ty(true);
        } else if kind == lexer.TOK_F32 {
            return this.ty_ctx.get_float_ty(32u32);
        } else if kind == lexer.TOK_F64 {
            return this.ty_ctx.get_float_ty(64u32);
        } else if kind == lexer.TOK_BOOL {
            return this.ty_ctx.get_bool_ty();
        }
        this.engine.report(diag.E_UNEXPECTED_TOKEN, "unexpected token", diag.SEV_ERROR)
            .span(this.cur_tok.range);
        this.synchronize();
        return nil;
    }

    pub func advance(): lexer.Token {
        this.prev_tok = this.cur_tok;
        this.cur_tok = this.next_tok;
        this.next_tok = this.lex.next_token().unwrap();
        return this.prev_tok;
    }

    func is_stmt_start(kind: i32): bool {
        return kind == lexer.TOK_LET
            || kind == lexer.TOK_CONST
            || kind == lexer.TOK_FUNC
            || kind == lexer.TOK_RET
            || kind == lexer.TOK_IF
            || kind == lexer.TOK_ELSE
            || kind == lexer.TOK_FOR
            || kind == lexer.TOK_BREAK
            || kind == lexer.TOK_CONT
            || kind == lexer.TOK_STRUCT
            || kind == lexer.TOK_PUB
            || kind == lexer.TOK_IMPL
            || kind == lexer.TOK_TRAIT
            || kind == lexer.TOK_MOD
            || kind == lexer.TOK_IMPORT
            || kind == lexer.TOK_STATIC
            || kind == lexer.TOK_EXTERN;
    }

    func is_synch_tok(kind: i32): bool {
        if this.is_stmt_start(kind) {
            return true;
        }
        if this.check (this.prev_tok, lexer.TOK_SEMI) || this.check (this.prev_tok, lexer.TOK_RBRACE)
            || this.check (this.prev_tok, lexer.TOK_RPAREN) {
            return true;
        }
        return false;
    }

    func synchronize() {
        if this.is_synch_tok(this.cur_tok.kind) {
            return;
        }
        this.advance();
        for !this.is_at_end() {
            if this.is_synch_tok(this.cur_tok.kind) {
                return;
            }
            this.advance();
        }
    }

    func check(kind: i32): bool {
        return this.check(this.cur_tok, kind);
    }

    func check(tok: lexer.Token, kind: i32): bool {
        return tok.kind == kind;
    }

    func check_trailing_semi(stmt: *Stmt, expect: bool): *Stmt {
        if expect {
            if !this.expect_semi() {
                this.engine.report(diag.E_UNEXPECTED_TOKEN, "expected ';'", diag.SEV_ERROR)
                    .span(this.cur_tok.range);
                this.synchronize();
            }
        }
        return stmt;
    }

    func match(kind: i32): bool {
        if this.check(kind) {
            this.advance();
            return true;
        }
        return false;
    }

    func expect_semi(): bool {
        return this.expect_tok(lexer.TOK_SEMI);
    }

    func expect_tok(kind: i32): bool {
        if !this.match(kind) {
            return false;
        }
        return true;
    }

    func is_at_end(): bool {
        return this.cur_tok.kind == lexer.TOK_EOF;
    }
}

pub struct Dumper {
    indent: u32;
}

impl Dumper {
    pub func dump(res: ParseResult) {
        return this.dump(res, 0u32);
    }

    pub func dump(res: ParseResult, start_indent: u32) {
        this.indent = start_indent;
        for let i = 0uz, i < res.count, i += 1 {
            if i != 0uz {
                this.print("\n");
            }
            let node = *(res.nodes + i);
            if Stmt.isa(node) {
                let stmt = Stmt.cast(node);
                this.dump_stmt(stmt);
            }
        }
    }

    func dump_stmt(stmt: *Stmt) {
        if stmt == nil {
            return;
        }
        let kind = stmt.kind();
        if kind == NODE_VAR_DECL {
            this.dump_var_decl(VarDecl.cast(stmt.(*Node)));
        } else if kind == NODE_FUNC_DECL {
            this.dump_func_decl(FuncDecl.cast(stmt.(*Node)));
        } else if kind == NODE_BLOCK_STMT {
            this.dump_block_stmt(BlockStmt.cast(stmt.(*Node)));
        } else if kind == NODE_RET_STMT {
            this.dump_ret_stmt(RetStmt.cast(stmt.(*Node)));
        }
    }

    func dump_var_decl(var_decl: *VarDecl) {
        this.print_with_indent("VarDecl");
        this.print_id(var_decl.(*Stmt));
        this.print(": ");
        if var_decl.is_const {
            this.print("const ");
        } else {
            this.print("let ");
        }
        this.print(var_decl.name);

        if var_decl.ty != nil {
            this.print(": ");
            let ty_str = var_decl.ty.to_string();
            this.print(ty_str.data());
            ty_str.destroy();
        }
        this.print("\n");
        if var_decl.init != nil {
            this.indent += 1;
            this.dump_expr(var_decl.init);
            this.indent -= 1;
        }
    }

    func dump_func_decl(func_decl: *FuncDecl) {
        this.print_with_indent("FuncDecl");
        this.print_id(func_decl.(*Stmt));
        this.print(": ");
        this.print(func_decl.name);

        this.print(" (");
        for let i = 0uz, i < func_decl.args_count, i += 1 {
            if i != 0uz {
                this.print(", ");
            }
            let arg = func_decl.args + i;
            this.print(arg.name);
            this.print(": ");
            let ty_str = arg.ty.to_string();
            this.print(ty_str.data());
            ty_str.destroy();
        }

        this.print("): ");
        let ret_ty_str = func_decl.ret_ty.to_string();
        this.print(ret_ty_str.data());
        ret_ty_str.destroy();
        this.print("\n");
        if func_decl.body != nil {
            this.indent += 1;
            this.dump_stmt(func_decl.body.(*Stmt));
            this.indent -= 1;
        }
    }

    func dump_block_stmt(block: *BlockStmt) {
        this.print_with_indent("BlockStmt");
        this.print_id(block.(*Stmt));
        this.print(":\n");
        this.indent += 1;
        for let i = 0uz, i < block.stmts_count, i += 1 {
            this.dump_stmt(*(block.stmts + i));
        }
        this.indent -= 1;
    }

    func dump_ret_stmt(ret: *RetStmt) {
        this.print_with_indent("RetStmt");
        this.print_id(ret.(*Stmt));
        this.print(":\n");
        if ret.expr != nil {
            this.indent += 1;
            this.dump_expr(ret.expr);
            this.indent -= 1;
        }
    }

    func dump_expr(expr: *Expr) {
        if expr == nil {
            return;
        }
        let kind = expr.kind();
        if kind == NODE_LIT_EXPR {
            this.dump_lit_expr(LitExpr.cast(expr.(*Node)));
        } else if kind == NODE_BIN_EXPR {
            this.dump_bin_expr(BinExpr.cast(expr.(*Node)));
        } else if kind == NODE_UN_EXPR {
            this.dump_un_expr(UnExpr.cast(expr.(*Node)));
        } else if kind == NODE_VAR_EXPR {
            this.dump_var_expr(VarExpr.cast(expr.(*Node)));
        } else if kind == NODE_CALL_EXPR {
            this.dump_call_expr(CallExpr.cast(expr.(*Node)));
        }
    }

    func dump_lit_expr(lit: *LitExpr) {
        this.print_with_indent("LitExpr");
        this.print_id(lit.(*Expr));
        this.print(": ");
        this.print(lit.val);
        this.print("\n");
    }

    func dump_bin_expr(bin: *BinExpr) {
        this.print_with_indent("BinExpr");
        this.print_id(bin.(*Expr));
        this.print(": ");
        this.print(bin_op_to_str(bin.op));
        this.print("\n");
        if bin.left != nil {
            this.indent += 1;
            this.dump_expr(bin.left);
            this.indent -= 1;
        }
        if bin.right != nil {
            this.indent += 1;
            this.dump_expr(bin.right);
            this.indent -= 1;
        }
    }

    func dump_un_expr(un: *UnExpr) {
        this.print_with_indent("UnExpr");
        this.print_id(un.(*Expr));
        this.print(": ");
        this.print(un_op_to_str(un.op));
        this.print("\n");
        if un.right != nil {
            this.indent += 1;
            this.dump_expr(un.right);
            this.indent -= 1;
        }
    }

    func dump_var_expr(var: *VarExpr) {
        this.print_with_indent("VarExpr");
        this.print_id(var.(*Expr));
        this.print(": ");
        this.print(var.name);
        this.print("\n");
    }

    func dump_call_expr(call: *CallExpr) {
        this.print_with_indent("CallExpr");
        this.print_id(call.(*Expr));
        this.print(":\n");

        this.indent += 1;
        this.print_with_indent("[callee]:\n");
        this.indent += 1;
        this.dump_expr(call.callee);
        this.indent -= 1;

        this.print_with_indent("[args]:\n");
        this.indent += 1;
        for let i = 0uz, i < call.args_count, i += 1 {
            this.dump_expr(*(call.args + i));
        }
        this.indent -= 2;
    }

    func print_indent() {
        const indent_width = 2u32;
        for let i = 0u32, i < this.indent * indent_width, i += 1 {
            io.print(" ");
        }
    }

    func print(msg: *u8) {
        io.print(msg);
    }

    func print(msg: std.StringView) {
        io.print(msg);
    }

    func print(n: i64) {
        io.print(n);
    }

    func print(n: u64) {
        io.print(n);
    }

    func print_with_indent(msg: *u8) {
        this.print_indent();
        this.print(msg);
    }

    func print_with_indent(msg: std.StringView) {
        this.print_indent();
        this.print(msg);
    }

    func print_with_indent(n: i64) {
        this.print_indent();
        this.print(n);
    }

    func print_with_indent(n: u64) {
        this.print_indent();
        this.print(n);
    }

    func print_id(stmt: *Stmt) {
        this.print("(");
        this.print(stmt.id().(u64));
        this.print(")");
    }

    func print_id(expr: *Expr) {
        this.print("(");
        this.print(expr.id().(u64));
        this.print(")");
    }
}
