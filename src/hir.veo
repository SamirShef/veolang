import std.math;
import std.mem;
import std.sys;
import std;
import types;
import llvm.smloc;
import basic;

pub const NODE_VARIABLE = 0;
pub const NODE_LITERAL  = 1;
pub const NODE_BINARY   = 2;
pub const NODE_UNARY    = 3;
pub const NODE_LOAD     = 4;

pub struct Node {
    kind: i32;
}

impl Node {
    pub static func new(kind: i32): Node {
        return Node { kind: kind };
    }

    pub func kind(): i32 {
        return this.kind;
    }
}

pub struct Variable {
    pub base: Node;
    pub id: basic.DefId;
    pub name: std.String;
    pub ty: *types.Type;
    pub init: *Node;
    pub next: *Variable;
}

impl Variable {
    pub static func isa(node: *Node): bool {
        if node == nil {
            return false;
        }
        return node.kind() == NODE_VARIABLE;
    }

    pub static func cast(node: *Node): *Variable {
        if !Variable.isa(node) {
            std.panic("RTTI Error: Failed cast to *Variable");
        }
        return node.(*Variable);
    }
}

pub struct Literal {
    pub base: Node;
    pub val: basic.Value;
}

impl Literal {
    pub static func isa(node: *Node): bool {
        if node == nil {
            return false;
        }
        return node.kind() == NODE_LITERAL;
    }

    pub static func cast(node: *Node): *Literal {
        if !Literal.isa(node) {
            std.panic("RTTI Error: Failed cast to *Literal");
        }
        return node.(*Literal);
    }
}

pub struct Binary {
    pub base: Node;
    pub op: i32; // ast.BinOp
    pub left: *Node;
    pub right: *Node;
    pub common_ty: *types.Type;
}

impl Binary {
    pub static func isa(node: *Node): bool {
        if node == nil {
            return false;
        }
        return node.kind() == NODE_BINARY;
    }

    pub static func cast(node: *Node): *Binary {
        if !Binary.isa(node) {
            std.panic("RTTI Error: Failed cast to *Binary");
        }
        return node.(*Binary);
    }
}

pub struct Unary {
    pub base: Node;
    pub op: i32; // ast.UnOp
    pub right: *Node;
    pub common_ty: *types.Type;
}

impl Unary {
    pub static func isa(node: *Node): bool {
        if node == nil {
            return false;
        }
        return node.kind() == NODE_UNARY;
    }

    pub static func cast(node: *Node): *Unary {
        if !Unary.isa(node) {
            std.panic("RTTI Error: Failed cast to *Unary");
        }
        return node.(*Unary);
    }
}

pub struct Load {
    pub base: Node;
    pub id: basic.DefId;
    pub ty: *types.Type;
}

impl Load {
    pub static func isa(node: *Node): bool {
        if node == nil {
            return false;
        }
        return node.kind() == NODE_LOAD;
    }

    pub static func cast(node: *Node): *Load {
        if !Load.isa(node) {
            std.panic("RTTI Error: Failed cast to *Load");
        }
        return node.(*Load);
    }
}

pub struct Context {
    alloc: *mem.ArenaAllocator;
    global_vars_start: *Variable;
    global_vars_end: *Variable;
}

impl Context {
    pub static func new(alloc: *mem.ArenaAllocator): Context {
        return Context {
            alloc: alloc,
            global_vars_start: nil,
            global_vars_end: nil
        };
    }

    pub func alloc_node(kind: i32): *Node {
        let size = 0uz;
        if kind == NODE_VARIABLE {
            size = @size_of(Variable);
        } else if kind == NODE_LITERAL {
            size = @size_of(Literal);
        } else if kind == NODE_BINARY {
            size = @size_of(Binary);
        } else if kind == NODE_UNARY {
            size = @size_of(Unary);
        } else if kind == NODE_LOAD {
            size = @size_of(Load);
        }
        if size == 0uz {
            std.panic("Cannot allocate hir.Node (unsupported node kind)");
            return nil;
        }
        let mem = this.alloc.alloc(size);
        let node = mem.(*Node);
        *node = Node.new(kind);
        return node;
    }

    pub func add_global_var(var: *Variable) {
        if this.global_vars_start == nil {
            this.global_vars_start = var;
            this.global_vars_end   = var;
        } else {
            this.global_vars_end.next = var;
            this.global_vars_end      = var;
        }
    }

    pub func global_vars_start(): *Variable {
        return this.global_vars_start;
    }
}

pub struct Builder {
    ctx: *Context;
}

impl Builder {
    pub static func new(ctx: *Context): Builder {
        return Builder { ctx: ctx };
    }

    pub func create_variable(alloc: mem.Allocator, id: basic.DefId, name: std.StringView,
                             ty: *types.Type, init: *Node): *Variable {
        let var  = this.ctx.alloc_node(NODE_VARIABLE).(*Variable);
        var.id   = id;
        var.name = std.String.from(alloc, name);
        var.ty   = ty;
        var.init = init;
        this.ctx.add_global_var(var);
        return var;
    }

    pub func create_literal(val: basic.Value): *Literal {
        let lit = this.ctx.alloc_node(NODE_LITERAL).(*Literal);
        lit.val = val;
        return lit;
    }

    pub func create_binary(op: i32, left: *Node, right: *Node, common_ty: *types.Type): *Binary {
        let bin       = this.ctx.alloc_node(NODE_BINARY).(*Binary);
        bin.op        = op;
        bin.left      = left;
        bin.right     = right;
        bin.common_ty = common_ty;
        return bin;
    }

    pub func create_unary(op: i32, right: *Node, common_ty: *types.Type): *Unary {
        let un       = this.ctx.alloc_node(NODE_UNARY).(*Unary);
        un.op        = op;
        un.right     = right;
        un.common_ty = common_ty;
        return un;
    }

    pub func create_load(id: basic.DefId, ty: *types.Type): *Load {
        let load = this.ctx.alloc_node(NODE_LOAD).(*Load);
        load.id  = id;
        load.ty  = ty;
        return load;
    }
}
