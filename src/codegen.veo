import std.mem;
import std.sys;
import std;
import llvm.smloc;
import types;
import basic;
import symbols;
import hir;

pub struct CodeGen {
    sym_table: *symbols.SymbolTable;
    hir_ctx: *hir.Context;
}

impl CodeGen {
    pub static func new(sym_table: *symbols.SymbolTable, hir_ctx: *hir.Context): CodeGen {
        return CodeGen {
            sym_table: sym_table,
            hir_ctx: hir_ctx
        };
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

    }
}
