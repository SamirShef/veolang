import std.sys;
import std.math;
import std;
import basic;

const FT_NOTE    = 0;
const FT_HELP    = 1;

const SEV_ERROR   = 0;
const SEV_WARNING = 1;
const SEV_NOTE    = 2;
const SEV_HELP    = 3;

const E_UNEXPECTED_TOKEN                       =  0;
const E_EXPECTED_EXPR                          =  1;
const E_UNCLOSED_STR_LIT                       =  2;
const E_UNCLOSED_CHAR_LIT                      =  3;
const E_INCORRECT_CHAR_LIT_LEN                 =  4;
const E_INT_SUFFIX_FOR_FLOAT                   =  5;
const E_INVALID_NUM_SUFFIX                     =  6;
const E_DIV_BY_ZERO                            =  7;
const E_REDEFINITION                           =  8;
const E_UNDEFINED                              =  9;
const W_UNUSEDVAR                              = 10;
const W_LOSSPRECISION                          = 11;

pub struct SpanLabel {
    pub span: basic.Span;
    pub msg: std.String;
    pub is_primary: bool;
}

pub struct Footer {
    pub kind: i32;
    pub msg: std.String;
}

impl Footer {
    pub func destroy() {
        this.msg.destroy();
    }
}

pub struct OptionSpanLabel {
    val: SpanLabel;
    has_val: bool;
}

impl OptionSpanLabel {
    pub static func some(val: SpanLabel): OptionSpanLabel {
        return OptionSpanLabel { has_val: true, val: val };
    }

    pub static func none(): OptionSpanLabel {
        return OptionSpanLabel { has_val: false };
    }

    pub func has_val(): bool {
        return this.has_val;
    }

    pub func unwrap(): SpanLabel {
        if !this.has_val {
            std.panic("Called unwrap() on a 'None' value (Option is empty)");
        }
        return this.val;
    }

    pub func unwrap_or(err_val: SpanLabel): SpanLabel {
        if !this.has_val {
            return err_val;
        }
        return this.val;
    }
}

pub struct ListSpanLabel {
    data: *SpanLabel;
    len: usize;
    cap: usize;
}

impl ListSpanLabel {
    pub static func new(): ListSpanLabel {
        let cap = 4uz;
        let data = sys.malloc(cap * @size_of(SpanLabel)).(*SpanLabel);
        return ListSpanLabel {
            data: data,
            len: 0,
            cap: cap
        };
    }

    pub func add(val: SpanLabel) {
        if this.cap < this.len + 1uz {
            let old_cap = this.cap;
            this.cap = math.max(this.cap * 2uz, this.len + 1uz);
            this.data = sys.realloc(
                            this.data.(*u8),
                            this.cap * @size_of(SpanLabel)
                        ).(*SpanLabel);
        }
        *(this.data + this.len) = val;
        this.len += 1;
    }

    pub func get(index: usize): OptionSpanLabel {
        if index < this.len {
            return OptionSpanLabel.some(*(this.data + index));
        }
        return OptionSpanLabel.none();
    }

    pub func data(): *SpanLabel {
        return this.data;
    }

    pub func len(): usize {
        return this.len;
    }

    pub func capacity(): usize {
        return this.cap;
    }

    pub func destroy() {
        sys.free(this.data.(*u8));
    }
}

pub struct OptionFooter {
    val: Footer;
    has_val: bool;
}

impl OptionFooter {
    pub static func some(val: Footer): OptionFooter {
        return OptionFooter { has_val: true, val: val };
    }

    pub static func none(): OptionFooter {
        return OptionFooter { has_val: false };
    }

    pub func has_val(): bool {
        return this.has_val;
    }

    pub func unwrap(): Footer {
        if !this.has_val {
            std.panic("Called unwrap() on a 'None' value (Option is empty)");
        }
        return this.val;
    }

    pub func unwrap_or(err_val: Footer): Footer {
        if !this.has_val {
            return err_val;
        }
        return this.val;
    }
}

pub struct ListFooter {
    data: *Footer;
    len: usize;
    cap: usize;
}

impl ListFooter {
    pub static func new(): ListFooter {
        let cap = 4uz;
        let data = sys.malloc(cap * @size_of(Footer)).(*Footer);
        return ListFooter {
            data: data,
            len: 0,
            cap: cap
        };
    }

    pub func add(val: Footer) {
        if this.cap < this.len + 1uz {
            let old_cap = this.cap;
            this.cap = math.max(this.cap * 2uz, this.len + 1uz);
            this.data = sys.realloc(
                            this.data.(*u8),
                            this.cap * @size_of(Footer)
                        ).(*Footer);
        }
        *(this.data + this.len) = val;
        this.len += 1;
    }

    pub func get(index: usize): OptionFooter {
        if index < this.len {
            return OptionFooter.some(*(this.data + index));
        }
        return OptionFooter.none();
    }

    pub func data(): *Footer {
        return this.data;
    }

    pub func len(): usize {
        return this.len;
    }

    pub func capacity(): usize {
        return this.cap;
    }

    pub func destroy() {
        for let i = 0uz, i < this.len, i += 1 {
            (this.data + i).destroy();
        }
        sys.free(this.data.(*u8));
    }
}

pub struct DiagBuilder {
    pub sev: i32;
    pub code: i32;
    pub msg: std.String;

    pub labels: ListSpanLabel;
    pub footers: ListFooter;
}

impl DiagBuilder {
    pub static func new(sev: i32, code: i32, msg: std.String): DiagBuilder {
        return DiagBuilder {
            sev: sev,
            code: code,
            msg: msg
        };
    }

    pub func span(span: basic.Span): *DiagBuilder {
        return this.span(span, std.String.from(""));
    }

    pub func span(span: basic.Span, is_primary: bool): *DiagBuilder {
        return this.span(span, std.String.from(""), is_primary);
    }

    pub func span(span: basic.Span, msg: std.String): *DiagBuilder {
        return this.span(span, msg, true);
    }

    pub func span(span: basic.Span, msg: std.String, is_primary: bool): *DiagBuilder {
        this.labels.add(SpanLabel { span: span, msg: msg, is_primary: is_primary });
        return this;
    }

    pub func note(msg: std.String): *DiagBuilder {
        this.footers.add(Footer { kind: FT_NOTE, msg: msg });
        return this;
    }

    pub func help(msg: std.String): *DiagBuilder {
        this.footers.add(Footer { kind: FT_HELP, msg: msg });
        return this;
    }

    pub func destroy() {
        this.msg.destroy();
    }
}

pub struct OptionDiagBuilder {
    val: DiagBuilder;
    has_val: bool;
}

impl OptionDiagBuilder {
    pub static func some(val: DiagBuilder): OptionDiagBuilder {
        return OptionDiagBuilder { has_val: true, val: val };
    }

    pub static func none(): OptionDiagBuilder {
        return OptionDiagBuilder { has_val: false };
    }

    pub func has_val(): bool {
        return this.has_val;
    }

    pub func unwrap(): DiagBuilder {
        if !this.has_val {
            std.panic("Called unwrap() on a 'None' value (Option is empty)");
        }
        return this.val;
    }

    pub func unwrap_or(err_val: DiagBuilder): DiagBuilder {
        if !this.has_val {
            return err_val;
        }
        return this.val;
    }
}

pub struct ListDiagBuilder {
    data: *DiagBuilder;
    len: usize;
    cap: usize;
}

impl ListDiagBuilder {
    pub static func new(): ListDiagBuilder {
        let cap = 4uz;
        let data = sys.malloc(cap * @size_of(DiagBuilder)).(*DiagBuilder);
        return ListDiagBuilder {
            data: data,
            len: 0,
            cap: cap
        };
    }

    pub func add(val: DiagBuilder) {
        if this.cap < this.len + 1uz {
            let old_cap = this.cap;
            this.cap = math.max(this.cap * 2uz, this.len + 1uz);
            this.data = sys.realloc(
                            this.data.(*u8),
                            this.cap * @size_of(DiagBuilder)
                        ).(*DiagBuilder);
        }
        *(this.data + this.len) = val;
        this.len += 1;
    }

    pub func get(index: usize): OptionDiagBuilder {
        if index < this.len {
            return OptionDiagBuilder.some(*(this.data + index));
        }
        return OptionDiagBuilder.none();
    }

    pub func data(): *DiagBuilder {
        return this.data;
    }

    pub func len(): usize {
        return this.len;
    }

    pub func capacity(): usize {
        return this.cap;
    }

    pub func destroy() {
        for let i = 0uz, i < this.len, i += 1 {
            (this.data + i).destroy();
        }
        sys.free(this.data.(*u8));
    }
}

pub struct DiagEngine {
    mgr: *basic.SourceMgr;
    diags: ListDiagBuilder;
    has_errs: bool;
}

impl DiagEngine {
    pub static func new(mgr: *basic.SourceMgr): DiagEngine {
        return DiagEngine {
            mgr: mgr,
            diags: ListDiagBuilder.new(),
            has_errs: false
        };
    }

    pub func report(code: i32, msg: std.String, sev: i32): DiagBuilder {
        return DiagBuilder.new(sev, code, msg);
    }

    pub func report(code: i32, msg: *u8, sev: i32): DiagBuilder {
        return this.report(code, std.String.from(msg), sev);
    }

    pub func render() {
        // TODO: implement
    }

    pub func destroy() {
        this.diags.destroy();
    }
}
