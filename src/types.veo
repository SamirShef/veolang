import std.mem;
import std;

pub const TYPE_INT   = 0;
pub const TYPE_FLOAT = 1;
pub const TYPE_SIZE  = 2;
pub const TYPE_BOOL  = 3;
pub const TYPE_NOTH  = 4;
pub const TYPE_FUNC  = 5;

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

    pub func max_unsigned_limit(): u64 {
        let width       = this.width;
        if width == 8u32 {
            return 0xFF;
        } else if width == 16u32 {
            return 0xFFFF;
        } else if width == 32u32 {
            return 0xFFFFFFFF;
        } else if width == 64u32 {
            return 0xFFFFFFFFFFFFFFFF;
        }
        return 0;
    }

    pub func max_signed_limit(): u64 {
        let width       = this.width;
        if width == 8u32 {
            return 0x7F;
        } else if width == 16u32 {
            return 0x7FFF;
        } else if width == 32u32 {
            return 0x7FFFFFFF;
        } else if width == 64u32 {
            return 0x7FFFFFFFFFFFFFFF;
        }
        return 0;
    }

    pub func max_signed_abs_limit(): u64 {
        let width       = this.width;
        if width == 8u32 {
            return 0x80;
        } else if width == 16u32 {
            return 0x8000;
        } else if width == 32u32 {
            return 0x80000000;
        } else if width == 64u32 {
            return 0x8000000000000000;
        }
        return 0;
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

pub struct BoolType {
    pub base: Type;
}

impl BoolType {
    pub static func isa(ty: *Type): bool {
        if ty == nil {
            return false;
        }
        return ty.kind() == TYPE_BOOL;
    }

    pub static func cast(ty: *Type): *BoolType {
        if !BoolType.isa(ty) {
            std.panic("RTTI Error: Failed cast to *BoolType");
        }
        return ty.(*BoolType);
    }
}

pub struct NothType {
    pub base: Type;
}

impl NothType {
    pub static func isa(ty: *Type): bool {
        if ty == nil {
            return false;
        }
        return ty.kind() == TYPE_NOTH;
    }

    pub static func cast(ty: *Type): *NothType {
        if !NothType.isa(ty) {
            std.panic("RTTI Error: Failed cast to *NothType");
        }
        return ty.(*NothType);
    }
}

pub struct FuncType {
    pub base: Type;
    pub args: **Type;
    pub args_count: usize;
    pub ret_ty: *Type;
}

impl FuncType {
    pub static func isa(ty: *Type): bool {
        if ty == nil {
            return false;
        }
        return ty.kind() == TYPE_FUNC;
    }

    pub static func cast(ty: *Type): *FuncType {
        if !FuncType.isa(ty) {
            std.panic("RTTI Error: Failed cast to *FuncType");
        }
        return ty.(*FuncType);
    }
}

pub struct Context {
    alloc: mem.ArenaAllocator;
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
    pub bool_t: *Type;
    pub noth_t: *Type;
    func_types: **FuncType;
    func_types_count: usize;
    func_types_cap: usize;
}

impl Context {
    pub static func new(): Context {
        let alloc     = mem.ArenaAllocator.init(64uz * mem.KB);
        let cap        = 16uz;
        let func_types = alloc.alloc(cap * @size_of(*FuncType));
        let ctx = Context {
            alloc: alloc,
            func_types: func_types.(**FuncType),
            func_types_count: 0uz,
            func_types_cap: cap
        };
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
        ctx.bool_t  = ctx.alloc_bool_ty();
        ctx.noth_t  = ctx.alloc_noth_ty();
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

    pub func get_bool_ty(): *Type {
        return this.bool_t;
    }

    pub func get_noth_ty(): *Type {
        return this.noth_t;
    }

    pub func get_func_ty(args: **Type, args_count: usize, ret_ty: *Type): *Type {
        for let i = 0uz, i < this.func_types_count, i += 1 {
            let fn_ty = *(this.func_types + i);
            if fn_ty.ret_ty == ret_ty && fn_ty.args_count == args_count {
                let match = true;
                for let j = 0uz, j < args_count, j += 1 {
                    if *(fn_ty.args + j) != *(args + j) {
                        match = false;
                        break;
                    }
                }
                if match {
                    return fn_ty.(*Type);
                }
            }
        }

        let persistent_args: **Type = nil;
        if args_count > 0uz {
            let raw_args = this.alloc.alloc(args_count * @size_of(*Type));
            persistent_args = raw_args.(**Type);
            for let i = 0uz, i < args_count, i += 1 {
                *(persistent_args + i) = *(args + i);
            }
        }

        let ty = this.alloc_func_ty(persistent_args, args_count, ret_ty);

        if this.func_types_count >= this.func_types_cap {
            let new_cap = this.func_types_cap * 2uz;
            let new_buf = this.alloc.alloc(new_cap * @size_of(*FuncType)).(**FuncType);
            for let i = 0uz, i < this.func_types_count, i += 1 {
                *(new_buf + i) = *(this.func_types + i);
            }
            this.func_types = new_buf;
            this.func_types_cap = new_cap;
        }

        *(this.func_types + this.func_types_count) = ty.(*FuncType);
        this.func_types_count += 1;

        return ty;
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

    pub func alloc_bool_ty(): *Type {
        let raw  = this.alloc.alloc(@size_of(BoolType));
        let ty   = raw.(*BoolType);
        ty.base  = Type.new(TYPE_BOOL);
        return ty.(*Type);
    }

    pub func alloc_noth_ty(): *Type {
        let raw  = this.alloc.alloc(@size_of(NothType));
        let ty   = raw.(*NothType);
        ty.base  = Type.new(TYPE_NOTH);
        return ty.(*Type);
    }

    pub func alloc_func_ty(args: **Type, args_count: usize, ret_ty: *Type): *Type {
        let raw        = this.alloc.alloc(@size_of(FuncType));
        let ty         = raw.(*FuncType);
        ty.base        = Type.new(TYPE_FUNC);
        ty.args        = args;
        ty.args_count  = args_count;
        ty.ret_ty      = ret_ty;
        return ty.(*Type);
    }
}

impl std.ToString for Type {
    pub func to_string(): std.String {
        if IntType.isa(this) {
            let int = IntType.cast(this);
            let str = std.String.from(int.is_unsigned ? "u" : "i");
            str.append(std.i32_to_string(int.width.(i32)));
            return str;
        } else if FloatType.isa(this) {
            let float = FloatType.cast(this);
            let str = std.String.from("f");
            str.append(std.i32_to_string(float.width.(i32)));
            return str;
        } else if SizeType.isa(this) {
            let size = SizeType.cast(this);
            let str = std.String.from(size.is_unsigned ? "u" : "i");
            str.append("size");
            return str;
        } else if BoolType.isa(this) {
            return std.String.from("bool");
        } else if NothType.isa(this) {
            return std.String.from("noth");
        } else if FuncType.isa(this) {
            let fn_ty = FuncType.cast(this);
            let str = std.String.from("func(");
            for let i = 0uz, i < fn_ty.args_count, i += 1 {
                if i > 0uz {
                    str.append(", ");
                }
                let arg_ty = *(fn_ty.args + i);
                if arg_ty != nil {
                    let arg_str = arg_ty.to_string();
                    str.append(arg_str);
                }
            }
            str.append("): ");
            let ret_str = fn_ty.ret_ty.to_string();
            str.append(ret_str);
            return str;
        }
        return std.String.from("");
    }
}
