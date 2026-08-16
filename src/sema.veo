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

// HashMaps

const MAP_STATE_EMPTY     = 0;
const MAP_STATE_OCCUPIED  = 1;
const MAP_STATE_TOMBSTONE = 2;

func hash_string(key: std.StringView): u32 {
    let hash = 2166136261u32;
    for let i = 0uz, i < key.len(), i += 1 {
        hash ^= *(key.data() + i);
        hash *= 16777619u32;
    }
    return hash;
}

struct HashMapU32DefIdEntry {
    pub key: u32;
    pub val: basic.DefId;
    pub state: i32;
}

pub struct HashMapU32DefId {
    buckets: *HashMapU32DefIdEntry;
    len: usize;
    cap: usize;
    tompstones_count: usize;
}

impl HashMapU32DefId {
    pub static func new(): HashMapU32DefId {
        let cap = 8uz;
        let buckets = sys.malloc(cap * @size_of(HashMapU32DefIdEntry))
            .(*HashMapU32DefIdEntry);
        return HashMapU32DefId { buckets: buckets, len: 0uz, cap: cap, tompstones_count: 0uz };
    }

    func resize(new_cap: usize) {
        let old_buckets = this.buckets;
        let old_cap = this.cap;

        let buckets = sys.malloc(new_cap * @size_of(HashMapU32DefIdEntry))
            .(*HashMapU32DefIdEntry);
        this.cap = new_cap;
        this.tompstones_count = 0;

        let mask = new_cap - 1uz;
        for let i = 0uz, i < old_cap, i += 1 {
            if (old_buckets + i).state != MAP_STATE_OCCUPIED {
                continue;
            }

            let key = (old_buckets + i).key;
            let hash = hash_string(std.StringView.from((&key).(*u8), @size_of(key)));
            let index = hash.(usize) & mask;
            for (buckets + index).state != MAP_STATE_EMPTY {
                index = (index + 1uz) & mask;
            }
            (buckets + index).key = key;
            (buckets + index).val = (old_buckets + i).val;
            (buckets + index).state = MAP_STATE_OCCUPIED;
        }
        this.buckets = buckets;
        sys.free(old_buckets.(*u8));
    }

    pub func insert(key: u32, val: basic.DefId): bool {
        if (this.len + this.tompstones_count) * 10uz >= this.cap * 7uz {
            // resize
            if this.tompstones_count > this.len {
                this.resize(this.cap);
            } else {
                this.resize(this.cap * 2uz);
            }
        }

        let hash = hash_string(std.StringView.from((&key).(*u8), @size_of(key)));
        let mask = this.cap - 1uz;
        let index = hash.(usize) & mask;
        let first_tompstone_idx = (-1).(usize);

        for {
            let entry = this.buckets + index;

            if entry.state == MAP_STATE_EMPTY {
                if first_tompstone_idx != (-1).(usize) {
                    index = first_tompstone_idx;
                    entry = this.buckets + index;
                    this.tompstones_count -= 1;
                }
                entry.key = key;
                entry.val = val;
                entry.state = MAP_STATE_OCCUPIED;
                this.len += 1;
                return true;
            }

            if entry.state == MAP_STATE_OCCUPIED {
                if entry.key == key {
                    entry.val = val;
                    return false;
                }
            } else if entry.state == MAP_STATE_TOMBSTONE {
                if first_tompstone_idx == (-1).(usize) {
                    first_tompstone_idx = index;
                }
            }

            index = (index + 1uz) & mask;
        }
    }

    pub func get(key: u32): basic.OptionDefId {
        let hash = hash_string(std.StringView.from((&key).(*u8), @size_of(key)));
        let mask = this.cap - 1uz;
        let index = hash.(usize) & mask;

        for {
            let entry = this.buckets + index;
            if entry.state == MAP_STATE_EMPTY {
                return basic.OptionDefId.none();
            }

            if entry.state == MAP_STATE_OCCUPIED && entry.key == key {
                return basic.OptionDefId.some(entry.val);
            }

            index = (index + 1uz) & mask;
        }

        return basic.OptionDefId.none();
    }

    pub func len(): usize {
        return this.len;
    }

    pub func destroy() {
        sys.free(this.buckets.(*u8));
        this.buckets = nil;
        this.len = 0;
        this.cap = 0;
        this.tompstones_count = 0;
    }
}

struct HashMapU32TypeEntry {
    pub key: u32;
    pub val: *types.Type;
    pub state: i32;
}

pub struct HashMapU32Type {
    buckets: *HashMapU32TypeEntry;
    len: usize;
    cap: usize;
    tompstones_count: usize;
}

impl HashMapU32Type {
    pub static func new(): HashMapU32Type {
        let cap = 8uz;
        let buckets = sys.malloc(cap * @size_of(HashMapU32TypeEntry))
            .(*HashMapU32TypeEntry);
        return HashMapU32Type { buckets: buckets, len: 0uz, cap: cap, tompstones_count: 0uz };
    }

    func resize(new_cap: usize) {
        let old_buckets = this.buckets;
        let old_cap = this.cap;

        let buckets = sys.malloc(new_cap * @size_of(HashMapU32TypeEntry))
            .(*HashMapU32TypeEntry);
        this.cap = new_cap;
        this.tompstones_count = 0;

        let mask = new_cap - 1uz;
        for let i = 0uz, i < old_cap, i += 1 {
            if (old_buckets + i).state != MAP_STATE_OCCUPIED {
                continue;
            }

            let key = (old_buckets + i).key;
            let hash = hash_string(std.StringView.from((&key).(*u8), @size_of(key)));
            let index = hash.(usize) & mask;
            for (buckets + index).state != MAP_STATE_EMPTY {
                index = (index + 1uz) & mask;
            }
            (buckets + index).key = key;
            (buckets + index).val = (old_buckets + i).val;
            (buckets + index).state = MAP_STATE_OCCUPIED;
        }
        this.buckets = buckets;
        sys.free(old_buckets.(*u8));
    }

    pub func insert(key: u32, val: *types.Type): bool {
        if (this.len + this.tompstones_count) * 10uz >= this.cap * 7uz {
            // resize
            if this.tompstones_count > this.len {
                this.resize(this.cap);
            } else {
                this.resize(this.cap * 2uz);
            }
        }

        let hash = hash_string(std.StringView.from((&key).(*u8), @size_of(key)));
        let mask = this.cap - 1uz;
        let index = hash.(usize) & mask;
        let first_tompstone_idx = (-1).(usize);

        for {
            let entry = this.buckets + index;

            if entry.state == MAP_STATE_EMPTY {
                if first_tompstone_idx != (-1).(usize) {
                    index = first_tompstone_idx;
                    entry = this.buckets + index;
                    this.tompstones_count -= 1;
                }
                entry.key = key;
                entry.val = val;
                entry.state = MAP_STATE_OCCUPIED;
                this.len += 1;
                return true;
            }

            if entry.state == MAP_STATE_OCCUPIED {
                if entry.key == key {
                    entry.val = val;
                    return false;
                }
            } else if entry.state == MAP_STATE_TOMBSTONE {
                if first_tompstone_idx == (-1).(usize) {
                    first_tompstone_idx = index;
                }
            }

            index = (index + 1uz) & mask;
        }
    }

    pub func get(key: u32): *types.Type {
        let hash = hash_string(std.StringView.from((&key).(*u8), @size_of(key)));
        let mask = this.cap - 1uz;
        let index = hash.(usize) & mask;

        for {
            let entry = this.buckets + index;
            if entry.state == MAP_STATE_EMPTY {
                return nil;
            }

            if entry.state == MAP_STATE_OCCUPIED && entry.key == key {
                return entry.val;
            }

            index = (index + 1uz) & mask;
        }

        return nil;
    }

    pub func len(): usize {
        return this.len;
    }

    pub func destroy() {
        sys.free(this.buckets.(*u8));
        this.buckets = nil;
        this.len = 0;
        this.cap = 0;
        this.tompstones_count = 0;
    }
}

struct HashMapDefIdTypeEntry {
    pub key: basic.DefId;
    pub val: *types.Type;
    pub state: i32;
}

pub struct HashMapDefIdType {
    buckets: *HashMapDefIdTypeEntry;
    len: usize;
    cap: usize;
    tompstones_count: usize;
}

impl HashMapDefIdType {
    pub static func new(): HashMapDefIdType {
        let cap = 8uz;
        let buckets = sys.malloc(cap * @size_of(HashMapDefIdTypeEntry))
            .(*HashMapDefIdTypeEntry);
        return HashMapDefIdType { buckets: buckets, len: 0uz, cap: cap, tompstones_count: 0uz };
    }

    func resize(new_cap: usize) {
        let old_buckets = this.buckets;
        let old_cap = this.cap;

        let buckets = sys.malloc(new_cap * @size_of(HashMapDefIdTypeEntry))
            .(*HashMapDefIdTypeEntry);
        this.cap = new_cap;
        this.tompstones_count = 0;

        let mask = new_cap - 1uz;
        for let i = 0uz, i < old_cap, i += 1 {
            if (old_buckets + i).state != MAP_STATE_OCCUPIED {
                continue;
            }

            let key = (old_buckets + i).key;
            let hash = hash_string(std.StringView.from((&key).(*u8), @size_of(key)));
            let index = hash.(usize) & mask;
            for (buckets + index).state != MAP_STATE_EMPTY {
                index = (index + 1uz) & mask;
            }
            (buckets + index).key = key;
            (buckets + index).val = (old_buckets + i).val;
            (buckets + index).state = MAP_STATE_OCCUPIED;
        }
        this.buckets = buckets;
        sys.free(old_buckets.(*u8));
    }

    pub func insert(key: basic.DefId, val: *types.Type): bool {
        if (this.len + this.tompstones_count) * 10uz >= this.cap * 7uz {
            // resize
            if this.tompstones_count > this.len {
                this.resize(this.cap);
            } else {
                this.resize(this.cap * 2uz);
            }
        }

        let hash = hash_string(std.StringView.from((&key).(*u8), @size_of(key)));
        let mask = this.cap - 1uz;
        let index = hash.(usize) & mask;
        let first_tompstone_idx = (-1).(usize);

        for {
            let entry = this.buckets + index;

            if entry.state == MAP_STATE_EMPTY {
                if first_tompstone_idx != (-1).(usize) {
                    index = first_tompstone_idx;
                    entry = this.buckets + index;
                    this.tompstones_count -= 1;
                }
                entry.key = key;
                entry.val = val;
                entry.state = MAP_STATE_OCCUPIED;
                this.len += 1;
                return true;
            }

            if entry.state == MAP_STATE_OCCUPIED {
                if entry.key.equals(key) {
                    entry.val = val;
                    return false;
                }
            } else if entry.state == MAP_STATE_TOMBSTONE {
                if first_tompstone_idx == (-1).(usize) {
                    first_tompstone_idx = index;
                }
            }

            index = (index + 1uz) & mask;
        }
    }

    pub func get(key: basic.DefId): *types.Type {
        let hash = hash_string(std.StringView.from((&key).(*u8), @size_of(key)));
        let mask = this.cap - 1uz;
        let index = hash.(usize) & mask;

        for {
            let entry = this.buckets + index;
            if entry.state == MAP_STATE_EMPTY {
                return nil;
            }

            if entry.state == MAP_STATE_OCCUPIED && entry.key.equals(key) {
                return entry.val;
            }

            index = (index + 1uz) & mask;
        }

        return nil;
    }

    pub func len(): usize {
        return this.len;
    }

    pub func destroy() {
        sys.free(this.buckets.(*u8));
        this.buckets = nil;
        this.len = 0;
        this.cap = 0;
        this.tompstones_count = 0;
    }
}

// HashMaps

pub struct Scope {
    pub parent: *Scope;
    pub entries: **symbols.Symbol;
    pub count: usize;
    pub cap: usize;
}

impl Scope {
    pub static func new(parent: *Scope): *Scope {
        let ptr: *symbols.Symbol;
        let scope     = sys.malloc(@size_of(Scope)).(*Scope);
        scope.parent  = parent;
        scope.cap     = 16uz;
        scope.entries = sys.malloc(@size_of(ptr) * scope.cap).(**symbols.Symbol);
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

    pub func insert(sym: *symbols.Symbol) {
        let ptr: *symbols.Symbol;
        if this.count >= this.cap {
            let old_cap = this.cap;
            this.cap = math.max(this.cap * 2uz, this.cap + 1uz);
            this.entries = sys.realloc(
                this.entries.(*u8),
                @size_of(ptr) * this.cap
            ).(**symbols.Symbol);
        }
        *(this.entries + this.count) = sym;
        this.count += 1;
    }

    pub func destroy() {
        sys.free(this.entries.(*u8));
        sys.free(this.(*u8));
    }
}

pub struct Context {
    resolutions: HashMapU32DefId;
    def_types: HashMapDefIdType;
    node_types: HashMapU32Type;
    next_def_id: u32;
}

impl Context {
    pub static func new(): Context {
        return Context {
            next_def_id: 0
        };
    }
}

pub struct Sema {
    current_scope: *Scope;
    builder: *hir.Builder;
    ty_ctx: *types.Context;
    sym_table: *symbols.SymbolTable;
    ctx: *Context;
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
    pub static func new(builder: *hir.Builder, ty_ctx: *types.Context,
                        sym_table: *symbols.SymbolTable, ctx: *Context): Sema {
        return Sema {
            current_scope: nil,
            builder: builder,
            ty_ctx: ty_ctx,
            sym_table: sym_table,
            ctx: ctx
        };
    }

    func enter_scope() {
        this.current_scope = Scope.new(this.current_scope);
    }

    func exit_scope() {
        if this.current_scope != nil {
            let old = this.current_scope;
            this.current_scope = this.current_scope.parent;
            old.destroy();
        }
    }

    pub func analyze(res: ast.ParseResult) {
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

        // let def_id       = var_decl.id.unwrap();
        // let var_sym      = sys.malloc(@size_of(symbols.VarSymbol)).(*symbols.VarSymbol);
        // var_sym.base     = symbols.Symbol.new(symbols.SYM_VAR, var_decl.name, def_id);
        // var_sym.is_const = false;
        // var_sym.val      = init.val;
        // this.sym_table.insert(var_sym.(*symbols.Symbol));
        // this.current_scope.insert(var_sym.(*symbols.Symbol));
        // this.builder.create_variable(def_id, var_decl.name, ty, init.node);
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
            let bin_expr = ast.BinExpr.cast(expr.(*ast.Node));
            return this.analyze_bin_expr(bin_expr, expected_ty);
        } else if kind == ast.NODE_UN_EXPR {
            let un_expr = ast.UnExpr.cast(expr.(*ast.Node));
            return this.analyze_un_expr(un_expr, expected_ty);
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
        let text = lit.val;
        let val_as_u64: u64;
        let ty: *types.Type;
        if kind >= lexer.TOK_I8_LIT && kind <= lexer.TOK_I64_LIT {
            let is_neg = false;
            let accum  = str_to_u64(text, &is_neg);

            val_as_u64 = signed_int_to_u64(accum, is_neg);
            ty         = this.tok_to_ty(kind);
            if !this.can_fit(val_as_u64, ty) {
                std.panic("Value out of range for some signed integer type");
            }
            // TODO: add implicit cast to expected_ty
        } else if kind >= lexer.TOK_U8_LIT && kind <= lexer.TOK_U64_LIT {
            let is_neg = false;
            let accum  = str_to_u64(text, &is_neg);
            if is_neg {
                std.panic("Unsigned integer cannot be negative");
            }

            val_as_u64 = accum;
            ty         = this.tok_to_ty(kind);
            if !this.can_fit(val_as_u64, ty) {
                std.panic("Value out of range for some unsigned integer type");
            }
            // TODO: add implicit cast to expected_ty
        } else if kind == lexer.TOK_INT_LIT {
            let is_neg = false;
            let accum  = str_to_u64(text, &is_neg);

            val_as_u64 = signed_int_to_u64(accum, is_neg);
            ty         = expected_ty;
            if ty == nil {
                ty = this.ty_ctx.get_int_ty(32u32, false);
            }
            if !this.can_fit(val_as_u64, ty) {
                std.panic("Value out of range for some integer type");
            }
        } else {
            std.panic("Unimplemented literal type kind");
            return ExprResult.invalid();
        }

        let val     = basic.Value.new(basic.VAL_CONST, val_as_u64, ty);
        let hir_lit = this.builder.create_literal(val);
        return ExprResult.new(val, hir_lit.(*hir.Node));
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
        let val     = basic.Value.new(basic.VAL_UNKNOWN, var_sym.ty);
        let node    = this.builder.create_load(var_sym.id(), var_sym.ty);
        return ExprResult.new(val, node.(*hir.Node));
    }

    func analyze_bin_expr(bin_expr: *ast.BinExpr, expected_ty: *types.Type): ExprResult {
        let left = this.analyze_expr(bin_expr.left, nil.(*types.Type));
        if !left.val.has_val() {
            return ExprResult.invalid();
        }
        let common_ty = left.val.unwrap().ty;
        let right     = this.analyze_expr(bin_expr.right, common_ty);
        if !right.val.has_val() {
            return ExprResult.invalid();
        }
        // TODO: add inference common type and cast left and right operands to common type

        let res_ty = this.res_ty_by_bin_op(bin_expr.op, common_ty);
        let val    = basic.Value.new(basic.VAL_UNKNOWN, res_ty);
        let node   = this.builder.create_binary(bin_expr.op, left.node, right.node, common_ty);
        return ExprResult.new(val, node.(*hir.Node));
    }

    func analyze_un_expr(un_expr: *ast.UnExpr, expected_ty: *types.Type): ExprResult {
        let right = this.analyze_expr(un_expr.right, nil.(*types.Type));
        if !right.val.has_val() {
            return ExprResult.invalid();
        }
        // TODO: add cast left operand to expected_ty type

        let common_ty = right.val.unwrap().ty;
        let res_ty    = this.res_ty_by_un_op(un_expr.op, common_ty);
        let val       = basic.Value.new(basic.VAL_UNKNOWN, res_ty);
        let node      = this.builder.create_unary(un_expr.op, right.node, common_ty);
        return ExprResult.new(val, node.(*hir.Node));
    }

    func res_ty_by_bin_op(op: i32, common_ty: *types.Type): *types.Type {
        if op >= ast.BIN_OP_EQ && op <= ast.BIN_OP_LOG_OR {
            return this.ty_ctx.get_bool_ty();
        }
        return common_ty;
    }

    func res_ty_by_un_op(op: i32, common_ty: *types.Type): *types.Type {
        if op == ast.UN_OP_NOT {
            return this.ty_ctx.get_bool_ty();
        }
        return common_ty;
    }

    func tok_to_ty(tok_kind: i32): *types.Type {
        if tok_kind == lexer.TOK_I8_LIT {
            return this.ty_ctx.get_int_ty(8u32, false);
        } else if tok_kind == lexer.TOK_I16_LIT {
            return this.ty_ctx.get_int_ty(16u32, false);
        } else if tok_kind == lexer.TOK_I32_LIT {
            return this.ty_ctx.get_int_ty(32u32, false);
        } else if tok_kind == lexer.TOK_I64_LIT {
            return this.ty_ctx.get_int_ty(64u32, false);
        } else if tok_kind == lexer.TOK_U8_LIT {
            return this.ty_ctx.get_int_ty(8u32, true);
        } else if tok_kind == lexer.TOK_U16_LIT {
            return this.ty_ctx.get_int_ty(16u32, true);
        } else if tok_kind == lexer.TOK_U32_LIT {
            return this.ty_ctx.get_int_ty(32u32, true);
        } else if tok_kind == lexer.TOK_U64_LIT {
            return this.ty_ctx.get_int_ty(64u32, true);
        }
        std.panic("Cannot convert token kind to type");
        return nil;
    }

    func can_fit(val: u64, ty: *types.Type): bool {
        let kind = ty.kind();
        if kind != types.TYPE_INT {
            std.panic("Type must be an integer");
        }

        let int_ty = types.IntType.cast(ty);
        let width = int_ty.width;
        let is_unsigned = int_ty.is_unsigned;

        if is_unsigned {
            if width == 8u32 {
                if val > 0xFFu64 {
                    return false;
                }
            } else if width == 16u32 {
                if val > 0xFFFFu64 {
                    return false;
                }
            } else if width == 32u32 {
                if val > 0xFFFFFFFFu64 {
                    return false;
                }
            } else if width == 64u32 {
                // is not necessary
            } else {
                return false;
            }
        } else {
            if width == 8u32 {
                let sign_mask = 0x80u64;
                let upper_mask = 0xFFFFFFFFFFFFFF80u64;

                if (val & sign_mask) == 0u64 {
                    if (val & upper_mask) != 0u64 {
                        return false;
                    }
                } else {
                    if (val & upper_mask) != upper_mask {
                        return false;
                    }
                }
            } else if width == 16u32 {
                let sign_mask = 0x8000u64;
                let upper_mask = 0xFFFFFFFFFFFF8000u64;

                if (val & sign_mask) == 0u64 {
                    if (val & upper_mask) != 0u64 {
                        return false;
                    }
                } else {
                    if (val & upper_mask) != upper_mask {
                        return false;
                    }
                }
            } else if width == 32u32 {
                let sign_mask = 0x80000000u64;
                let upper_mask = 0xFFFFFFFF80000000u64;

                if (val & sign_mask) == 0u64 {
                    if (val & upper_mask) != 0u64 {
                        return false;
                    }
                } else {
                    if (val & upper_mask) != upper_mask {
                        return false;
                    }
                }
            } else if width == 64u32 {
                // is not necessary
            } else {
                std.panic("Unsupported integer width");
            }
        }

        return true;
    }
}

func str_to_u64(str: std.StringView, is_neg: *bool): u64 {
    let len    = str.len();
    let data   = str.data();
    let idx    = 0uz;

    let first_char = *data;
    if first_char == '-'.(u8) {
        *is_neg = true;
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
    return accum;
}

func signed_int_to_u64(accum: u64, is_neg: bool): u64 {
    if is_neg {
        return (-(accum.(i64))).(u64);
    } else {
        return accum;
    }
}
