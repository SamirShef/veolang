import std.mem;
import std.math;
import std;
import std.sys;
import llvm.smloc;
import types;
import basic;

pub const SYM_VAR    = 0;
pub const SYM_FUNC   = 1;
pub const SYM_STRUCT = 2;

pub struct Symbol {
    kind: i32;
    name: std.StringView;
    id: basic.DefId;
}

impl Symbol {
    pub static func new(kind: i32, name: std.StringView, id: basic.DefId): Symbol {
        return Symbol {
            kind: kind,
            name: name,
            id: id
        };
    }

    pub func kind(): i32 {
        return this.kind;
    }

    pub func name(): std.StringView {
        return this.name;
    }

    pub func id(): basic.DefId {
        return this.id;
    }
}

pub struct VarSymbol {
    pub base: Symbol;
    pub is_const: bool;
    pub val: basic.OptionValue;
    pub ty: *types.Type;
}

impl VarSymbol {
    pub static func isa(sym: *Symbol): bool {
        if sym == nil {
            return false;
        }
        return sym.kind() == SYM_VAR;
    }

    pub static func cast(sym: *Symbol): *VarSymbol {
        if !VarSymbol.isa(sym) {
            std.panic("RTTI Error: Failed cast to *VarSymbol");
        }
        return sym.(*VarSymbol);
    }

    pub func name(): std.StringView {
        return this.base.name();
    }

    pub func id(): basic.DefId {
        return this.base.id();
    }
}

pub struct SymbolTable {
    pub entries: **Symbol;
    pub count: usize;
    pub cap: usize;
}

impl SymbolTable {
    pub static func new(): SymbolTable {
        let ptr: *Symbol;
        let cap     = 16uz;
        let entries = sys.malloc(@size_of(ptr) * cap).(**Symbol);
        let count   = 0uz;
        return SymbolTable {
            entries: entries,
            count: count,
            cap: cap
        };
    }

    pub func insert(sym: *Symbol) {
        if sym == nil { return; }

        let existing_id = this.lookup_by_id(sym.id());
        if existing_id != nil {
            std.panic("Compiler Internal Error: Duplicate global DefId registered");
        }

        if this.count >= this.cap {
            let old_cap = this.cap;
            this.cap = math.max(this.cap * 2uz, this.cap + 1uz);
            this.entries = sys.realloc(this.entries.(*u8), @size_of(sym) * this.cap).(**Symbol);
        }

        *(this.entries + this.count) = sym;
        this.count += 1uz;
    }

    pub func lookup_by_name(name: std.StringView): *Symbol {
        for let i = 0uz, i < this.count, i += 1 {
            let sym = *(this.entries + i);
            if sym.name().compare_to(name) == 0 {
                return sym;
            }
        }
        return nil;
    }

    pub func lookup_by_id(id: basic.DefId): *Symbol {
        for let i = 0uz, i < this.count, i += 1 {
            let sym = *(this.entries + i);
            if sym.id().mod_id == id.mod_id && sym.id().sym_id == id.sym_id {
                return sym;
            }
        }
        return nil;
    }
}
