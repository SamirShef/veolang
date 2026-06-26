import llvm.smloc;

pub struct Span {
    pub start: smloc.SMLoc;
    pub end: smloc.SMLoc;
}
