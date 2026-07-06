import std.math;
import std.mem;
import std.sys;
import std;
import types;
import llvm.smloc;
import basic;
import hir;
import std.io;
import llvm.source_mgr;
import lexer;
import ast;
import symbols;

pub struct Scope {
    pub parent: *Scope;
    pub entries: **symbols.Symbol;
    pub count: usize;
    pub cap: usize;
}

impl Scope {
    pub static func new(alloc: mem.Allocator, parent: *Scope): *Scope {
        let ptr: *symbols.Symbol;
        let scope     = alloc.alloc(@size_of(Scope)).(*Scope);
        scope.parent  = parent;
        scope.cap     = 16uz;
        scope.entries = alloc.alloc(@size_of(ptr) * scope.cap).(**symbols.Symbol);
        scope.count   = 0uz;
        return scope;
    }

    pub func lookup_local(name: std.StringView): *symbols.Symbol {
        for let i = 0uz, i < this.count, i += 1 {
            let sym = *(this.entries + i);
            if sym.name().compare_to(name) == 0 {
                return sym;
            }
        }
        return nil;
    }

    pub func lookup_recursive(name: std.StringView): *symbols.Symbol {
        let current = this;
        for current != nil {
            let sym = current.lookup_local(name);
            if sym != nil {
                return sym;
            }
            current = current.parent;
        }
        return nil;
    }

    pub func insert(alloc: mem.Allocator, sym: *symbols.Symbol) {
        let ptr: *symbols.Symbol;
        if this.count >= this.cap {
            let old_cap = this.cap;
            this.cap = math.max(this.cap * 2uz, this.cap + 1uz);
            this.entries = alloc.realloc(
                this.entries.(*u8),
                old_cap,
                @size_of(ptr) * this.cap
            ).(**symbols.Symbol);
        }
        *(this.entries + this.count) = sym;
        this.count += 1;
    }

    pub func destroy(alloc: mem.Allocator) {
        alloc.destroy(this.entries.(*u8));
        alloc.destroy(this.(*u8));
    }
}

pub struct Sema {
    alloc: mem.MallocAllocator;
    current_scope: *Scope;
    builder: *hir.Builder;
    ty_ctx: *types.Context;
    sym_table: *symbols.SymbolTable;
}

struct ExprResult {
    pub val: basic.OptionValue;
    pub node: *hir.Node;
}

impl ExprResult {
    pub static func new(val: basic.Value, node: *hir.Node): ExprResult {
        return ExprResult {
            val: basic.OptionValue.some(val),
            node: node
        };
    }

    pub static func new(val: basic.Value): ExprResult {
        return ExprResult.new(val, nil.(*hir.Node));
    }

    pub static func invalid(): ExprResult {
        return ExprResult {
            val: basic.OptionValue.none(),
            node: nil
        };
    }
}

impl Sema {
    pub static func new(alloc: mem.MallocAllocator, builder: *hir.Builder,
                        ty_ctx: *types.Context, sym_table: *symbols.SymbolTable): Sema {
        return Sema {
            alloc: alloc,
            current_scope: nil,
            builder: builder,
            ty_ctx: ty_ctx,
            sym_table: sym_table
        };
    }

    func enter_scope() {
        this.current_scope = Scope.new(this.alloc, this.current_scope);
    }

    func exit_scope() {
        if this.current_scope != nil {
            let old = this.current_scope;
            this.current_scope = this.current_scope.parent;
            old.destroy(this.alloc);
        }
    }

    pub func analyze(alloc: mem.Allocator, res: ast.ParseResult) {
        this.enter_scope();
        for let i = 0uz, i < res.count, i += 1 {
            let node = *(res.nodes + i);
            if ast.Stmt.isa(node) {
                this.analyze_stmt(ast.Stmt.cast(node));
            }
        }
        this.exit_scope();
    }

    func analyze_stmt(stmt: *ast.Stmt) {
        if stmt == nil {
            return;
        }

        let kind = stmt.kind();
        if kind == ast.NODE_VAR_DECL {
            this.analyze_var_decl(ast.VarDecl.cast(stmt.(*ast.Node)));
        } else {
            std.panic("Unsupported statement kind");
        }
    }

    func analyze_var_decl(var_decl: *ast.VarDecl) {
        let existing = this.current_scope.lookup_local(var_decl.name);
        if existing != nil {
            std.panic("Redefinition of variable");
            return;
        }

        let init: ExprResult;
        let ty = var_decl.ty;
        if var_decl.init != nil {
            init = this.analyze_expr(var_decl.init, ty);
        }
        if ty == nil && init.val.has_val() {
            ty = init.val.unwrap().ty;
        }

        let def_id       = var_decl.id.unwrap();
        let var_sym      = this.alloc.alloc(@size_of(symbols.VarSymbol)).(*symbols.VarSymbol);
        var_sym.base     = symbols.Symbol.new(symbols.SYM_VAR, var_decl.name, def_id);
        var_sym.is_const = false;
        var_sym.val      = init.val;
        this.sym_table.insert(this.alloc, var_sym.(*symbols.Symbol));
        this.current_scope.insert(this.alloc, var_sym.(*symbols.Symbol));
        this.builder.create_variable(this.alloc, def_id, var_decl.name, ty, init.node);
    }

    func analyze_expr(expr: *ast.Expr, expected_ty: *types.Type): ExprResult {
        if expr == nil {
            return ExprResult.invalid();
        }

        let kind = expr.kind();
        if kind == ast.NODE_VAR_EXPR {
            let var_expr = ast.VarExpr.cast(expr.(*ast.Node));
            return this.analyze_var_expr(var_expr, expected_ty);
        } else if kind == ast.NODE_BIN_EXPR {
            std.panic("Unimplemented NODE_BIN_EXPR");
            return ExprResult.invalid();
        } else if kind == ast.NODE_UN_EXPR {
            std.panic("Unimplemented NODE_UN_EXPR");
            return ExprResult.invalid();
        } else if kind == ast.NODE_LIT_EXPR {
            let lit_expr = ast.LitExpr.cast(expr.(*ast.Node));
            return this.analyze_lit_expr(lit_expr, expected_ty);
        } else {
            std.panic("Unsupported expression kind");
            return ExprResult.invalid();
        }
    }

    func analyze_lit_expr(lit: *ast.LitExpr, expected_ty: *types.Type): ExprResult {
        let kind = lit.tok_kind;
        if kind == lexer.TOK_I32_LIT {
            let text   = lit.val;
            let len    = text.len();
            let data   = text.data();
            let is_neg = false;
            let idx    = 0uz;

            let first_char = *data;
            if first_char == '-'.(u8) {
                is_neg = true;
                idx += 1;
            }

            let accum = 0u64;

            for idx < len {
                let ch = *(data + idx);
                if ch >= '0'.(u8) && ch <= '9'.(u8) {
                    let digit = (ch - '0'.(u8)).(u64);
                    accum     = accum * 10u64 + digit;
                }
                idx += 1uz;
            }

            let final_i32 = 0i32;
            if is_neg {
                final_i32 = -(accum.(i32));
            } else {
                final_i32 = accum.(i32);
            }
            let val_as_u64 = final_i32.(u64);
            let ty = this.ty_ctx.get_int_ty(32u32, false);
            if expected_ty != nil {
                ty = expected_ty;
            }

            let val     = basic.Value.new(basic.VAL_CONST, val_as_u64, ty);
            let hir_lit = this.builder.create_literal(val);

            return ExprResult.new(val, hir_lit.(*hir.Node));
        } else {
            std.panic("Unimplemented literal type kind");
            return ExprResult.invalid();
        }
    }

    func analyze_var_expr(var_expr: *ast.VarExpr, expected_ty: *types.Type): ExprResult {
        let resolved = this.current_scope.lookup_recursive(var_expr.name);
        if resolved == nil {
            resolved = this.sym_table.lookup_by_name(var_expr.name);
        }
        if resolved == nil {
            std.panic("Use of undeclared variable");
            return ExprResult.invalid();
        }
        if !symbols.VarSymbol.isa(resolved) {
            std.panic("Symbol is not a variable");
            return ExprResult.invalid();
        }
        let var_sym = symbols.VarSymbol.cast(resolved);
        let val     = var_sym.is_const ? var_sym.val.unwrap()
                                       : basic.Value.new(basic.VAL_UNKNOWN, var_sym.ty);
        let node    = this.builder.create_load(var_sym.id(), var_sym.ty);
        return ExprResult.new(val, node.(*hir.Node));
    }
}
