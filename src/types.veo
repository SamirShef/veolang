import std.mem;
import std;

pub const TYPE_INT   = 0;
pub const TYPE_FLOAT = 1;
pub const TYPE_SIZE   = 2;

pub struct Type {
    kind: i32;
}

impl Type {
    pub static func new(kind: i32): Type {
        return Type { kind: kind };
    }

    pub func kind(): i32 {
        return this.kind;
    }
}

pub struct Context {
    alloc: *mem.ArenaAllocator;
    pub i8_t: *Type;
    pub i16_t: *Type;
    pub i32_t: *Type;
    pub i64_t: *Type;
    pub isize_t: *Type;
    pub u8_t: *Type;
    pub u16_t: *Type;
    pub u32_t: *Type;
    pub u64_t: *Type;
    pub usize_t: *Type;
    pub f32_t: *Type;
    pub f64_t: *Type;
    // pub bool_t: *Type; // later
    // pub char_t: *Type; // later
}

impl Context {
    pub static func new(alloc: *mem.ArenaAllocator): Context {
        let ctx = Context { alloc: alloc };
        ctx.i8_t    = ctx.alloc_int_ty(8u32, false);
        ctx.i16_t   = ctx.alloc_int_ty(16u32, false);
        ctx.i32_t   = ctx.alloc_int_ty(32u32, false);
        ctx.i64_t   = ctx.alloc_int_ty(64u32, false);
        ctx.isize_t = ctx.alloc_size_ty(false);
        ctx.u8_t    = ctx.alloc_int_ty(8u32, true);
        ctx.u16_t   = ctx.alloc_int_ty(16u32, true);
        ctx.u32_t   = ctx.alloc_int_ty(32u32, true);
        ctx.u64_t   = ctx.alloc_int_ty(64u32, true);
        ctx.usize_t = ctx.alloc_size_ty(true);
        ctx.f32_t   = ctx.alloc_float_ty(32u32);
        ctx.f64_t   = ctx.alloc_float_ty(64u32);
        return ctx;
    }

    pub func get_int_ty(width: u32, is_unsigned: bool): *Type {
        if is_unsigned {
            if width == 8u32 {
                return this.u8_t;
            } else if width == 16u32 {
                return this.u16_t;
            } else if width == 32u32 {
                return this.u32_t;
            } else if width == 64u32 {
                return this.u64_t;
            }
        } else {
            if width == 8u32 {
                return this.i8_t;
            } else if width == 16u32 {
                return this.i16_t;
            } else if width == 32u32 {
                return this.i32_t;
            } else if width == 64u32 {
                return this.i64_t;
            }
        }
        std.panic("Unsupported integer width");
        return nil;
    }

    pub func get_size_ty(is_unsigned: bool): *Type {
        if is_unsigned {
            return this.usize_t;
        } else {
            return this.isize_t;
        }
    }

    pub func get_float_ty(width: u32): *Type {
        if width == 32u32 {
            return this.f32_t;
        } else if width == 64u32 {
            return this.f64_t;
        }
        std.panic("Unsupported float width");
        return nil;
    }

    pub func alloc_int_ty(width: u32, is_unsigned: bool): *Type {
        let raw        = this.alloc.alloc(@size_of(IntType));
        let ty         = raw.(*IntType);
        ty.base        = Type.new(TYPE_INT);
        ty.width       = width;
        ty.is_unsigned = is_unsigned;
        return ty.(*Type);
    }

    pub func alloc_size_ty(is_unsigned: bool): *Type {
        let raw        = this.alloc.alloc(@size_of(SizeType));
        let ty         = raw.(*SizeType);
        ty.base        = Type.new(TYPE_SIZE);
        ty.is_unsigned = is_unsigned;
        return ty.(*Type);
    }

    pub func alloc_float_ty(width: u32): *Type {
        let raw  = this.alloc.alloc(@size_of(FloatType));
        let ty   = raw.(*FloatType);
        ty.base  = Type.new(TYPE_FLOAT);
        ty.width = width;
        return ty.(*Type);
    }
}

impl std.ToString for Type {
    pub func to_string(alloc: mem.Allocator): std.String {
        if IntType.isa(this) {
            let int = IntType.cast(this);
            let str: std.String;
            str.append(alloc, int.is_unsigned ? "u" : "i");
            str.append(alloc, std.i32_to_string(alloc, int.width.(i32)));
            return str;
        } else if FloatType.isa(this) {
            let float = FloatType.cast(this);
            let str = std.String.from(alloc, "f");
            str.append(alloc, std.i32_to_string(alloc, float.width.(i32)));
            return str;
        } else if SizeType.isa(this) {
            let size = IntType.cast(this);
            let str: std.String;
            str.append(alloc, size.is_unsigned ? "u" : "i");
            str.append(alloc, "size");
            return str;
        }
        return std.String.from(alloc, "");
    }
}

pub struct IntType {
    pub base: Type;
    pub width: u32;
    pub is_unsigned: bool;
}

impl IntType {
    pub static func isa(ty: *Type): bool {
        if ty == nil {
            return false;
        }
        return ty.kind() == TYPE_INT;
    }

    pub static func cast(ty: *Type): *IntType {
        if !IntType.isa(ty) {
            std.panic("RTTI Error: Failed cast to *IntType");
        }
        return ty.(*IntType);
    }
}

pub struct FloatType {
    pub base: Type;
    pub width: u32;
}

impl FloatType {
    pub static func isa(ty: *Type): bool {
        if ty == nil {
            return false;
        }
        return ty.kind() == TYPE_FLOAT;
    }

    pub static func cast(ty: *Type): *FloatType {
        if !FloatType.isa(ty) {
            std.panic("RTTI Error: Failed cast to *FloatType");
        }
        return ty.(*FloatType);
    }
}

pub struct SizeType {
    pub base: Type;
    pub is_unsigned: bool;
}

impl SizeType {
    pub static func isa(ty: *Type): bool {
        if ty == nil {
            return false;
        }
        return ty.kind() == TYPE_SIZE;
    }

    pub static func cast(ty: *Type): *SizeType {
        if !SizeType.isa(ty) {
            std.panic("RTTI Error: Failed cast to *SizeType");
        }
        return ty.(*SizeType);
    }
}
