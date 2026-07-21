import llvm.smloc;
import std.mem;
import std;
import types;

pub struct Span {
    pub start: smloc.SMLoc;
    pub end: smloc.SMLoc;
}

impl Span {
    pub static func new(start: smloc.SMLoc, end: smloc.SMLoc): Span {
        return Span { start: start, end: end };
    }

    pub static func new(start: smloc.SMLoc): Span {
        return Span { start: start, end: start };
    }
}

pub const VAL_UNKNOWN = 0;
pub const VAL_CONST   = 1;

pub struct Value {
    pub kind: i32;
    pub ty: *types.Type;
    pub as_int: u64;
    pub as_float: f64;
}

impl Value {
    pub static func new(kind: i32, as_int: u64, as_float: f64, ty: *types.Type): Value {
        return Value {
            kind: kind,
            ty: ty,
            as_int: as_int,
            as_float: as_float
        };
    }

    pub static func new(kind: i32, as_int: u64, ty: *types.Type): Value {
        return Value.new(kind, as_int, 0.0, ty);
    }

    pub static func new(kind: i32, as_float: f64, ty: *types.Type): Value {
        return Value.new(kind, 0u64, as_float, ty);
    }

    pub static func new(kind: i32, ty: *types.Type): Value {
        return Value.new(kind, 0u64, 0.0, ty);
    }
}

pub struct DefId {
    pub mod_id: u64;
    pub sym_id: u64;
}

impl DefId {
    pub static func new(mod_id: u64, sym_id: u64): DefId {
        return DefId {
            mod_id: mod_id,
            sym_id: sym_id
        };
    }
}

pub struct OptionValue {
    has_val: bool;
    val: Value;
}

impl OptionValue {
    pub static func some(val: Value): OptionValue {
        return OptionValue { has_val: true, val: val };
    }

    pub static func none(): OptionValue {
        return OptionValue { has_val: false };
    }

    pub func has_val(): bool {
        return this.has_val;
    }

    pub func unwrap(): Value {
        if !this.has_val {
            std.panic("Called unwrap() on a 'None' value (Option is empty)");
        }
        return this.val;
    }

    pub func unwrap_or(err_val: Value): Value {
        if !this.has_val {
            return err_val;
        }
        return this.val;
    }
}

pub struct OptionDefId {
    has_val: bool;
    val: DefId;
}

impl OptionDefId {
    pub static func some(val: DefId): OptionDefId {
        return OptionDefId { has_val: true, val: val };
    }

    pub static func none(): OptionDefId {
        return OptionDefId { has_val: false };
    }

    pub func has_val(): bool {
        return this.has_val;
    }

    pub func unwrap(): DefId {
        if !this.has_val {
            std.panic("Called unwrap() on a 'None' value (Option is empty)");
        }
        return this.val;
    }

    pub func unwrap_or(err_val: DefId): DefId {
        if !this.has_val {
            return err_val;
        }
        return this.val;
    }
}

pub func hash64(key: *u8, len: usize): u64 {
    let hash = 2166136261u64;
    for let i = 0uz, i < len, i += 1 {
        hash ^= *(key + i);
        hash *= 16777619u64;
    }
    return hash;
}

pub func hash64(key: std.StringView): u64 {
    return hash64(key.data(), key.len());
}

pub func hash64(key: std.String): u64 {
    return hash64(key.data(), key.len());
}
