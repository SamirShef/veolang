import llvm.smloc;
import std.mem;
import std.sys;
import std.math;
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

    pub static func invalid(): DefId {
        return DefId.new((-1).(u64), (-1).(u64));
    }

    pub func equals(other: DefId): bool {
        return this.mod_id == other.mod_id && this.sym_id == other.sym_id;
    }

    pub func is_invalid(): bool {
        return this.equals(DefId.invalid());
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

pub struct OptionU32 {
    val: u32;
    has_val: bool;
}

impl OptionU32 {
    pub static func some(val: u32): OptionU32 {
        return OptionU32 { has_val: true, val: val };
    }

    pub static func none(): OptionU32 {
        return OptionU32 { has_val: false };
    }

    pub func has_val(): bool {
        return this.has_val;
    }

    pub func unwrap(): u32 {
        if !this.has_val {
            std.panic("Called unwrap() on a 'None' value (Option is empty)");
        }
        return this.val;
    }

    pub func unwrap_or(err_val: u32): u32 {
        if !this.has_val {
            return err_val;
        }
        return this.val;
    }
}

pub struct ListU32 {
    data: *u32;
    len: usize;
    cap: usize;
}

impl ListU32 {
    pub static func new(): ListU32 {
        let cap = 4uz;
        let data = sys.malloc(cap * @size_of(u32)).(*u32);
        return ListU32 {
            data: data,
            len: 0,
            cap: cap
        };
    }

    pub func add(val: u32) {
        if this.cap < this.len + 1uz {
            let old_cap = this.cap;
            this.cap = math.max(this.cap * 2uz, this.len + 1uz);
            this.data = sys.realloc(
                            this.data.(*u8),
                            this.cap * @size_of(u32)
                        ).(*u32);
        }
        *(this.data + this.len) = val;
        this.len += 1;
    }

    pub func get(index: usize): OptionU32 {
        if index < this.len {
            return OptionU32.some(*(this.data + index));
        }
        return OptionU32.none();
    }

    pub func data(): *u32 {
        return this.data;
    }

    pub func len(): usize {
        return this.len;
    }

    pub func capacity(): usize {
        return this.cap;
    }

    pub func destroy() {
        sys.free(this.data.(*u8));
    }
}

pub struct File {
    name: std.StringView;
    content: std.String;
    line_starts: ListU32;
}

impl File {
    pub static func new(name: std.StringView, content: std.String, line_starts: ListU32): File {
        return File {
            name: name,
            content: content,
            line_starts: line_starts
        };
    }

    pub func content(): std.String {
        return this.content;
    }

    pub func destroy() {
        this.content.destroy();
    }
}

pub struct OptionFile {
    val: File;
    has_val: bool;
}

impl OptionFile {
    pub static func some(val: File): OptionFile {
        return OptionFile { has_val: true, val: val };
    }

    pub static func none(): OptionFile {
        return OptionFile { has_val: false };
    }

    pub func has_val(): bool {
        return this.has_val;
    }

    pub func unwrap(): File {
        if !this.has_val {
            std.panic("Called unwrap() on a 'None' value (Option is empty)");
        }
        return this.val;
    }

    pub func unwrap_or(err_val: File): File {
        if !this.has_val {
            return err_val;
        }
        return this.val;
    }
}

pub struct ListFile {
    data: *File;
    len: usize;
    cap: usize;
}

impl ListFile {
    pub static func new(): ListFile {
        let cap = 4uz;
        let data = sys.malloc(cap * @size_of(File)).(*File);
        return ListFile {
            data: data,
            len: 0,
            cap: cap
        };
    }

    pub func add(val: File) {
        if this.cap < this.len + 1uz {
            let old_cap = this.cap;
            this.cap = math.max(this.cap * 2uz, this.len + 1uz);
            this.data = sys.realloc(
                            this.data.(*u8),
                            this.cap * @size_of(File)
                        ).(*File);
        }
        *(this.data + this.len) = val;
        this.len += 1;
    }

    pub func get(index: usize): OptionFile {
        if index < this.len {
            return OptionFile.some(*(this.data + index));
        }
        return OptionFile.none();
    }

    pub func data(): *File {
        return this.data;
    }

    pub func len(): usize {
        return this.len;
    }

    pub func capacity(): usize {
        return this.cap;
    }

    pub func destroy() {
        for let i = 0uz, i < this.len, i += 1 {
            (this.data + i).destroy();
        }
        sys.free(this.data.(*u8));
    }
}

pub struct SourceMgr {
    buffers: ListFile;
}

impl SourceMgr {
    pub static func new(): SourceMgr {
        return SourceMgr {
            buffers: ListFile.new()
        };
    }

    pub func add_buffer(name: std.StringView, content: std.String): usize {
        let id = this.buffers.len();
        let line_starts = ListU32.new();
        this.buffers.add(File.new(name, content, line_starts));
        return id;
    }

    pub func get_buffer(id: usize): OptionFile {
        return this.buffers.get(id);
    }

    pub func destroy() {
        this.buffers.destroy();
    }
}
