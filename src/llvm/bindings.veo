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

extern "C" {
    pub func LLVMContextCreate(): LLVMContextRef;

    pub func LLVMModuleCreateWithNameInContext(ModuleID: *u8, C: LLVMContextRef): LLVMModuleRef;

    pub func LLVMCreateBuilderInContext(C: LLVMContextRef): LLVMBuilderRef;

    pub func LLVMInt32TypeInContext(C: LLVMContextRef): LLVMTypeRef;

    pub func LLVMIntTypeInContext(C: LLVMContextRef, NumBits: u32): LLVMTypeRef;

    pub func LLVMAddGlobal(M: LLVMModuleRef, Ty: LLVMTypeRef, Name: *u8): LLVMValueRef;

    pub func LLVMSetInitalizer(GlobalVar: LLVMValueRef, ConstantVal: LLVMValueRef);

    pub func LLVMConstInt(IntTy: LLVMTypeRef, N: u64, SignExtend: bool): LLVMValueRef;

    pub func LLVMBuildLoad2(B: LLVMBuilderRef, Ty: LLVMTypeRef, PointerVal: LLVMValueRef, Name: *u8): LLVMValueRef;

    pub func LLVMDumpModule(M: LLVMModuleRef);
}
