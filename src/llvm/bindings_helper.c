#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>

void
__veo_initialize_llvm () {
    LLVMInitializeAllTargetInfos ();
    LLVMInitializeAllTargets ();
    LLVMInitializeAllTargetMCs ();
    LLVMInitializeAllAsmParsers ();
    LLVMInitializeAllAsmPrinters ();
}
