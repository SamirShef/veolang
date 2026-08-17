import std.math;
import std.mem;
import std.sys;
import std;
import types;
import basic;
import hir;
import llvm.bindings;
import lexer;
import ast;

/*

pub struct CodeGen {
    sym_table: *symbols.SymbolTable;
    hir_ctx: *hir.Context;
    module: bindings.LLVMModuleRef;
    ctx: bindings.LLVMContextRef;
    builder: bindings.LLVMBuilderRef;
}

impl CodeGen {
    pub static func new(name: *u8, sym_table: *symbols.SymbolTable, hir_ctx: *hir.Context,
                        triple: *u8, data_layout: bindings.LLVMTargetDataRef): CodeGen {
        let ctx     = bindings.LLVMContextCreate();
        let module  = bindings.LLVMModuleCreateWithNameInContext(name, ctx);
        let builder = bindings.LLVMCreateBuilderInContext(ctx);
        bindings.LLVMSetTarget(module, triple);
        bindings.LLVMSetModuleDataLayout(module, data_layout);
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

    pub func generate(): bindings.LLVMModuleRef {
        let vars = this.hir_ctx.global_vars_start();
        this.generate_global_variables(vars);
        this.generate_implicit_main();
        return this.module;
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
        let ty   = this.generate_ty(var.ty);
        let glob = bindings.LLVMAddGlobal(this.module, ty, var.name.data());
        let init = var.init != nil ? this.generate_expr(var.init)
                                   : bindings.LLVMConstNull(ty);
        bindings.LLVMSetInitializer(glob, init);
    }

    func generate_expr(expr: *hir.Node): bindings.LLVMValueRef {
        let null: bindings.LLVMValueRef;
        if expr == nil {
            return null;
        }

        let kind = expr.kind();
        if kind == hir.NODE_LITERAL {
            return this.generate_literal(hir.Literal.cast(expr));
        } else if kind == hir.NODE_BINARY {
            return this.generate_binary(hir.Binary.cast(expr));
        } else if kind == hir.NODE_UNARY {
            return this.generate_unary(hir.Unary.cast(expr));
        }
        std.panic("Generating non-literal expressions does not implemented");
        return null;
    }

    func generate_literal(lit: *hir.Literal): bindings.LLVMValueRef {
        let val = lit.val;
        let ty = val.ty;
        if types.IntType.isa(ty) {
            let int = types.IntType.cast(ty);
            let llvm_ty = this.generate_ty(ty);
            return bindings.LLVMConstInt(llvm_ty, val.as_int, !int.is_unsigned);
        }
        std.panic("Generating non-integer literals does not implemented");
        let null: bindings.LLVMValueRef;
        return null;
    }

    func generate_binary(bin: *hir.Binary): bindings.LLVMValueRef {
        let op        = bin.op;
        let left      = this.generate_expr(bin.left);
        let right     = this.generate_expr(bin.right);
        let common_ty = bin.common_ty;

        let is_float    = types.FloatType.isa(common_ty);
        let is_unsigned = types.IntType.isa(common_ty)
            ? types.IntType.cast(common_ty).is_unsigned
            : (types.SizeType.isa(common_ty) ? types.SizeType.cast(common_ty).is_unsigned : false);

        if op == ast.BIN_OP_PLUS {
            if is_float {
                return bindings.LLVMBuildFAdd(this.builder, left, right, "fadd.tmp");
            }
            return bindings.LLVMBuildAdd(this.builder, left, right, "add.tmp");
        } else if op == ast.BIN_OP_MINUS {
            if is_float {
                return bindings.LLVMBuildFSub(this.builder, left, right, "fsub.tmp");
            }
            return bindings.LLVMBuildSub(this.builder, left, right, "sub.tmp");
        } else if op == ast.BIN_OP_MUL {
            if is_float {
                return bindings.LLVMBuildFMul(this.builder, left, right, "fmul.tmp");
            }
            return bindings.LLVMBuildMul(this.builder, left, right, "mul.tmp");
        } else if op == ast.BIN_OP_DIV {
            if is_float {
                return bindings.LLVMBuildFDiv(this.builder, left, right, "fdiv.tmp");
            } else if is_unsigned {
                return bindings.LLVMBuildUDiv(this.builder, left, right, "udiv.tmp");
            }
            return bindings.LLVMBuildSDiv(this.builder, left, right, "sdiv.tmp");
        } else if op == ast.BIN_OP_REM {
            if is_float {
                return bindings.LLVMBuildFRem(this.builder, left, right, "frem.tmp");
            } else if is_unsigned {
                return bindings.LLVMBuildURem(this.builder, left, right, "urem.tmp");
            }
            return bindings.LLVMBuildSRem(this.builder, left, right, "srem.tmp");
        } else if op == ast.BIN_OP_EQ {
            if is_float {
                return bindings.LLVMBuildFCmp(this.builder, bindings.LLVMRealPredicate.oeq(), left, right, "fcmp.oeq.tmp");
            }
            return bindings.LLVMBuildICmp(this.builder, bindings.LLVMIntPredicate.eq(), left, right, "icmp.eq.tmp");
        } else if op == ast.BIN_OP_NOT_EQ {
            if is_float {
                return bindings.LLVMBuildFCmp(this.builder, bindings.LLVMRealPredicate.one(), left, right, "fcmp.one.tmp");
            }
            return bindings.LLVMBuildICmp(this.builder, bindings.LLVMIntPredicate.ne(), left, right, "icmp.ne.tmp");
        } else if op == ast.BIN_OP_LT {
            if is_float {
                return bindings.LLVMBuildFCmp(this.builder, bindings.LLVMRealPredicate.olt(), left, right, "fcmp.olt.tmp");
            } else if is_unsigned {
                return bindings.LLVMBuildICmp(this.builder, bindings.LLVMIntPredicate.ult(), left, right, "icmp.ult.tmp");
            }
            return bindings.LLVMBuildICmp(this.builder, bindings.LLVMIntPredicate.slt(), left, right, "icmp.slt.tmp");
        } else if op == ast.BIN_OP_LT_EQ {
            if is_float {
                return bindings.LLVMBuildFCmp(this.builder, bindings.LLVMRealPredicate.ole(), left, right, "fcmp.ole.tmp");
            } else if is_unsigned {
                return bindings.LLVMBuildICmp(this.builder, bindings.LLVMIntPredicate.ule(), left, right, "icmp.ule.tmp");
            }
            return bindings.LLVMBuildICmp(this.builder, bindings.LLVMIntPredicate.sle(), left, right, "icmp.sle.tmp");
        } else if op == ast.BIN_OP_GT {
            if is_float {
                return bindings.LLVMBuildFCmp(this.builder, bindings.LLVMRealPredicate.ogt(), left, right, "fcmp.ogt.tmp");
            } else if is_unsigned {
                return bindings.LLVMBuildICmp(this.builder, bindings.LLVMIntPredicate.ugt(), left, right, "icmp.ugt.tmp");
            }
            return bindings.LLVMBuildICmp(this.builder, bindings.LLVMIntPredicate.sgt(), left, right, "icmp.sgt.tmp");
        } else if op == ast.BIN_OP_GT_EQ {
            if is_float {
                return bindings.LLVMBuildFCmp(this.builder, bindings.LLVMRealPredicate.oge(), left, right, "fcmp.oge.tmp");
            } else if is_unsigned {
                return bindings.LLVMBuildICmp(this.builder, bindings.LLVMIntPredicate.uge(), left, right, "icmp.uge.tmp");
            }
            return bindings.LLVMBuildICmp(this.builder, bindings.LLVMIntPredicate.sge(), left, right, "icmp.sge.tmp");
        } else if op == ast.BIN_OP_BIT_AND || op == ast.BIN_OP_LOG_AND {
            return bindings.LLVMBuildAnd(this.builder, left, right, "and.tmp");
        } else if op == ast.BIN_OP_BIT_OR || op == ast.BIN_OP_LOG_OR {
            return bindings.LLVMBuildOr(this.builder, left, right, "or.tmp");
        } else if op == ast.BIN_OP_BIT_XOR {
            return bindings.LLVMBuildXor(this.builder, left, right, "xor.tmp");
        }
        std.panic("Generating binary expression with unsupported operator");
        let null: bindings.LLVMValueRef;
        return null;
    }

    func generate_unary(un: *hir.Unary): bindings.LLVMValueRef {
        let op        = un.op;
        let right     = this.generate_expr(un.right);
        let common_ty = un.common_ty;

        let is_float = types.FloatType.isa(common_ty);

        if op == ast.UN_OP_MINUS {
            if is_float {
                return bindings.LLVMBuildFNeg(this.builder, right, "fneg.tmp");
            }
            return bindings.LLVMBuildNeg(this.builder, right, "neg.tmp");
        } else if op == ast.UN_OP_NOT {
            return bindings.LLVMBuildNot(this.builder, right, "not.tmp");
        } else if op == ast.UN_OP_INVERSE {
            let llvm_ty = bindings.LLVMTypeOf(right);
            let mask    = bindings.LLVMConstInt(llvm_ty, 0xFFFFFFFFFFFFFFFFu64, true);
            return bindings.LLVMBuildXor(this.builder, right, mask, "xor.tmp");
        }

        std.panic("Generating unary expression with unsupported operator");
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

    func generate_implicit_main() {
        let i32_ty = bindings.LLVMInt32TypeInContext(this.ctx);
        let fn_ty  = bindings.LLVMFunctionType(i32_ty, nil.(*bindings.LLVMTypeRef), 0u32, false);
        let fn     = bindings.LLVMAddFunction(this.module, "main", fn_ty);
        let entry  = bindings.LLVMAppendBasicBlockInContext(this.ctx, fn, "entry");
        bindings.LLVMPositionBuilderAtEnd(this.builder, entry);
        let ret_val = bindings.LLVMConstInt(i32_ty, 0u32, false);
        bindings.LLVMBuildRet(this.builder, ret_val);
    }
}

*/
