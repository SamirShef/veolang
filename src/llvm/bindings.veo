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

/*
 * opaque struct
 */
pub struct LLVMBasicBlockRef {
    ptr: *u8;
}

pub const LLVMIntEQ  = 32;
pub const LLVMIntNE  = 33;
pub const LLVMIntUGT = 34;
pub const LLVMIntUGE = 35;
pub const LLVMIntULT = 36;
pub const LLVMIntULE = 37;
pub const LLVMIntSGT = 38;
pub const LLVMIntSGE = 39;
pub const LLVMIntSLT = 40;
pub const LLVMIntSLE = 41;

pub struct LLVMIntPredicate {
    pred: i32;
}

impl LLVMIntPredicate {
    pub static func eq(): LLVMIntPredicate {
        return LLVMIntPredicate { pred: LLVMIntEQ };
    }

    pub static func ne(): LLVMIntPredicate {
        return LLVMIntPredicate { pred: LLVMIntNE };
    }

    pub static func ugt(): LLVMIntPredicate {
        return LLVMIntPredicate { pred: LLVMIntUGT };
    }

    pub static func uge(): LLVMIntPredicate {
        return LLVMIntPredicate { pred: LLVMIntUGE };
    }

    pub static func ult(): LLVMIntPredicate {
        return LLVMIntPredicate { pred: LLVMIntULT };
    }

    pub static func ule(): LLVMIntPredicate {
        return LLVMIntPredicate { pred: LLVMIntULE };
    }

    pub static func sgt(): LLVMIntPredicate {
        return LLVMIntPredicate { pred: LLVMIntSGT };
    }

    pub static func sge(): LLVMIntPredicate {
        return LLVMIntPredicate { pred: LLVMIntSGE };
    }

    pub static func slt(): LLVMIntPredicate {
        return LLVMIntPredicate { pred: LLVMIntSLT };
    }

    pub static func sle(): LLVMIntPredicate {
        return LLVMIntPredicate { pred: LLVMIntSLE };
    }
}

// LLVMRealPredicate
pub const LLVMRealOEQ = 1;
pub const LLVMRealOGT = 2;
pub const LLVMRealOGE = 3;
pub const LLVMRealOLT = 4;
pub const LLVMRealOLE = 5;
pub const LLVMRealONE = 6;

pub struct LLVMRealPredicate {
    pred: i32;
}

impl LLVMRealPredicate {
    pub static func oeq(): LLVMRealPredicate {
        return LLVMRealPredicate { pred: LLVMRealOEQ };
    }

    pub static func ogt(): LLVMRealPredicate {
        return LLVMRealPredicate { pred: LLVMRealOGT };
    }

    pub static func oge(): LLVMRealPredicate {
        return LLVMRealPredicate { pred: LLVMRealOGE };
    }

    pub static func olt(): LLVMRealPredicate {
        return LLVMRealPredicate { pred: LLVMRealOLT };
    }

    pub static func ole(): LLVMRealPredicate {
        return LLVMRealPredicate { pred: LLVMRealOLE };
    }

    pub static func one(): LLVMRealPredicate {
        return LLVMRealPredicate { pred: LLVMRealONE };
    }
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

    pub func LLVMFunctionType(ReturnType: LLVMTypeRef, ParamTypes: *LLVMTypeRef,
                              ParamCount: u32, IsVarArg: bool): LLVMTypeRef;

    pub func LLVMAddFunction(M: LLVMModuleRef, Name: *u8, FunctionTy: LLVMTypeRef): LLVMValueRef;

    pub func LLVMCountParams(Fn: LLVMValueRef): u32;

    pub func LLVMGetParams(Fn: LLVMValueRef, Params: *LLVMValueRef);

    pub func LLVMGetParam(Fn: LLVMValueRef, Index: u32): LLVMValueRef;

    pub func LLVMGetParamParent(Inst: LLVMValueRef): LLVMValueRef;

    pub func LLVMGetFirstParam(Fn: LLVMValueRef): LLVMValueRef;

    pub func LLVMGetLastParam(Fn: LLVMValueRef): LLVMValueRef;

    pub func LLVMGetNextParam(Arg: LLVMValueRef): LLVMValueRef;

    pub func LLVMGetPreviousParam(Arg: LLVMValueRef): LLVMValueRef;

    pub func LLVMAppendBasicBlockInContext(C: LLVMContextRef, Fn: LLVMValueRef,
                                           Name: *u8): LLVMBasicBlockRef;

    pub func LLVMPositionBuilderAtEnd(Builder: LLVMBuilderRef, Block: LLVMBasicBlockRef);

    pub func LLVMBuildRetVoid(Builder: LLVMBuilderRef): LLVMValueRef;

    pub func LLVMBuildRet(Builder: LLVMBuilderRef, V: LLVMValueRef): LLVMValueRef;

    pub func LLVMConstNull(Ty: LLVMTypeRef): LLVMValueRef;

    pub func LLVMBuildAdd(Builder: LLVMBuilderRef, LHS: LLVMValueRef, RHS: LLVMValueRef,
                          Name: *u8): LLVMValueRef;
    pub func LLVMBuildFAdd(Builder: LLVMBuilderRef, LHS: LLVMValueRef, RHS: LLVMValueRef,
                          Name: *u8): LLVMValueRef;

    pub func LLVMBuildSub(B: LLVMBuilderRef, LHS: LLVMValueRef, RHS: LLVMValueRef,
                          Name: *u8): LLVMValueRef;
    pub func LLVMBuildFSub(B: LLVMBuilderRef, LHS: LLVMValueRef, RHS: LLVMValueRef,
                           Name: *u8): LLVMValueRef;

    pub func LLVMBuildMul(B: LLVMBuilderRef, LHS: LLVMValueRef, RHS: LLVMValueRef,
                          Name: *u8): LLVMValueRef;
    pub func LLVMBuildFMul(B: LLVMBuilderRef, LHS: LLVMValueRef, RHS: LLVMValueRef,
                           Name: *u8): LLVMValueRef;

    pub func LLVMBuildUDiv(B: LLVMBuilderRef, LHS: LLVMValueRef, RHS: LLVMValueRef,
                           Name: *u8): LLVMValueRef;
    pub func LLVMBuildSDiv(B: LLVMBuilderRef, LHS: LLVMValueRef, RHS: LLVMValueRef,
                           Name: *u8): LLVMValueRef;
    pub func LLVMBuildFDiv(B: LLVMBuilderRef, LHS: LLVMValueRef, RHS: LLVMValueRef,
                           Name: *u8): LLVMValueRef;

    pub func LLVMBuildURem(B: LLVMBuilderRef, LHS: LLVMValueRef, RHS: LLVMValueRef,
                           Name: *u8): LLVMValueRef;
    pub func LLVMBuildSRem(B: LLVMBuilderRef, LHS: LLVMValueRef, RHS: LLVMValueRef,
                           Name: *u8): LLVMValueRef;
    pub func LLVMBuildFRem(B: LLVMBuilderRef, LHS: LLVMValueRef, RHS: LLVMValueRef,
                           Name: *u8): LLVMValueRef;

    pub func LLVMBuildAnd(B: LLVMBuilderRef, LHS: LLVMValueRef, RHS: LLVMValueRef,
                          Name: *u8): LLVMValueRef;
    pub func LLVMBuildOr(B: LLVMBuilderRef, LHS: LLVMValueRef, RHS: LLVMValueRef,
                         Name: *u8): LLVMValueRef;
    pub func LLVMBuildXor(B: LLVMBuilderRef, LHS: LLVMValueRef, RHS: LLVMValueRef,
                          Name: *u8): LLVMValueRef;

    pub func LLVMBuildICmp(B: LLVMBuilderRef, Op: LLVMIntPredicate, LHS: LLVMValueRef,
                           RHS: LLVMValueRef, Name: *u8): LLVMValueRef;
    pub func LLVMBuildFCmp(B: LLVMBuilderRef, Op: LLVMRealPredicate, LHS: LLVMValueRef,
                           RHS: LLVMValueRef, Name: *u8): LLVMValueRef;

    pub func LLVMBuildNeg(B: LLVMBuilderRef, V: LLVMValueRef, Name: *u8): LLVMValueRef;

    pub func LLVMBuildFNeg(B: LLVMBuilderRef, V: LLVMValueRef, Name: *u8): LLVMValueRef;

    pub func LLVMBuildNot(B: LLVMBuilderRef, V: LLVMValueRef, Name: *u8): LLVMValueRef;

    pub func LLVMTypeOf(Val: LLVMValueRef): LLVMTypeRef;
}

pub func init_llvm() {
    __veo_initialize_llvm();
}
