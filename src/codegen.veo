import std.math;
import std.mem;
import std.sys;
import std;
import llvm.smloc;
import types;
import basic;
import symbols;
import hir;
import llvm.bindings;

pub struct CodeGen {
    sym_table: *symbols.SymbolTable;
    hir_ctx: *hir.Context;
    module: bindings.LLVMModuleRef;
    ctx: bindings.LLVMContextRef;
    builder: bindings.LLVMBuilderRef;
}

impl CodeGen {
    pub static func new(name: *u8, sym_table: *symbols.SymbolTable, hir_ctx: *hir.Context): CodeGen {
        let ctx     = bindings.LLVMContextCreate();
        let module  = bindings.LLVMModuleCreateWithNameInContext(name, ctx);
        let builder = bindings.LLVMCreateBuilderInContext(ctx);
        return CodeGen {
            sym_table: sym_table,
            hir_ctx: hir_ctx,
            module: module,
            ctx: ctx,
            builder: builder
        };
    }

    pub func dump_mod() {
        bindings.LLVMDumpModule(this.module);
    }

    pub func generate() {
        let vars = this.hir_ctx.global_vars_start();
        this.generate_global_variables(vars);
    }

    func generate_global_variables(start: *hir.Variable) {
        if start == nil {
            return;
        }
        for start != nil {
            this.generate_variable(start);
            start = start.next;
        }
    }

    func generate_variable(var: *hir.Variable) {
        let ty = this.generate_ty(var.ty);
        let glob = bindings.LLVMAddGlobal(this.module, ty, var.name.data());
    }

    func generate_expr(expr: *hir.Node): bindings.LLVMValueRef {
        std.panic("Generating expressions does not implemented");
        let null: bindings.LLVMValueRef;
        return null;
    }

    func generate_ty(ty: *types.Type): bindings.LLVMTypeRef {
        let kind = ty.kind();
        if kind != types.TYPE_INT {
            std.panic("Generating of non-integer types does not implemented");
        }
        let int_ty = types.IntType.cast(ty);
        return bindings.LLVMIntTypeInContext(this.ctx, int_ty.width);
    }
}
