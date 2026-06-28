import llvm.smloc;

pub struct Span {
    pub start: smloc.SMLoc;
    pub end: smloc.SMLoc;
}

impl Span {
    pub static func new(start: smloc.SMLoc, end: smloc.SMLoc): Span {
        return Span { start: start, end: end };
    }

    pub static func new(start: smloc.SMLoc): Span {
        return Span { start: start, end: start };
    }
}
