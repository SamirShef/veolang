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
pub const NODE_UN_EXPR    = 103;
pub const NODE_VAR_EXPR   = 104;
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

    pub func set_range(range: basic.Span) {
        this.range = range;
    }

    pub func set_range(start: smloc.SMLoc, end: smloc.SMLoc) {
        this.set_range(basic.Span.new(start, end));
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

    pub func kind(): i32 {
        return this.base.kind();
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

    pub func kind(): i32 {
        return this.base.kind();
    }

    pub func range(): basic.Span {
        return this.base.range();
    }

    pub func set_range(range: basic.Span) {
        this.base.set_range(range);
    }

    pub func set_range(start: smloc.SMLoc, end: smloc.SMLoc) {
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

    pub func alloc_var_decl(range: basic.Span, name: std.StringView, is_const: bool,
                            type: *types.Type, init: *Expr): *VarDecl {
        let mem_ptr = this.alloc.alloc(@size_of(VarDecl));
        let node    = mem_ptr.(*VarDecl);

        node.base     = Stmt.new(NODE_VAR_DECL, range);
        node.name     = name;
        node.is_const = is_const;
        node.ty       = type;
        node.init     = init;

        return node;
    }

    pub func alloc_lit_expr(range: basic.Span, val: std.StringView, tok_kind: i32): *LitExpr {
        let mem_ptr = this.alloc.alloc(@size_of(LitExpr));
        let node    = mem_ptr.(*LitExpr);

        node.base     = Expr.new(NODE_LIT_EXPR, range);
        node.val      = val;
        node.tok_kind = tok_kind;

        return node;
    }

    pub func alloc_bin_expr(range: basic.Span, op: i32, left: *Expr, right: *Expr): *BinExpr {
        let mem_ptr = this.alloc.alloc(@size_of(BinExpr));
        let node    = mem_ptr.(*BinExpr);

        node.base  = Expr.new(NODE_BIN_EXPR, range);
        node.op    = op;
        node.left  = left;
        node.right = right;

        return node;
    }

    pub func alloc_un_expr(range: basic.Span, op: i32, right: *Expr): *UnExpr {
        let mem_ptr = this.alloc.alloc(@size_of(UnExpr));
        let node    = mem_ptr.(*UnExpr);

        node.base  = Expr.new(NODE_UN_EXPR, range);
        node.op    = op;
        node.right = right;

        return node;
    }

    pub func alloc_var_expr(range: basic.Span, name: std.StringView): *VarExpr {
        let mem_ptr = this.alloc.alloc(@size_of(VarExpr));
        let node    = mem_ptr.(*VarExpr);

        node.base     = Expr.new(NODE_VAR_EXPR, range);
        node.name     = name;

        return node;
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

func tok_is_assignment(kind: i32): bool {
    return kind == lexer.TOK_EQ || kind == lexer.TOK_PLUS_EQ || kind == lexer.TOK_MINUS_EQ
        || kind == lexer.TOK_STAR_EQ || kind == lexer.TOK_SLASH_EQ || kind == lexer.TOK_PERCENT_EQ
        || kind == lexer.TOK_AMP_EQ || kind == lexer.TOK_PIPE_EQ || kind == lexer.TOK_CARET_EQ;
}

func tok_precedence(kind: i32): i32 {
    if kind == lexer.TOK_DOT {
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
    } else if kind == lexer.TOK_PLUS || kind == lexer.TOK_MINUS {
        return PREC_SUM;
    } else if kind == lexer.TOK_STAR || kind == lexer.TOK_SLASH || kind == lexer.TOK_PERCENT {
        return PREC_PRODUCT;
    }
    return PREC_LOWEST;
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
                this.synchronize();
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
        return this.parse_stmt(true);
    }

    func parse_stmt(expect_semi: bool): *Stmt {
        let kind = this.cur_tok.kind;
        if kind == lexer.TOK_LET || kind == lexer.TOK_CONST {
            return this.check_trailing_semi(this.parse_var_decl(), expect_semi);
        }
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
            let op    = tok_to_bin_op(this.advance().kind);
            let right = this.parse_expr(prec, allow_struct);
            let end   = this.last_tok.range.end;
            let range = basic.Span.new(start, end);
            left      = this.ast_ctx.alloc_bin_expr(range, op, left, right).(*Expr);
        }
        return left;
    }

    func parse_primary_expr(): *Expr {
        return this.parse_primary_expr(true);
    }

    func parse_primary_expr(allow_struct: bool): *Expr {
        let tok  = this.advance();
        let kind = tok.kind;
        if kind == lexer.TOK_BOOL_LIT || kind == lexer.TOK_CHAR_LIT || kind == lexer.TOK_I8_LIT
            || kind == lexer.TOK_I16_LIT || kind == lexer.TOK_I32_LIT || kind == lexer.TOK_I64_LIT
            || kind == lexer.TOK_ISIZE_LIT || kind == lexer.TOK_U8_LIT || kind == lexer.TOK_U16_LIT
            || kind == lexer.TOK_U32_LIT || kind == lexer.TOK_U64_LIT || kind == lexer.TOK_USIZE_LIT
            || kind == lexer.TOK_F32_LIT || kind == lexer.TOK_F64_LIT
            || kind == lexer.TOK_INT_LIT || kind == lexer.TOK_STR_LIT {
            return this.ast_ctx.alloc_lit_expr(tok.range, tok.val, kind).(*Node);
        }
        if kind == lexer.TOK_ID {
            return this.ast_ctx.alloc_var_expr(tok.range, tok.val).(*Node);
        }
        if kind == lexer.TOK_LPAREN {
            let expr = this.parse_expr();
            if expr != nil {
                expr.set_range(tok.range.start, expr.range().end);
            }
            if this.expect_tok(lexer.TOK_RPAREN) && expr != nil {
                expr.set_range(expr.range().start, this.last_tok.range.end);
            }
            return expr;
        }
        if kind == lexer.TOK_MINUS || kind == lexer.TOK_BANG || kind == lexer.TOK_TILDE {
            let expr = this.parse_expr(PREC_UNARY, allow_struct);
            let range = basic.Span.new(tok.range.start, this.last_tok.range.end);
            return this.ast_ctx.alloc_un_expr(range, tok_to_un_op(kind), expr).(*Expr);
        }
        this.synchronize();
        return nil;
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
        }
        this.synchronize();
        return nil;
    }

    pub func advance(): lexer.Token {
        this.last_tok = this.cur_tok;
        this.cur_tok = this.next_tok;
        this.next_tok = this.lex.next_token().unwrap();
        return this.last_tok;
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
        if this.check (this.last_tok, lexer.TOK_SEMI) || this.check (this.last_tok, lexer.TOK_RBRACE)
            || this.check (this.last_tok, lexer.TOK_RPAREN) {
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
            this.expect_semi();
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
            this.synchronize();
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
        }
    }

    func dump_var_decl(var_decl: *VarDecl) {
        this.print_with_indent("VarDecl: ");
        if var_decl.is_const {
            this.print("const ");
        } else {
            this.print("let ");
        }
        this.print(var_decl.name);
        if var_decl.ty != nil {
            this.print(": ");
            let alloc: mem.MallocAllocator;
            let ty_str = var_decl.ty.to_string(alloc);
            this.print(ty_str.data());
            ty_str.destroy(alloc);
        }
        this.print("\n");
        if var_decl.init != nil {
            this.indent += 1;
            this.dump_expr(var_decl.init);
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
        }
    }

    func dump_lit_expr(lit: *LitExpr) {
        this.print_with_indent("LitExpr: ");
        this.print(lit.val);
        this.print("\n");
    }

    func dump_bin_expr(bin: *BinExpr) {
        this.print_with_indent("BinExpr: ");
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
        this.print_with_indent("UnExpr: ");
        this.print(un_op_to_str(un.op));
        this.print("\n");
        if un.right != nil {
            this.indent += 1;
            this.dump_expr(un.right);
            this.indent -= 1;
        }
    }

    func dump_var_expr(var: *VarExpr) {
        this.print_with_indent("VarExpr: ");
        this.print(var.name);
        this.print("\n");
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

    func print_with_indent(msg: *u8) {
        this.print_indent();
        this.print(msg);
    }

    func print_with_indent(msg: std.StringView) {
        this.print_indent();
        this.print(msg);
    }
}
