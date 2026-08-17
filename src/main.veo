import std.math;
import std;
import basic;
import lexer;
import std.fs;
import std.mem;
import std.io;
import std.sys;
import types;
import ast;
import hir;
import sema;
import llvm.bindings;
import codegen;

let emit_ir  = true;
let emit_asm = false;

func main(): i32 {
    let main_file_name = std.StringView.from("src/tests/var_decl.veo");
    let main_file = fs.File.open(main_file_name, "r");
    if !main_file.is_open() {
        std.panic("Cannot open file src/main.veo");
    }
    let content   = main_file.read_all();
    let mgr       = basic.SourceMgr.new();
    let buffer_id = mgr.add_buffer(main_file_name, content); // [OWNERSHIP: ACQUIRE]
    let mod_id    = basic.hash64("main", 4uz);

    let lex       = lexer.Lexer.new(mgr, buffer_id);
    let ty_ctx    = types.Context.new();
    let ast_ctx   = ast.Context.new();
    let parser    = ast.Parser.new(&lex, &ty_ctx, &ast_ctx);
    let parse_res = parser.parse();

    let dumper: ast.Dumper;
    dumper.dump(parse_res);

    let hir_ctx     = hir.Context.new();
    let hir_builder = hir.Builder.new(&hir_ctx);
    let sema_ctx    = sema.Context.new();
    let resolver    = sema.NameResolver.new(&sema_ctx);
    resolver.resolve(parse_res);
    sema_ctx.dump_resolutions();

    /*
    bindings.init_llvm();

    let target: bindings.LLVMTargetRef;
    let target_err: *u8;
    let triple_str = "x86_64-pc-linux-gnu";

    if bindings.LLVMGetTargetFromTriple(triple_str, &target, &target_err) {
        io.print("\033[31mError looking up target: \033[0m");
        io.println(target_err);
        bindings.LLVMDisposeMessage(target_err);
        mgr.destroy();
        return 1;
    }

    let cpu            = "generic";
    let features       = "";
    let opt_level      = bindings.LLVMCodeGenOptLevel.none();
    let reloc          = bindings.LLVMRelocMode.pic();
    let code_model     = bindings.LLVMCodeModel.default();
    let target_machine = bindings.LLVMCreateTargetMachine(
        target, triple_str, cpu, features, opt_level, reloc, code_model
    );
    let data_layout    = bindings.LLVMCreateTargetDataLayout(target_machine);

    let gen    = codegen.CodeGen.new("test_mod", &sym_table, &hir_ctx, triple_str, data_layout);
    let module = gen.generate();

    let print_mod_err_msg: *u8;
    if emit_ir && bindings.LLVMPrintModuleToFile(module, "src/tests/var_decl.ll", &print_mod_err_msg) {
        io.print("\033[31mError on prints LLVM IR to file: \033[0m");
        io.println(print_mod_err_msg);
        bindings.LLVMDisposeMessage(print_mod_err_msg);
        mgr.destroy();
        return 1;
    }

    if !emit_obj_file(module, target_machine, "src/tests/var_decl.o") {
        bindings.LLVMDisposeTargetData(data_layout);
        bindings.LLVMDisposeTargetMachine(target_machine);
        mgr.destroy();
        return 1;
    }

    if emit_asm && !emit_asm_file(module, target_machine, "src/tests/var_decl.s") {
        bindings.LLVMDisposeTargetData(data_layout);
        bindings.LLVMDisposeTargetMachine(target_machine);
        mgr.destroy();
        return 1;
    }

    sys.system("clang src/tests/var_decl.o -o src/tests/var_decl");

    bindings.LLVMDisposeTargetData(data_layout);
    bindings.LLVMDisposeTargetMachine(target_machine);
    */
    mgr.destroy();
    return 0;
}

func emit_file(
    module: bindings.LLVMModuleRef,
    target_machine: bindings.LLVMTargetMachineRef,
    file_name: *u8,
    file_type: bindings.LLVMCodeGenFileType): bool {
    let error_msg: *u8;
    if bindings.LLVMTargetMachineEmitToFile(target_machine, module, file_name, file_type, &error_msg) {
        io.print("\033[31mCould not emit file ");
        io.print(file_name);
        io.print(": ");
        io.print(error_msg);
        io.println("\033[0m");
        bindings.LLVMDisposeMessage(error_msg);
        return false;
    }
    return true;
}

func emit_obj_file(
    module: bindings.LLVMModuleRef,
    target_machine: bindings.LLVMTargetMachineRef,
    file_name: *u8): bool {
    return emit_file(module, target_machine, file_name, bindings.LLVMCodeGenFileType.object_file());
}

func emit_asm_file(
    module: bindings.LLVMModuleRef,
    target_machine: bindings.LLVMTargetMachineRef,
    file_name: *u8): bool {
    return emit_file(module, target_machine, file_name, bindings.LLVMCodeGenFileType.assembly());
}
