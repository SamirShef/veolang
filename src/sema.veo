import std.math;
import std.mem;
import std.sys;
import std;
import types;
import basic;
import hir;
import std.io;
import lexer;
import ast;
import diag;

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
    pub buckets: *HashMapU32DefIdEntry;
    len: usize;
    cap: usize;
    tompstones_count: usize;
}

impl HashMapU32DefId {
    pub static func new(): HashMapU32DefId {
        let cap = 8uz;
        let buckets = sys.malloc(cap * @size_of(HashMapU32DefIdEntry))
            .(*HashMapU32DefIdEntry);
        for let i = 0uz, i < cap, i += 1 {
            (buckets + i).state = MAP_STATE_EMPTY;
        }
        return HashMapU32DefId { buckets: buckets, len: 0uz, cap: cap, tompstones_count: 0uz };
    }

    func resize(new_cap: usize) {
        let old_buckets = this.buckets;
        let old_cap = this.cap;

        let buckets = sys.malloc(new_cap * @size_of(HashMapU32DefIdEntry))
            .(*HashMapU32DefIdEntry);
        for let i = 0uz, i < new_cap, i += 1 {
            (buckets + i).state = MAP_STATE_EMPTY;
        }
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

    pub func cap(): usize {
        return this.cap;
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
        for let i = 0uz, i < cap, i += 1 {
            (buckets + i).state = MAP_STATE_EMPTY;
        }
        return HashMapU32Type { buckets: buckets, len: 0uz, cap: cap, tompstones_count: 0uz };
    }

    func resize(new_cap: usize) {
        let old_buckets = this.buckets;
        let old_cap = this.cap;

        let buckets = sys.malloc(new_cap * @size_of(HashMapU32TypeEntry))
            .(*HashMapU32TypeEntry);
        for let i = 0uz, i < new_cap, i += 1 {
            (buckets + i).state = MAP_STATE_EMPTY;
        }
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
        for let i = 0uz, i < cap, i += 1 {
            (buckets + i).state = MAP_STATE_EMPTY;
        }
        return HashMapDefIdType { buckets: buckets, len: 0uz, cap: cap, tompstones_count: 0uz };
    }

    func resize(new_cap: usize) {
        let old_buckets = this.buckets;
        let old_cap = this.cap;

        let buckets = sys.malloc(new_cap * @size_of(HashMapDefIdTypeEntry))
            .(*HashMapDefIdTypeEntry);
        for let i = 0uz, i < new_cap, i += 1 {
            (buckets + i).state = MAP_STATE_EMPTY;
        }
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

pub const SYM_VALUE = 0;
pub const SYM_FUNC  = 1;
pub const SYM_TYPE  = 2;

pub struct ScopeEntry {
    pub kind: i32;
    pub name: std.StringView;
    pub def_id: basic.DefId;
}

pub struct Scope {
    pub parent: *Scope;
    pub entries: *ScopeEntry;
    pub count: usize;
    pub cap: usize;
}

impl Scope {
    pub static func new(parent: *Scope): *Scope {
        let scope     = sys.malloc(@size_of(Scope)).(*Scope);
        scope.parent  = parent;
        scope.cap     = 16uz;
        scope.entries = sys.malloc(scope.cap * @size_of(ScopeEntry)).(*ScopeEntry);
        scope.count   = 0uz;
        return scope;
    }

    pub func lookup_local(name: std.StringView, kind: i32): basic.OptionDefId {
        for let i = 0uz, i < this.count, i += 1 {
            let entry = this.entries + i;
            if entry.kind == kind && entry.name.compare_to(name) == 0 {
                return basic.OptionDefId.some(entry.def_id);
            }
        }
        return basic.OptionDefId.none();
    }

    pub func lookup_recursive(name: std.StringView, kind: i32): basic.OptionDefId {
        let cur = this;
        for cur != nil {
            let res = cur.lookup_local(name, kind);
            if res.has_val() {
                return res;
            }
            cur = cur.parent;
        }
        return basic.OptionDefId.none();
    }

    pub func insert(name: std.StringView, def_id: basic.DefId, kind: i32) {
        if this.count >= this.cap {
            let old_cap = this.cap;
            this.cap = math.max(this.cap * 2uz, this.cap + 1uz);
            this.entries = sys.realloc(
                this.entries.(*u8),
                this.cap * @size_of(ScopeEntry)
            ).(*ScopeEntry);
        }
        let entry = this.entries + this.count;
        entry.kind = kind;
        entry.name = name;
        entry.def_id = def_id;
        this.count += 1;
    }

    pub func destroy() {
        sys.free(this.entries.(*u8));
        sys.free(this.(*u8));
    }
}

pub struct Context {
    pub resolutions: HashMapU32DefId;
    pub def_types: HashMapDefIdType;
    pub node_types: HashMapU32Type;
    pub ty_ctx: *types.Context;
    next_def_id: u32;
}

impl Context {
    pub static func new(ty_ctx: *types.Context): Context {
        return Context {
            resolutions: HashMapU32DefId.new(),
            def_types: HashMapDefIdType.new(),
            node_types: HashMapU32Type.new(),
            ty_ctx: ty_ctx,
            next_def_id: 0
        };
    }

    pub func next_def_id(): basic.DefId {
        let res = basic.DefId.new(0u32, this.next_def_id);
        this.next_def_id += 1;
        return res;
    }

    pub func dump_resolutions() {
        io.println("--- RESOLUTIONS DUMP ---");
        for let i = 0uz, i < this.resolutions.cap(), i += 1 {
            let entry = this.resolutions.buckets + i;
            if entry.state == MAP_STATE_OCCUPIED {
                io.print("NodeId(");
                sys.__veo_print_u64(1, entry.key.(u64));
                io.print(") -> DefId(");
                sys.__veo_print_u64(1, entry.val.sym_id.(u64));
                io.println(")");
            }
        }
    }
}

pub struct NamesResolver {
    engine: *diag.DiagEngine;
    current_scope: *Scope;
    ctx: *Context;
}

impl NamesResolver {
    pub static func new(engine: *diag.DiagEngine, ctx: *Context): NamesResolver {
        return NamesResolver {
            engine: engine,
            current_scope: nil,
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

    pub func resolve(res: ast.ParseResult) {
        this.enter_scope();
        for let i = 0uz, i < res.count, i += 1 {
            let node = *(res.nodes + i);
            if ast.Stmt.isa(node) {
                this.resolve_stmt(ast.Stmt.cast(node));
            }
        }
        this.exit_scope();
    }

    func resolve_stmt(stmt: *ast.Stmt) {
        if stmt == nil {
            return;
        }

        let kind = stmt.kind();
        if kind == ast.NODE_VAR_DECL {
            this.resolve_var_decl(ast.VarDecl.cast(stmt.(*ast.Node)));
        } else if kind == ast.NODE_FUNC_DECL {
            this.resolve_func_decl(ast.FuncDecl.cast(stmt.(*ast.Node)));
        } else if kind == ast.NODE_BLOCK_STMT {
            this.resolve_block_stmt(ast.BlockStmt.cast(stmt.(*ast.Node)));
        } else if kind == ast.NODE_RET_STMT {
            this.resolve_ret_stmt(ast.RetStmt.cast(stmt.(*ast.Node)));
        }
    }

    func resolve_var_decl(var_decl: *ast.VarDecl) {
        let existing = this.current_scope.lookup_local(var_decl.name, SYM_VALUE);
        if existing.has_val() {
            let msg = std.String.from("redefinition of symbol '");
            msg.append(var_decl.name);
            msg.append("'");
            this.engine.report(diag.E_REDEFINITION, msg, diag.SEV_ERROR)
                .span(var_decl.(*ast.Node).range());
            return;
        }

        if var_decl.ty != nil {
            this.resolve_ty(var_decl.ty);
        }
        this.resolve_expr(var_decl.init);
        let def_id = this.ctx.next_def_id();
        this.ctx.resolutions.insert(var_decl.(*ast.Stmt).id(), def_id);
        this.current_scope.insert(var_decl.name, def_id, SYM_VALUE);
    }

    func resolve_func_decl(func_decl: *ast.FuncDecl) {
        let existing = this.current_scope.lookup_local(func_decl.name, SYM_FUNC);
        if existing.has_val() {
            let msg = std.String.from("redefinition of symbol '");
            msg.append(func_decl.name);
            msg.append("'");
            this.engine.report(diag.E_REDEFINITION, msg, diag.SEV_ERROR)
                .span(func_decl.(*ast.Node).range());
            return;
        }

        let func_def_id = this.ctx.next_def_id();
        this.ctx.resolutions.insert(func_decl.(*ast.Stmt).id(), func_def_id);
        this.current_scope.insert(func_decl.name, func_def_id, SYM_FUNC);

        this.enter_scope();

        for let i = 0uz, i < func_decl.args_count, i += 1 {
            let arg = func_decl.args + i;
            if arg.ty != nil {
                this.resolve_ty(arg.ty);
            }

            let arg_existing = this.current_scope.lookup_local(arg.name, SYM_VALUE);
            if arg_existing.has_val() {
                let msg = std.String.from("redefinition of argument '");
                msg.append(arg.name);
                msg.append("'");
                this.engine.report(diag.E_REDEFINITION, msg, diag.SEV_ERROR)
                    .span(func_decl.(*ast.Node).range());
            } else {
                let arg_def_id = this.ctx.next_def_id();
                this.current_scope.insert(arg.name, arg_def_id, SYM_VALUE);
            }
        }

        let block = func_decl.body;
        for let i = 0uz, i < block.stmts_count, i += 1 {
            this.resolve_stmt(*(block.stmts + i));
        }
        this.exit_scope();
    }

    func resolve_block_stmt(block: *ast.BlockStmt) {
        this.enter_scope();
        for let i = 0uz, i < block.stmts_count, i += 1 {
            this.resolve_stmt(*(block.stmts + i));
        }
        this.exit_scope();
    }

    func resolve_ret_stmt(ret: *ast.RetStmt) {
        this.resolve_expr(ret.expr);
    }

    func resolve_expr(expr: *ast.Expr) {
        if expr == nil {
            return;
        }

        let kind = expr.kind();
        if kind == ast.NODE_VAR_EXPR {
            let var_expr = ast.VarExpr.cast(expr.(*ast.Node));
            return this.resolve_var_expr(var_expr);
        } else if kind == ast.NODE_BIN_EXPR {
            let bin_expr = ast.BinExpr.cast(expr.(*ast.Node));
            return this.resolve_bin_expr(bin_expr);
        } else if kind == ast.NODE_UN_EXPR {
            let un_expr = ast.UnExpr.cast(expr.(*ast.Node));
            return this.resolve_un_expr(un_expr);
        } else if kind == ast.NODE_LIT_EXPR {
            // not necessary
        } else if kind == ast.NODE_CALL_EXPR {
            let call_expr = ast.CallExpr.cast(expr.(*ast.Node));
            return this.resolve_call_expr(call_expr);
        }
    }

    func resolve_var_expr(var_expr: *ast.VarExpr) {
        let resolved = this.current_scope.lookup_recursive(var_expr.name, SYM_VALUE);
        if !resolved.has_val() {
            let msg = std.String.from("undefined name '");
            msg.append(var_expr.name);
            msg.append("'");
            this.engine.report(diag.E_UNDEFINED, msg, diag.SEV_ERROR)
                .span(var_expr.(*ast.Node).range());
            return;
        }
        this.ctx.resolutions.insert(var_expr.(*ast.Expr).id(), resolved.unwrap());
    }

    func resolve_bin_expr(bin_expr: *ast.BinExpr) {
        this.resolve_expr(bin_expr.left);
        this.resolve_expr(bin_expr.right);
    }

    func resolve_un_expr(un_expr: *ast.UnExpr) {
        this.resolve_expr(un_expr.right);
    }

    func resolve_call_expr(call_expr: *ast.CallExpr) {
        if call_expr.callee != nil {
            if call_expr.callee.kind() == ast.NODE_VAR_EXPR {
                let var_expr = ast.VarExpr.cast(call_expr.callee.(*ast.Node));

                let resolved = this.current_scope.lookup_recursive(var_expr.name, SYM_FUNC);
                if resolved.has_val() {
                    this.ctx.resolutions.insert(var_expr.(*ast.Expr).id(), resolved.unwrap());
                } else {
                    let val_resolved = this.current_scope.lookup_recursive(var_expr.name, SYM_VALUE);
                    if val_resolved.has_val() {
                        this.ctx.resolutions.insert(var_expr.(*ast.Expr).id(), val_resolved.unwrap());
                    } else {
                        let msg = std.String.from("undefined function '");
                        msg.append(var_expr.name);
                        msg.append("'");
                        this.engine.report(diag.E_UNDEFINED, msg, diag.SEV_ERROR)
                            .span(var_expr.(*ast.Node).range());
                    }
                }
            } else {
                this.resolve_expr(call_expr.callee);
            }
        }

        for let i = 0uz, i < call_expr.args_count, i += 1 {
            this.resolve_expr(*(call_expr.args + i));
        }
    }

    func resolve_ty(ty: *types.Type) {
        if ty == nil {
            return;
        }
        // TODO: implement after adding structure and trait types
    }
}

pub struct TypeChecker {
    engine: *diag.DiagEngine;
    ctx: *Context;
}

struct ParsedInt {
    pub abs_val: u64;
    pub is_neg: bool;
    pub is_overflow: bool;
}

impl TypeChecker {
    pub static func new(engine: *diag.DiagEngine, ctx: *Context): TypeChecker {
        return TypeChecker {
            engine: engine,
            ctx: ctx
        };
    }

    pub func check(res: ast.ParseResult) {
        for let i = 0uz, i < res.count, i += 1 {
            let node = *(res.nodes + i);
            if ast.Stmt.isa(node) {
                this.check_stmt(ast.Stmt.cast(node));
            }
        }
    }

    func check_stmt(stmt: *ast.Stmt) {
        if stmt == nil {
            return;
        }

        let kind = stmt.kind();
        if kind == ast.NODE_VAR_DECL {
            this.check_var_decl(ast.VarDecl.cast(stmt.(*ast.Node)));
        }
        // TODO: add check FuncDecl
        // TODO: add check BlockStmt
        // TODO: add check RetStmt
    }

    func check_var_decl(var_decl: *ast.VarDecl) {
        // TODO: implement logic
        let ty = var_decl.ty;
        this.check_expr(var_decl.init, ty);
        // let def_id = this.ctx.next_def_id();
        // this.ctx.resolutions.insert(var_decl.(*ast.Stmt).id(), def_id);
        // this.current_scope.insert(var_decl.name, def_id);
    }

    func check_expr(expr: *ast.Expr, expected_ty: *types.Type): bool {
        if expr == nil {
            return false;
        }

        let kind = expr.kind();
        if kind == ast.NODE_VAR_EXPR {
            let var_expr = ast.VarExpr.cast(expr.(*ast.Node));
            return this.check_var_expr(var_expr, expected_ty);
        } else if kind == ast.NODE_BIN_EXPR {
            let bin_expr = ast.BinExpr.cast(expr.(*ast.Node));
            return this.check_bin_expr(bin_expr, expected_ty);
        } else if kind == ast.NODE_UN_EXPR {
            let un_expr = ast.UnExpr.cast(expr.(*ast.Node));
            return this.check_un_expr(un_expr, expected_ty);
        } else if kind == ast.NODE_LIT_EXPR {
            let lit_expr = ast.LitExpr.cast(expr.(*ast.Node));
            return this.check_lit_expr(lit_expr, expected_ty);
        }
        return false;
    }

    func check_var_expr(var_expr: *ast.VarExpr, expected_ty: *types.Type): bool {
        let node_id    = var_expr.(*ast.Expr).id();
        let def_id_opt = this.ctx.resolutions.get(node_id);

        let actual_ty = this.ctx.def_types.get(def_id_opt.unwrap());
        if actual_ty == nil {
            return false;
        }

        if expected_ty != nil && actual_ty != expected_ty {
            let msg = std.String.from("mismatched types: expected '");
            msg.append(expected_ty.to_string());
            msg.append("', found '");
            msg.append(actual_ty.to_string());
            msg.append("'");
            this.engine.report(diag.E_TYPE_MISMATCH, msg, diag.SEV_ERROR)
                .span(var_expr.(*ast.Node).range());
            return false;
        }

        this.ctx.node_types.insert(node_id, actual_ty);
        return true;
    }

    func check_bin_expr(bin_expr: *ast.BinExpr, expected_ty: *types.Type): bool {
        let node_id = bin_expr.(*ast.Expr).id();
        let bool_ty = this.ctx.ty_ctx.get_bool_ty();

        if bin_expr.op >= ast.BIN_OP_EQ && bin_expr.op <= ast.BIN_OP_LOG_OR {
            if expected_ty != nil && expected_ty != bool_ty {
                let msg = std.String.from("mismatched types: comparison evaluates to 'bool', expected '");
                msg.append(expected_ty.to_string());
                msg.append("'");
                this.engine.report(diag.E_TYPE_MISMATCH, msg, diag.SEV_ERROR)
                    .span(bin_expr.(*ast.Node).range());
                return false;
            }

            if bin_expr.op == ast.BIN_OP_LOG_AND || bin_expr.op == ast.BIN_OP_LOG_OR {
                let left_ok  = this.check_expr(bin_expr.left, bool_ty);
                let right_ok = this.check_expr(bin_expr.right, bool_ty);
                if !left_ok || !right_ok {
                    return false;
                }
            } else {
                let left_ty = this.infer_expr(bin_expr.left);
                if left_ty == nil {
                    return false;
                }
                if !this.check_expr(bin_expr.right, left_ty) {
                    return false;
                }
            }

            this.ctx.node_types.insert(node_id, bool_ty);
            return true;
        }

        let target_ty = expected_ty;
        if target_ty == nil {
            target_ty = this.infer_expr(bin_expr.left);
            if target_ty == nil {
                return false;
            }
        } else {
            if !this.check_expr(bin_expr.left, target_ty) {
                return false;
            }
        }

        if !this.check_expr(bin_expr.right, target_ty) {
            return false;
        }

        this.ctx.node_types.insert(node_id, target_ty);
        return true;
    }

    func check_un_expr(un_expr: *ast.UnExpr, expected_ty: *types.Type): bool {
        let node_id = un_expr.(*ast.Expr).id();
        let bool_ty = this.ctx.ty_ctx.get_bool_ty();

        if un_expr.op == ast.UN_OP_NOT {
            if expected_ty != nil && expected_ty != bool_ty {
                let msg = std.String.from("mismatched types: boolean negation evaluates to 'bool', expected '");
                msg.append(expected_ty.to_string());
                msg.append("'");
                this.engine.report(diag.E_TYPE_MISMATCH, msg, diag.SEV_ERROR)
                    .span(un_expr.(*ast.Node).range());
                return false;
            }

            if !this.check_expr(un_expr.right, bool_ty) {
                return false;
            }

            this.ctx.node_types.insert(node_id, bool_ty);
            return true;
        }

        let target_ty = expected_ty;
        if target_ty == nil {
            target_ty = this.infer_expr(un_expr.right);
            if target_ty == nil {
                return false;
            }
        }

        if !this.check_expr(un_expr.right, target_ty) {
            return false;
        }

        this.ctx.node_types.insert(node_id, target_ty);
        return true;
    }

    func check_lit_expr(lit_expr: *ast.LitExpr, expected_ty: *types.Type): bool {
        // TODO: implement logic

        if lit_expr.tok_kind != lexer.TOK_NUM_LIT {
            return false;
        }
        // only numbers:
        return this.check_num_lit_expr(lit_expr, expected_ty);
    }

    func check_num_lit_expr(lit_expr: *ast.LitExpr, expected_ty: *types.Type): bool {
        if is_floating(lit_expr.val) {
            return this.check_float_lit_expr(lit_expr, expected_ty);
        }
        return this.check_int_lit_expr(lit_expr, expected_ty);
    }

    func check_int_lit_expr(lit_expr: *ast.LitExpr, expected_ty: *types.Type): bool {
        let parsed_int = parse_str_to_int(lit_expr.val);
        if parsed_int.is_overflow {
            this.engine.report(diag.E_CANNOT_FIT, "cannot fit integer literal to any integer type",
                diag.SEV_ERROR)
                .span(lit_expr.(*ast.Node).range());
            return false;
        }
        if expected_ty != nil && !types.IntType.isa(expected_ty) {
            let msg = std.String.from("mismatched types: expected '");
            msg.append(expected_ty.to_string());
            msg.append("', found integer");
            this.engine.report(diag.E_TYPE_MISMATCH, msg, diag.SEV_ERROR)
                .span(lit_expr.(*ast.Node).range());
            return false;
        }
        if expected_ty == nil {
            expected_ty = this.ctx.ty_ctx.get_int_ty(32u32, false);
        }
        let int_ty = types.IntType.cast(expected_ty);
        if !this.can_fit(parsed_int.abs_val, parsed_int.is_neg, int_ty) {
            let msg = std.String.from("cannot fit integer literal to ");
            msg.append(expected_ty.to_string());
            msg.append(" type");

            let label_msg = std.String.from("must be in range [");
            if int_ty.is_unsigned {
                label_msg.append("0, ");
                let max_limit_tmp = std.usize_to_string(int_ty.max_unsigned_limit().(usize));
                label_msg.append(max_limit_tmp);
                max_limit_tmp.destroy();
            } else {
                label_msg.append("-");
                let min_limit_tmp = std.usize_to_string(int_ty.max_signed_abs_limit().(usize));
                label_msg.append(min_limit_tmp);
                min_limit_tmp.destroy();

                label_msg.append(", ");

                let max_limit_tmp = std.usize_to_string(int_ty.max_signed_limit().(usize));
                label_msg.append(max_limit_tmp);
                max_limit_tmp.destroy();
            }
            label_msg.append("]");

            this.engine.report(diag.E_CANNOT_FIT, msg, diag.SEV_ERROR)
                .span(lit_expr.(*ast.Node).range(), label_msg);
            return false;
        }
        return true;
    }

    func check_float_lit_expr(lit_expr: *ast.LitExpr, expected_ty: *types.Type): bool {
        // TODO: implement logic
        return false;
    }

    pub func infer_expr(expr: *ast.Expr): *types.Type {
        if expr == nil {
            return nil;
        }

        let kind = expr.kind();
        let inferred_ty: *types.Type;

        if kind == ast.NODE_VAR_EXPR {
            inferred_ty = this.infer_var_expr(ast.VarExpr.cast(expr.(*ast.Node)));
        } else if kind == ast.NODE_BIN_EXPR {
            inferred_ty = this.infer_bin_expr(ast.BinExpr.cast(expr.(*ast.Node)));
        } else if kind == ast.NODE_UN_EXPR {
            inferred_ty = this.infer_un_expr(ast.UnExpr.cast(expr.(*ast.Node)));
        } else if kind == ast.NODE_LIT_EXPR {
            inferred_ty = this.infer_lit_expr(ast.LitExpr.cast(expr.(*ast.Node)));
        }

        if inferred_ty != nil {
            this.ctx.node_types.insert(expr.id(), inferred_ty);
        }

        return inferred_ty;
    }

    func infer_var_expr(var_expr: *ast.VarExpr): *types.Type {
        let node_id = var_expr.(*ast.Expr).id();
        let def_id_opt = this.ctx.resolutions.get(node_id);

        if !def_id_opt.has_val() {
            return nil;
        }

        let ty = this.ctx.def_types.get(def_id_opt.unwrap());
        if ty == nil {
            return nil;
        }
        return ty;
    }

    func infer_bin_expr(bin_expr: *ast.BinExpr): *types.Type {
        let left  = this.infer_expr(bin_expr.left);
        if left == nil {
            return nil;
        }
        let right = this.infer_expr(bin_expr.right);
        if right == nil {
            return nil;
        }
        if bin_expr.op >= ast.BIN_OP_EQ && bin_expr.op <= ast.BIN_OP_LOG_OR {
            return this.ctx.ty_ctx.get_bool_ty();
        }
        return this.get_common_ty(left, right);
    }

    func infer_un_expr(un_expr: *ast.UnExpr): *types.Type {
        if un_expr.op == ast.UN_OP_NOT {
            return this.ctx.ty_ctx.get_bool_ty();
        }
        return this.infer_expr(un_expr.right);
    }

    func infer_lit_expr(lit_expr: *ast.LitExpr): *types.Type {
        if lit_expr.tok_kind == lexer.TOK_NUM_LIT {
            if is_floating(lit_expr.val) {
                return this.ctx.ty_ctx.get_float_ty(64u32);
            }
            return this.ctx.ty_ctx.get_int_ty(32u32, false);
        }
        // TODO: implement logic
        return nil;
    }

    func can_fit(val: u64, is_neg: bool, expected_ty: *types.IntType): bool {
        if expected_ty.is_unsigned {
            if is_neg {
                return false;
            }
            return val <= expected_ty.max_unsigned_limit();
        }
        if is_neg {
            return val <= expected_ty.max_signed_abs_limit();
        }
        return val <= expected_ty.max_signed_limit();
    }

    func get_common_ty(a: *types.Type, b: *types.Type): *types.Type {
        if a == b {
            return a;
        }
        if a == nil || b == nil {
            return nil;
        }
        return nil;
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

func is_floating(str: std.StringView): bool {
    for let i = 0uz, i < str.len(), i += 1 {
        if str.get(i).unwrap() == '.'.(u8) {
            return true;
        }
    }
    return false;
}

func parse_str_to_int(str: std.StringView): ParsedInt {
    let is_neg = false;
    let res: u64;
    for let i = 0uz, i < str.len(), i += 1 {
        let c = str.get(i).unwrap();
        if c == '-'.(u8) {
            is_neg = true;
            continue;
        }

        let digit = c - '0'.(u8);
        if res > ((18446744073709551615u64 - digit) / 10u64) {
            return ParsedInt { abs_val: 0, is_neg: is_neg, is_overflow: true };
        }
        res = res * 10u64 + digit;
    }
    return ParsedInt { abs_val: res, is_neg: is_neg, is_overflow: false };
}
