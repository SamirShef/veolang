import std.mem;
import std;
import llvm.smloc;
import basic;
import types;

pub const ACCESS_PRIV = 0;
pub const ACCESS_PUB  = 1;

pub const NODE_STMT_START =   0;
pub const NODE_VAR_DECL   =   0;
pub const NODE_STMT_END   = 100;

pub const NODE_EXPR_START = 101;
pub const NODE_LIT_EXPR   = 101;
pub const NODE_BIN_EXPR   = 102;
pub const NODE_EXPR_END   = 200;

pub struct Node {
    kind: i32;
    range: basic.Span;
}

impl Node {
    pub static func new(kind: i32, range: basic.Span): Node {
        return Node {
            kind: kind,
            range: range
        };
    }

    pub func kind(): i32 {
        return this.kind;
    }

    pub func range(): basic.Span {
        return this.range;
    }
}

pub struct Stmt {
    base: Node;
    access: i32;
}

impl Stmt {
    pub static func new(kind: i32, range: basic.Span): Stmt {
        return Stmt {
            base: Node.new(kind, range),
            access: ACCESS_PRIV
        };
    }

    pub static func new(access: i32, kind: i32, range: basic.Span): Stmt {
        return Stmt {
            base: Node.new(kind, range),
            access: access
        };
    }

    pub static func isa(node: *Node): bool {
        if node == nil {
            return false;
        }
        return node.kind() >= NODE_STMT_START && node.kind() <= NODE_STMT_END;
    }

    pub static func cast(node: *Node): *Stmt {
        if !Stmt.isa(node) {
            std.panic("RTTI Error: Failed cast to *Stmt");
        }
        return node.(*Stmt);
    }
}

pub struct Expr {
    base: Node;
}

impl Expr {
    pub static func new(kind: i32, range: basic.Span): Expr {
        return Expr {
            base: Node.new(kind, range)
        };
    }

    pub static func isa(node: *Node): bool {
        if node == nil {
            return false;
        }
        return node.kind() >= NODE_EXPR_START && node.kind() <= NODE_EXPR_END;
    }

    pub static func cast(node: *Node): *Expr {
        if !Expr.isa(node) {
            std.panic("RTTI Error: Failed cast to *Expr");
        }
        return node.(*Expr);
    }
}

pub struct VarDecl {
    pub base: Stmt;
    pub name: std.StringView;
    pub ty: *types.Type;
    pub init: *Expr;
}

impl VarDecl {
    pub static func alloc(alloc: mem.Allocator, range: basic.Span, name: std.StringView,
                          type: *types.Type, init: *Expr): *VarDecl {
        let mem_ptr = alloc.alloc(@size_of(VarDecl));
        let node = mem_ptr.(*VarDecl);

        node.base = Stmt.new(NODE_VAR_DECL, range);
        node.name = name;
        node.ty   = type;
        node.init = init;

        return node;
    }

    pub static func isa(node: *Node): bool {
        if node == nil {
            return false;
        }
        return node.kind() == NODE_VAR_DECL;
    }

    pub static func cast(node: *Node): *VarDecl {
        if !VarDecl.isa(node) {
            std.panic("RTTI Error: Failed cast to *VarDecl");
        }
        return node.(*VarDecl);
    }
}
