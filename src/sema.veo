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

pub struct ScopeEntry {
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

    pub func lookup_local(name: std.StringView): basic.OptionDefId {
        for let i = 0uz, i < this.count, i += 1 {
            let entry = this.entries + i;
            if entry.name.compare_to(name) == 0 {
                return basic.OptionDefId.some(entry.def_id);
            }
        }
        return basic.OptionDefId.none();
    }

    pub func lookup_recursive(name: std.StringView): basic.OptionDefId {
        let curr = this;
        for curr != nil {
            let res = curr.lookup_local(name);
            if res.has_val() {
                return res;
            }
            curr = curr.parent;
        }
        return basic.OptionDefId.none();
    }

    pub func insert(name: std.StringView, def_id: basic.DefId) {
        if this.count >= this.cap {
            let old_cap = this.cap;
            this.cap = math.max(this.cap * 2uz, this.cap + 1uz);
            this.entries = sys.realloc(
                this.entries.(*u8),
                this.cap * @size_of(ScopeEntry)
            ).(*ScopeEntry);
        }
        let entry = this.entries + this.count;
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
    def_types: HashMapDefIdType;
    node_types: HashMapU32Type;
    next_def_id: u32;
}

impl Context {
    pub static func new(): Context {
        return Context {
            resolutions: HashMapU32DefId.new(),
            def_types: HashMapDefIdType.new(),
            node_types: HashMapU32Type.new(),
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

pub struct NameResolver {
    current_scope: *Scope;
    ctx: *Context;
}

impl NameResolver {
    pub static func new(ctx: *Context): NameResolver {
        return NameResolver {
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
        } else {
            std.panic("Unsupported statement kind");
        }
    }

    func resolve_var_decl(var_decl: *ast.VarDecl) {
        let existing = this.current_scope.lookup_local(var_decl.name);
        if existing.has_val() {
            std.panic("Redefinition of variable");
            return;
        }

        this.resolve_expr(var_decl.init);
        let def_id = this.ctx.next_def_id();
        this.ctx.resolutions.insert(var_decl.(*ast.Stmt).id(), def_id);
        this.current_scope.insert(var_decl.name, def_id);
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
            let lit_expr = ast.LitExpr.cast(expr.(*ast.Node));
            return this.resolve_lit_expr(lit_expr);
        } else {
            std.panic("Unsupported expression kind");
        }
    }

    func resolve_lit_expr(lit: *ast.LitExpr) {}

    func resolve_var_expr(var_expr: *ast.VarExpr) {
        let resolved = this.current_scope.lookup_recursive(var_expr.name);
        if !resolved.has_val() {
            std.panic("Use of undeclared variable");
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
