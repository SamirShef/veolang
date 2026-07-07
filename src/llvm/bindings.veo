pub const LLVMCodeModelDefault    = 0;
pub const LLVMCodeModelJITDefault = 1;
pub const LLVMCodeModelTiny       = 2;
pub const LLVMCodeModelSmall      = 3;
pub const LLVMCodeModelKernel     = 4;
pub const LLVMCodeModelMedium     = 5;
pub const LLVMCodeModelLarge      = 6;

pub struct LLVMCodeModel {
    model: i32;
}

impl LLVMCodeModel {
    pub static func default(): LLVMCodeModel {
        return LLVMCodeModel { model: LLVMCodeModelDefault };
    }

    pub static func jit_default(): LLVMCodeModel {
        return LLVMCodeModel { model: LLVMCodeModelJITDefault };
    }

    pub static func tiny(): LLVMCodeModel {
        return LLVMCodeModel { model: LLVMCodeModelTiny };
    }

    pub static func small(): LLVMCodeModel {
        return LLVMCodeModel { model: LLVMCodeModelSmall };
    }

    pub static func kernel(): LLVMCodeModel {
        return LLVMCodeModel { model: LLVMCodeModelKernel };
    }

    pub static func medium(): LLVMCodeModel {
        return LLVMCodeModel { model: LLVMCodeModelMedium };
    }

    pub static func large(): LLVMCodeModel {
        return LLVMCodeModel { model: LLVMCodeModelLarge };
    }
}

pub const LLVMAssemblyFile = 0;
pub const LLVMObjectFile   = 1;

pub struct LLVMCodeGenFileType {
    type: i32;
}

impl LLVMCodeGenFileType {
    pub static func assembly(): LLVMCodeGenFileType {
        return LLVMCodeGenFileType { type: LLVMAssemblyFile };
    }

    pub static func object_file(): LLVMCodeGenFileType {
        return LLVMCodeGenFileType { type: LLVMObjectFile };
    }
}

pub const LLVMRelocDefault      = 0;
pub const LLVMRelocStatic       = 1;
pub const LLVMRelocPIC          = 2;
pub const LLVMRelocDynamicNoPic = 3;
pub const LLVMRelocROPI         = 4;
pub const LLVMRelocRWPI         = 5;
pub const LLVMRelocROPI_RWPI    = 6;

pub struct LLVMRelocMode {
    mode: i32;
}

impl LLVMRelocMode {
    pub static func default(): LLVMRelocMode {
        return LLVMRelocMode { mode: LLVMRelocDefault };
    }

    pub static func @static(): LLVMRelocMode {
        return LLVMRelocMode { mode: LLVMRelocStatic };
    }

    pub static func pic(): LLVMRelocMode {
        return LLVMRelocMode { mode: LLVMRelocPIC };
    }

    pub static func dynamic_no_pic(): LLVMRelocMode {
        return LLVMRelocMode { mode: LLVMRelocDynamicNoPic };
    }

    pub static func ropi(): LLVMRelocMode {
        return LLVMRelocMode { mode: LLVMRelocROPI };
    }

    pub static func rwpi(): LLVMRelocMode {
        return LLVMRelocMode { mode: LLVMRelocRWPI };
    }

    pub static func ropi_rwpi(): LLVMRelocMode {
        return LLVMRelocMode { mode: LLVMRelocROPI_RWPI };
    }
}

pub const LLVMCodeGenLevelNone       = 0;
pub const LLVMCodeGenLevelLess       = 1;
pub const LLVMCodeGenLevelDefault    = 2;
pub const LLVMCodeGenLevelAggressive = 3;

pub struct LLVMCodeGenOptLevel {
    level: i32;
}

impl LLVMCodeGenOptLevel {
    pub static func none(): LLVMCodeGenOptLevel {
        return LLVMCodeGenOptLevel { level: LLVMCodeGenLevelNone };
    }

    pub static func less(): LLVMCodeGenOptLevel {
        return LLVMCodeGenOptLevel { level: LLVMCodeGenLevelLess };
    }

    pub static func default(): LLVMCodeGenOptLevel {
        return LLVMCodeGenOptLevel { level: LLVMCodeGenLevelDefault };
    }

    pub static func aggressive(): LLVMCodeGenOptLevel {
        return LLVMCodeGenOptLevel { level: LLVMCodeGenLevelAggressive };
    }
}

/*
 * opaque struct
 */
pub struct LLVMContextRef {
    ptr: *u8;
}

/*
 * opaque struct
 */
pub struct LLVMModuleRef {
    ptr: *u8;
}

/*
 * opaque struct
 */
pub struct LLVMBuilderRef {
    ptr: *u8;
}

/*
 * opaque struct
 */
pub struct LLVMTypeRef {
    ptr: *u8;
}

/*
 * opaque struct
 */
pub struct LLVMValueRef {
    ptr: *u8;
}

/*
 * opaque struct
 */
pub struct LLVMTargetRef {
    ptr: *u8;
}

/*
 * opaque struct
 */
pub struct LLVMTargetDataRef {
    ptr: *u8;
}

/*
 * opaque struct
 */
pub struct LLVMTargetMachineRef {
    ptr: *u8;
}

/*
 * opaque struct
 */
pub struct LLVMPassBuilderOptionsRef {
    ptr: *u8;
}

/*
 * opaque struct
 */
pub struct LLVMErrorRef {
    ptr: *u8;
}

extern "C" {
    pub func LLVMContextCreate(): LLVMContextRef;

    pub func LLVMModuleCreateWithNameInContext(ModuleID: *u8, C: LLVMContextRef): LLVMModuleRef;

    pub func LLVMCreateBuilderInContext(C: LLVMContextRef): LLVMBuilderRef;

    pub func LLVMInt32TypeInContext(C: LLVMContextRef): LLVMTypeRef;

    pub func LLVMIntTypeInContext(C: LLVMContextRef, NumBits: u32): LLVMTypeRef;

    pub func LLVMAddGlobal(M: LLVMModuleRef, Ty: LLVMTypeRef, Name: *u8): LLVMValueRef;

    pub func LLVMSetInitializer(GlobalVar: LLVMValueRef, ConstantVal: LLVMValueRef);

    pub func LLVMConstInt(IntTy: LLVMTypeRef, N: u64, SignExtend: bool): LLVMValueRef;

    pub func LLVMBuildLoad2(B: LLVMBuilderRef, Ty: LLVMTypeRef, PointerVal: LLVMValueRef, Name: *u8): LLVMValueRef;

    pub func LLVMDumpModule(M: LLVMModuleRef);

    pub func LLVMDisposeMessage(Message: *u8);

    pub func LLVMPrintModuleToFile(M: LLVMModuleRef, Filename: *u8, ErrorMessage: **u8): bool;

    pub func LLVMCreatePassBuilderOptions(): LLVMPassBuilderOptionsRef;

    pub func LLVMDisposePassBuilderOptions(Options: LLVMPassBuilderOptionsRef);

    pub func LLVMRunPasses(M: LLVMModuleRef, Passes: *u8, TM: LLVMTargetMachineRef,
                           Options: LLVMPassBuilderOptionsRef): LLVMErrorRef;

    pub func LLVMGetErrorMessage(Err: LLVMErrorRef): *u8;

    pub func LLVMDisposeErrorMessage(Err: LLVMErrorRef);

        func __veo_initialize_llvm();

    pub func LLVMGetTargetFromTriple(Triple: *u8, T: *LLVMTargetRef, ErrorMessage: **u8): bool;

    pub func LLVMCreateTargetDataLayout(T: LLVMTargetMachineRef): LLVMTargetDataRef;

    pub func LLVMDisposeTargetData(TD: LLVMTargetDataRef);

    pub func LLVMCreateTargetMachine(T: LLVMTargetRef, Triple: *u8, CPU: *u8, Features: *u8,
                                     Level: LLVMCodeGenOptLevel, Reloc: LLVMRelocMode,
                                     CodeModel: LLVMCodeModel): LLVMTargetMachineRef;

    pub func LLVMDisposeTargetMachine(T: LLVMTargetMachineRef);

    pub func LLVMTargetMachineEmitToFile(T: LLVMTargetMachineRef, M: LLVMModuleRef, Filename: *u8,
                                         codegen: LLVMCodeGenFileType, ErrorMessage: **u8): bool;

    pub func LLVMSetTarget(M: LLVMModuleRef, Triple: *u8);

    pub func LLVMSetModuleDataLayout(M: LLVMModuleRef, DL: LLVMTargetDataRef);
}

pub func init_llvm() {
    __veo_initialize_llvm();
}
