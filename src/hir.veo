import std.math;
import std.mem;
import std.sys;
import std;
import types;
import basic;

pub const VAL_INT   = 0;
pub const VAL_FLOAT = 1;
pub const VAL_STR   = 2;
pub const VAL_NIL   = 3;

pub struct Value {
    pub kind: i32;
    pub as_int: u64;
    pub as_float: f64;
    pub as_str: std.StringView;
}

impl Value {
    pub static func from_int(as_int: u64): Value {
        return Value { kind: VAL_INT, as_int: as_int, as_float: 0.0, as_str: std.StringView.from("") };
    }

    pub static func from_float(as_float: f64): Value {
        return Value { kind: VAL_FLOAT, as_int: 0, as_float: as_float, as_str: std.StringView.from("") };
    }

    pub static func from_string(as_str: std.StringView): Value {
        return Value { kind: VAL_STR, as_int: 0, as_float: 0.0, as_str: as_str };
    }

    pub static func from_nil(): Value {
        return Value { kind: VAL_NIL, as_int: 0, as_float: 0.0, as_str: std.StringView.from("") };
    }
}

pub const OPERAND_LOCAL = 0;
pub const OPERAND_DEF   = 1;
pub const OPERAND_CONST = 2;

pub struct Operand {
    pub kind: i32;
    pub ty: *types.Type;
    pub local_id: u32;
    pub def_id: basic.DefId;
    pub val: Value;
}

impl Operand {
    pub static func from_local(id: u32, ty: *types.Type): Operand {
        return Operand {
            kind: OPERAND_LOCAL,
            ty: ty,
            local_id: id,
            def_id: basic.DefId.invalid(),
            val: Value.from_nil()
        };
    }

    pub static func from_def(id: basic.DefId, ty: *types.Type): Operand {
        return Operand {
            kind: OPERAND_DEF,
            ty: ty,
            local_id: 0u32,
            def_id: id,
            val: Value.from_nil()
        };
    }

    pub static func from_const(val: Value, ty: *types.Type): Operand {
        return Operand {
            kind: OPERAND_CONST,
            ty: ty,
            local_id: 0u32,
            def_id: basic.DefId.invalid(),
            val: val
        };
    }
}

pub const INST_LOAD   = 1; // dest = *op1
pub const INST_STORE  = 2; // *op1 = op2
pub const INST_BINARY = 3; // dest = op1 sub_op op2
pub const INST_UNARY  = 4; // dest = sub_op op1

pub struct Inst {
    pub kind: i32;
    pub prev: *Inst;
    pub next: *Inst;
    pub ty: *types.Type;
    pub dest: u32;
    pub op1: Operand;
    pub op2: Operand;
    pub sub_op: i32;
}

impl Inst {
    pub static func new(kind: i32, dest: u32, ty: *types.Type, op1: Operand, op2: Operand): Inst {
        return Inst {
            prev: nil,
            next: nil,
            kind: kind,
            dest: dest,
            ty: ty,
            op1: op1,
            op2: op2,
            sub_op: 0
        };
    }
}

pub const TERM_RET         = 0;
pub const TERM_GOTO        = 1;
pub const TERM_COND_BRANCH = 2;
pub const TERM_UNREACHABLE  = 3;

pub struct Terminator {
    pub kind: i32;
    pub ret_val: Operand;
    pub cond: Operand;
    pub then_block: *u8; // TODO: replace to *BasicBlock
    pub else_block: *u8; // TODO: replace to *BasicBlock
}

impl Terminator {
    pub static func make_ret(val: Operand): Terminator {
        return Terminator {
            kind: TERM_RET,
            ret_val: val,
            cond: Operand.from_local(0u32, nil.(*types.Type)),
            then_block: nil,
            else_block: nil
        };
    }

    pub static func make_goto(target: *BasicBlock): Terminator {
        return Terminator {
            kind: TERM_GOTO,
            ret_val: Operand.from_local(0u32, nil.(*types.Type)),
            cond: Operand.from_local(0u32, nil.(*types.Type)),
            then_block: target.(*u8),
            else_block: nil
        };
    }

    pub static func make_cond_br(cond: Operand, then_b: *BasicBlock, else_b: *BasicBlock): Terminator {
        return Terminator {
            kind: TERM_COND_BRANCH,
            ret_val: Operand.from_local(0u32, nil.(*types.Type)),
            cond: cond,
            then_block: then_b.(*u8),
            else_block: else_b.(*u8)
        };
    }

    pub static func make_unreachable(): Terminator {
        return Terminator {
            kind: TERM_UNREACHABLE,
            ret_val: Operand.from_local(0u32, nil.(*types.Type)),
            cond: Operand.from_local(0u32, nil.(*types.Type)),
            then_block: nil,
            else_block: nil
        };
    }
}

pub struct BasicBlock {
    pub prev: *BasicBlock;
    pub next: *BasicBlock;
    pub name: std.StringView;

    pub inst_head: *Inst;
    pub inst_tail: *Inst;
    pub terminator: Terminator;
}

impl BasicBlock {
    pub static func new(name: std.StringView): BasicBlock {
        return BasicBlock {
            prev: nil,
            next: nil,
            name: name,
            inst_head: nil,
            inst_tail: nil,
            terminator: Terminator.make_unreachable()
        };
    }

    pub func push_inst_back(inst: *Inst) {
        inst.prev = this.inst_tail;
        inst.next = nil;
        if this.inst_tail != nil {
            this.inst_tail.next = inst;
        } else {
            this.inst_head = inst;
        }
        this.inst_tail = inst;
    }

    pub func insert_inst_before(anchor: *Inst, inst: *Inst) {
        if anchor == nil {
            this.push_inst_back(inst);
            return;
        }
        inst.next = anchor;
        inst.prev = anchor.prev;
        if anchor.prev != nil {
            anchor.prev.next = inst;
        } else {
            this.inst_head = inst;
        }
        anchor.prev = inst;
    }

    pub func insert_inst_after(anchor: *Inst, inst: *Inst) {
        if anchor == nil || anchor == this.inst_tail {
            this.push_inst_back(inst);
            return;
        }
        inst.prev = anchor;
        inst.next = anchor.next;
        if anchor.next != nil {
            anchor.next.prev = inst;
        } else {
            this.inst_tail = inst;
        }
        anchor.next = inst;
    }

    pub func remove_inst(inst: *Inst) {
        if inst.prev != nil {
            inst.prev.next = inst.next;
        } else {
            this.inst_head = inst.next;
        }
        if inst.next != nil {
            inst.next.prev = inst.prev;
        } else {
            this.inst_tail = inst.prev;
        }
        inst.prev = nil;
        inst.next = nil;
    }
}

pub struct LocalDecl {
    pub id: u32;
    pub ty: *types.Type;
    pub name: std.StringView;
}

pub struct Function {
    pub def_id: basic.DefId;
    pub name: std.StringView;
    pub ty: *types.Type;

    pub params: **LocalDecl;
    pub params_count: usize;
    pub params_cap: usize;

    pub locals: **LocalDecl;
    pub locals_count: usize;
    pub locals_cap: usize;

    pub block_head: *BasicBlock;
    pub block_tail: *BasicBlock;
}

impl Function {
    pub static func new(def_id: basic.DefId, name: std.StringView, ty: *types.Type,
                        alloc: *mem.ArenaAllocator): *Function {
        let raw = alloc.alloc(@size_of(Function));
        let fn_obj = raw.(*Function);

        let cap = 8uz;
        let locals_buf = alloc.alloc(cap * @size_of(*LocalDecl)).(**LocalDecl);

        *fn_obj = Function {
            def_id: def_id,
            name: name,
            ty: ty,
            locals: locals_buf,
            locals_count: 0uz,
            locals_cap: cap,
            block_head: nil,
            block_tail: nil
        };
        return fn_obj;
    }

    pub func alloc_local(alloc: *mem.ArenaAllocator, ty: *types.Type, name: std.StringView): u32 {
        let id = this.locals_count.(u32);
        let raw = alloc.alloc(@size_of(LocalDecl));
        let decl = raw.(*LocalDecl);
        *decl = LocalDecl { id: id, ty: ty, name: name };

        if this.locals_count >= this.locals_cap {
            let new_cap = this.locals_cap * 2uz;
            let new_buf = alloc.alloc(new_cap * @size_of(*LocalDecl)).(**LocalDecl);
            for let i = 0uz, i < this.locals_count, i += 1 {
                *(new_buf + i) = *(this.locals + i);
            }
            this.locals = new_buf;
            this.locals_cap = new_cap;
        }

        *(this.locals + this.locals_count) = decl;
        this.locals_count += 1;
        return id;
    }

    pub func add_param(alloc: *mem.ArenaAllocator, name: std.StringView, ty: *types.Type): u32 {
        let local_id = this.alloc_local(alloc, ty, name);
        let param_decl = *(this.locals + local_id.(usize));

        if this.params_count >= this.params_cap {
            let new_cap = this.params_cap * 2uz;
            let new_buf = alloc.alloc(new_cap * @size_of(*LocalDecl)).(**LocalDecl);
            for let i = 0uz, i < this.params_count, i += 1 {
                *(new_buf + i) = *(this.params + i);
            }
            this.params = new_buf;
            this.params_cap = new_cap;
        }

        *(this.params + this.params_count) = param_decl;
        this.params_count += 1;
        return local_id;
    }

    pub func push_block_back(block: *BasicBlock) {
        block.prev = this.block_tail;
        block.next = nil;
        if this.block_tail != nil {
            this.block_tail.next = block;
        } else {
            this.block_head = block;
        }
        this.block_tail = block;
    }

    pub func insert_block_before(anchor: *BasicBlock, block: *BasicBlock) {
        if anchor == nil {
            this.push_block_back(block);
            return;
        }
        block.next = anchor;
        block.prev = anchor.prev;
        if anchor.prev != nil {
            anchor.prev.next = block;
        } else {
            this.block_head = block;
        }
        anchor.prev = block;
    }

    pub func insert_block_after(anchor: *BasicBlock, block: *BasicBlock) {
        if anchor == nil || anchor == this.block_tail {
            this.push_block_back(block);
            return;
        }
        block.prev = anchor;
        block.next = anchor.next;
        if anchor.next != nil {
            anchor.next.prev = block;
        } else {
            this.block_tail = block;
        }
        anchor.next = block;
    }
}

pub struct Variable {
    pub def_id: basic.DefId;
    pub name: std.StringView;
    pub ty: *types.Type;
    pub init_val: Operand;
    pub is_const: bool;
}

impl Variable {
    pub static func new(def_id: basic.DefId, name: std.StringView, ty: *types.Type,
                        init_val: Operand, is_const: bool): Variable {
        return Variable {
            def_id: def_id,
            name: name,
            ty: ty,
            init_val: init_val,
            is_const: is_const
        };
    }
}

pub struct Context {
    pub alloc: mem.ArenaAllocator;
    globals: **Variable;
    globals_count: usize;
    globals_cap: usize;
    functions: **Function;
    functions_count: usize;
    functions_cap: usize;
}

impl Context {
    pub static func new(): Context {
        let alloc = mem.ArenaAllocator.init(64uz * mem.KB);
        let cap = 16uz;
        return Context {
            alloc: alloc,
            globals: alloc.alloc(cap * @size_of(*Variable)).(**Variable),
            globals_count: 0uz,
            globals_cap: cap,
            functions: alloc.alloc(cap * @size_of(*Function)).(**Function),
            functions_count: 0uz,
            functions_cap: cap
        };
    }

    pub func add_global(def_id: basic.DefId, name: std.StringView, ty: *types.Type,
                        init_val: Operand, is_const: bool): *Variable {
        let raw = this.alloc.alloc(@size_of(Variable));
        let var_ptr = raw.(*Variable);
        *var_ptr = Variable.new(def_id, name, ty, init_val, is_const);

        if this.globals_count >= this.globals_cap {
            let new_cap = this.globals_cap * 2uz;
            let new_buf = this.alloc.alloc(new_cap * @size_of(*Variable)).(**Variable);
            for let i = 0uz, i < this.globals_count, i += 1 {
                *(new_buf + i) = *(this.globals + i);
            }
            this.globals = new_buf;
            this.globals_cap = new_cap;
        }

        *(this.globals + this.globals_count) = var_ptr;
        this.globals_count += 1;
        return var_ptr;
    }

    pub func add_function(fn_obj: *Function) {
        if this.functions_count >= this.functions_cap {
            let new_cap = this.functions_cap * 2uz;
            let new_buf = this.alloc.alloc(new_cap * @size_of(*Function)).(**Function);
            for let i = 0uz, i < this.functions_count, i += 1 {
                *(new_buf + i) = *(this.functions + i);
            }
            this.functions = new_buf;
            this.functions_cap = new_cap;
        }

        *(this.functions + this.functions_count) = fn_obj;
        this.functions_count += 1;
    }
}

pub struct Builder {
    ctx: *Context;
    current_fn: *Function;
    current_block: *BasicBlock;
}

impl Builder {
    pub static func new(ctx: *Context): Builder {
        return Builder {
            ctx: ctx,
            current_fn: nil,
            current_block: nil
        };
    }

    pub func create_global(def_id: basic.DefId, name: std.StringView, ty: *types.Type,
                          init_val: Operand, is_const: bool): *Variable {
        return this.ctx.add_global(def_id, name, ty, init_val, is_const);
    }

    pub func create_func(def_id: basic.DefId, name: std.StringView, ty: *types.Type): *Function {
        let fn_obj = Function.new(def_id, name, ty, &this.ctx.alloc);
        this.ctx.add_function(fn_obj);
        this.current_fn = fn_obj;
        return fn_obj;
    }

    pub func add_param(name: std.StringView, ty: *types.Type): u32 {
        if this.current_fn == nil {
            std.panic("Builder error: current_fn is nil when adding parameter");
        }
        return this.current_fn.add_param(&this.ctx.alloc, name, ty);
    }

    pub func create_local(name: std.StringView, ty: *types.Type): u32 {
        if this.current_fn == nil {
            std.panic("Builder error: current_fn is nil when creating local variable");
        }
        return this.current_fn.alloc_local(&this.ctx.alloc, ty, name);
    }
    pub func create_block(name: std.StringView): *BasicBlock {
        let raw = this.ctx.alloc.alloc(@size_of(BasicBlock));
        let bb = raw.(*BasicBlock);
        *bb = BasicBlock.new(name);
        if this.current_fn != nil {
            this.current_fn.push_block_back(bb);
        }
        return bb;
    }

    pub func emit_load(dest_local: u32, src_ptr: Operand): *Inst {
        let raw = this.ctx.alloc.alloc(@size_of(Inst));
        let inst = raw.(*Inst);
        *inst = Inst.new(INST_LOAD, dest_local, src_ptr.ty, src_ptr, Operand.from_local(0u32, nil.(*types.Type)));
        this.current_block.push_inst_back(inst);
        return inst;
    }

    pub func emit_store(dest_ptr: Operand, val: Operand): *Inst {
        let raw = this.ctx.alloc.alloc(@size_of(Inst));
        let inst = raw.(*Inst);
        *inst = Inst.new(INST_STORE, 0u32, nil.(*types.Type), dest_ptr, val);
        this.current_block.push_inst_back(inst);
        return inst;
    }

    pub func emit_ret(val: Operand) {
        this.current_block.terminator = Terminator.make_ret(val);
    }

    pub func emit_goto(target: *BasicBlock) {
        this.current_block.terminator = Terminator.make_goto(target);
    }

    pub func emit_cond_br(cond: Operand, then_b: *BasicBlock, else_b: *BasicBlock) {
        this.current_block.terminator = Terminator.make_cond_br(cond, then_b, else_b);
    }

    pub func emit_unreachable() {
        this.current_block.terminator = Terminator.make_unreachable();
    }
}
