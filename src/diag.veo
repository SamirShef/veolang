import std.sys;
import std.math;
import std.io;
import std;
import basic;
import color;

const FT_NOTE    = 0;
const FT_HELP    = 1;

pub const SEV_ERROR   = 0;
pub const SEV_WARNING = 1;
pub const SEV_NOTE    = 2;
pub const SEV_HELP    = 3;

pub const E_UNEXPECTED_TOKEN                       =  0;
pub const E_EXPECTED_EXPR                          =  1;
pub const E_UNCLOSED_STR_LIT                       =  2;
pub const E_UNCLOSED_CHAR_LIT                      =  3;
pub const E_INCORRECT_CHAR_LIT_LEN                 =  4;
pub const E_INT_SUFFIX_FOR_FLOAT                   =  5;
pub const E_INVALID_NUM_SUFFIX                     =  6;
pub const E_DIV_BY_ZERO                            =  7;
pub const E_REDEFINITION                           =  8;
pub const E_UNDEFINED                              =  9;
pub const W_UNUSEDVAR                              = 10;
pub const W_LOSSPRECISION                          = 11;

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
        for let i = 0uz, i < this.len, i += 1 {
            (this.data + i).msg.destroy();
        }
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
            msg: msg,
            labels: ListSpanLabel.new(),
            footers: ListFooter.new()
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

    pub func span(span: basic.Span, msg: *u8): *DiagBuilder {
        return this.span(span, std.String.from(msg), true);
    }

    pub func span(span: basic.Span, msg: std.String, is_primary: bool): *DiagBuilder {
        this.labels.add(SpanLabel { span: span, msg: msg, is_primary: is_primary });
        return this;
    }

    pub func span(span: basic.Span, msg: *u8, is_primary: bool): *DiagBuilder {
        return this.span(span, std.String.from(msg), is_primary);
    }

    pub func note(msg: std.String): *DiagBuilder {
        this.footers.add(Footer { kind: FT_NOTE, msg: msg });
        return this;
    }

    pub func note(msg: *u8): *DiagBuilder {
        return this.note(std.String.from(msg));
    }

    pub func help(msg: std.String): *DiagBuilder {
        this.footers.add(Footer { kind: FT_HELP, msg: msg });
        return this;
    }

    pub func help(msg: *u8): *DiagBuilder {
        return this.help(std.String.from(msg));
    }

    pub func sort_spans(): *DiagBuilder {
        sort_diag_spans(this);
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

    pub func report(code: i32, msg: std.String, sev: i32): *DiagBuilder {
        if sev == SEV_ERROR {
            this.has_errs = true;
        }
        let builder = DiagBuilder.new(sev, code, msg);
        this.diags.add(builder);
        return this.diags.data() + this.diags.len() - 1uz;
    }

    pub func report(code: i32, msg: *u8, sev: i32): *DiagBuilder {
        return this.report(code, std.String.from(msg), sev);
    }

    pub func render() {
        for let i = 0uz, i < this.diags.len(), i += 1 {
            this.render_diag(this.diags.data() + i);
        }
    }

    pub func has_errs(): bool {
        return this.has_errs;
    }

    func render_diag(diag: *DiagBuilder) {
        diag.sort_spans();
        this.print_diagnostic_header(diag);
        this.print_diagnostic_body(diag);
    }

    pub func destroy() {
        this.diags.destroy();
    }

    func sort_spans() {
        for let i = 0uz, i < this.diags.len(), i += 1 {
            this.diags
                .get(i)
                .unwrap()
                .sort_spans();
        }
    }

    func print_diagnostic_header(diag: *DiagBuilder) {
        let color_code = severity_to_color(diag.sev);
        let sev_str = severity_to_string(diag.sev);
        let code_str = format_code_4_digits(diag.code);
        let prefix = severity_to_prefix(diag.sev);

        sys.write(2, color.BOLD, sys.strlen(color.BOLD));
        sys.write(2, color_code, sys.strlen(color_code));
        sys.write(2, sev_str, sys.strlen(sev_str));
        sys.write(2, color.WHITE, sys.strlen(color.WHITE));
        sys.write(2, "[", 1uz);

        sys.write(2, color.BOLD, sys.strlen(color.BOLD));
        sys.write(2, color_code, sys.strlen(color_code));
        sys.write(2, &prefix, 1uz);
        sys.write(2, code_str.data(), code_str.len());
        sys.write(2, color.WHITE, sys.strlen(color.WHITE));
        sys.write(2, "]: ", 3uz);

        sys.write(2, diag.msg.data(), diag.msg.len());
        sys.write(2, "\n", 1uz);

        code_str.destroy();
    }

    pub func print_diagnostic_body(diag: *DiagBuilder) {
        let labels_len = diag.labels.len();
        if labels_len == 0uz {
            return;
        }

        let max_line = 1u32;
        for let i = 0uz, i < labels_len, i += 1 {
            let label = diag.labels.get(i).unwrap();
            let line_info = this.mgr.find_loc(label.span.start);
            if line_info.line > max_line {
                max_line = line_info.line;
            }
        }
        let max_line_width = digit_count(max_line.(i32));

        let last_file_id = (-1).(u32);
        let prev_end_line = 0u32;

        for let i = 0uz, i < labels_len, i += 1 {
            let label = diag.labels.get(i).unwrap();
            let file_id = label.span.start.file_id;
            let line_info = this.mgr.find_loc(label.span.start);
            let start_loc = this.mgr.find_loc(label.span.start);
            let end_loc = this.mgr.find_loc(label.span.end);

            if i == 0uz || last_file_id != file_id {
                if i != 0uz {
                    sys.write(2, "\n", 1uz);
                }
                let file_name = this.mgr.get_buffer(file_id).unwrap().name;
                print_spaces(max_line_width);
                sys.write(2, " --> ", 5uz);
                sys.write(2, file_name.data(), file_name.len());
                sys.write(2, ":", 1uz);
                sys.__veo_print_u64(2, line_info.line.(u64));
                sys.write(2, ":", 1uz);
                sys.__veo_print_u64(2, line_info.col.(u64));
                sys.write(2, "\n", 1uz);
                last_file_id = file_id;
            } else if start_loc.line > prev_end_line && start_loc.line - prev_end_line > 2u32 {
                print_spaces(max_line_width);
                sys.write(2, "  ...\n", 6uz);
            }

            if i == 0uz {
                print_spaces(max_line_width);
                sys.write(2, "  |\n", 4uz);
            }

            if start_loc.line == end_loc.line {
                let line_num_str = std.usize_to_string(line_info.line.(usize));
                print_spaces(max_line_width - line_num_str.len().(i32) + 1);
                sys.write(2, color.YELLOW, sys.strlen(color.YELLOW));
                sys.write(2, line_num_str.data(), line_num_str.len());
                sys.write(2, color.WHITE, sys.strlen(color.WHITE));
                sys.write(2, " | ", 3uz);

                let line_content = this.mgr.get_line_content(file_id, line_info.line);
                sys.write(2, line_content.data(), line_content.len());
                sys.write(2, "\n", 1uz);

                print_spaces(max_line_width);
                sys.write(2, "  | ", 4uz);
                print_spaces(line_info.col.(i32) - 1);

                let underline_char = label.is_primary ? '^'.(u8) : '-'.(u8);
                let span_len = label.span.end.offset - label.span.start.offset;
                if span_len < 1u32 {
                    span_len = 1u32;
                }

                sys.write(2, color.RED, sys.strlen(color.RED));
                for let k = 0u32, k < span_len, k += 1 {
                    sys.write(2, &underline_char, 1uz);
                }
                sys.write(2, color.WHITE, sys.strlen(color.WHITE));

                if label.msg.len() > 0uz {
                    sys.write(2, " ", 1uz);
                    sys.write(2, label.msg.data(), label.msg.len());
                }
                sys.write(2, "\n", 1uz);

                if i == labels_len - 1uz {
                    print_spaces(max_line_width);
                    sys.write(2, "  |\n", 4uz);
                }

                line_num_str.destroy();
            } else {
                sys.write(2, color.RESET, sys.strlen(color.RESET));
                let line_diff = end_loc.line - start_loc.line;
                if line_diff > 2u32 {
                    let line_num_str1 = std.usize_to_string(start_loc.line.(usize));
                    print_spaces(max_line_width - line_num_str1.len().(i32) + 1);
                    sys.write(2, color.BOLD, sys.strlen(color.BOLD));
                    sys.write(2, color.YELLOW, sys.strlen(color.YELLOW));
                    sys.write(2, line_num_str1.data(), line_num_str1.len());
                    sys.write(2, color.WHITE, sys.strlen(color.WHITE));
                    sys.write(2, " | ", 3uz);

                    sys.write(2, color.RED, sys.strlen(color.RED));
                    sys.write(2, "/ ", 2uz);
                    sys.write(2, color.WHITE, sys.strlen(color.WHITE));

                    let content1 = this.mgr.get_line_content(file_id, start_loc.line);
                    sys.write(2, content1.data(), content1.len());
                    sys.write(2, "\n", 1uz);
                    line_num_str1.destroy();

                    print_spaces(max_line_width);
                    sys.write(2, "  | ", 4uz);
                    sys.write(2, color.RED, sys.strlen(color.RED));
                    sys.write(2, "| ...\n", 6uz);
                    sys.write(2, color.WHITE, sys.strlen(color.WHITE));

                    let line_num_str2 = std.usize_to_string(end_loc.line.(usize));
                    print_spaces(max_line_width - line_num_str2.len().(i32) + 1);
                    sys.write(2, color.BOLD, sys.strlen(color.BOLD));
                    sys.write(2, color.YELLOW, sys.strlen(color.YELLOW));
                    sys.write(2, line_num_str2.data(), line_num_str2.len());
                    sys.write(2, color.WHITE, sys.strlen(color.WHITE));
                    sys.write(2, " | ", 3uz);

                    sys.write(2, color.RED, sys.strlen(color.RED));
                    sys.write(2, "| ", 2uz);
                    sys.write(2, color.WHITE, sys.strlen(color.WHITE));

                    let content2 = this.mgr.get_line_content(file_id, end_loc.line);
                    sys.write(2, content2.data(), content2.len());
                    sys.write(2, "\n", 1uz);
                    line_num_str2.destroy();
                } else {
                    for let line_num = start_loc.line, line_num <= end_loc.line, line_num += 1 {
                        let line_num_str = std.usize_to_string(line_num.(usize));
                        print_spaces(max_line_width - line_num_str.len().(i32) + 1);
                        sys.write(2, color.BOLD, sys.strlen(color.BOLD));
                        sys.write(2, color.YELLOW, sys.strlen(color.YELLOW));
                        sys.write(2, line_num_str.data(), line_num_str.len());
                        sys.write(2, color.WHITE, sys.strlen(color.WHITE));
                        sys.write(2, " | ", 3uz);

                        sys.write(2, color.RED, sys.strlen(color.RED));
                        if line_num == start_loc.line {
                            sys.write(2, "/ ", 2uz);
                        } else {
                            sys.write(2, "| ", 2uz);
                        }
                        sys.write(2, color.WHITE, sys.strlen(color.WHITE));

                        let line_content = this.mgr.get_line_content(file_id, line_num);
                        sys.write(2, line_content.data(), line_content.len());
                        sys.write(2, "\n", 1uz);

                        line_num_str.destroy();
                    }
                }

                print_spaces(max_line_width);
                sys.write(2, "  | ", 4uz);
                sys.write(2, color.RED, sys.strlen(color.RED));
                sys.write(2, "|_", 2uz);

                let underline_char = label.is_primary ? '^'.(u8) : '-'.(u8);
                let end_col = end_loc.col;
                if end_col < 1u32 {
                    end_col = 1u32;
                }

                for let k = 0u32, k < end_col, k += 1 {
                    sys.write(2, &underline_char, 1uz);
                }
                sys.write(2, color.WHITE, sys.strlen(color.WHITE));

                if label.msg.len() > 0uz {
                    sys.write(2, " ", 1uz);
                    sys.write(2, label.msg.data(), label.msg.len());
                }
                sys.write(2, "\n", 1uz);
            }

            prev_end_line = end_loc.line;
        }

        sys.write(2, color.RESET, sys.strlen(color.RESET));
        let footers_len = diag.footers.len();
        for let i = 0uz, i < footers_len, i += 1 {
            let footer = diag.footers.get(i).unwrap();
            sys.write(2, color.CYAN, sys.strlen(color.CYAN));
            print_spaces(max_line_width);
            if footer.kind == FT_NOTE {
                sys.write(2, "  = note: ", 10uz);
            } else {
                sys.write(2, "  = help: ", 10uz);
            }
            sys.write(2, color.RESET, sys.strlen(color.RESET));
            for let i = 0uz, i < footer.msg.len(), i += 1 {
                sys.write(2, (footer.msg.data() + i), 1uz);
                if footer.msg.get(i).unwrap() == '\n'.(u8) {
                    print_spaces(max_line_width + 10);
                }
            }
            sys.write(2, "\n", 1uz);
        }
    }
}

func is_span_label_less(a: SpanLabel, b: SpanLabel): bool {
    if a.span.start.file_id != b.span.start.file_id {
        return a.span.start.file_id < b.span.start.file_id;
    }
    if a.span.start.offset != b.span.start.offset {
        return a.span.start.offset < b.span.start.offset;
    }
    return a.span.end.offset < b.span.end.offset;
}

pub func sort_diag_spans(builder: *DiagBuilder) {
    let data = builder.labels.data();
    let len = builder.labels.len();
    if len < 2uz {
        return;
    }

    for let i = 0uz, i < len - 1uz, i += 1 {
        for let j = 0uz, j < len - 1uz - i, j += 1 {
            let curr = *(data + j);
            let next = *(data + j + 1uz);
            if is_span_label_less(next, curr) {
                *(data + j) = next;
                *(data + j + 1uz) = curr;
            }
        }
    }
}

pub func digit_count(line: i32): i32 {
    if line == 0 {
        return 1;
    }
    let count = 0;
    let temp = line;
    if temp < 0 {
        temp = -temp;
    }
    for temp > 0 {
        count += 1;
        temp /= 10;
    }
    return count;
}

func severity_to_string(sev: i32): *u8 {
    if sev == SEV_ERROR {
        return "error";
    } else if sev == SEV_WARNING {
        return "warning";
    } else if sev == SEV_NOTE {
        return "note";
    } else if sev == SEV_HELP {
        return "help";
    }
    return "error";
}

func severity_to_prefix(sev: i32): u8 {
    if sev == SEV_ERROR {
        return 'E'.(u8);
    } else if sev == SEV_WARNING {
        return 'W'.(u8);
    } else if sev == SEV_NOTE {
        return 'N'.(u8);
    } else if sev == SEV_HELP {
        return 'H'.(u8);
    }
    return 'E'.(u8);
}

func severity_to_color(sev: i32): *u8 {
    if sev == SEV_ERROR {
        return color.RED;
    } else if sev == SEV_WARNING {
        return color.YELLOW;
    } else if sev == SEV_NOTE {
        return color.WHITE;
    } else if sev == SEV_HELP {
        return color.CYAN;
    }
    return color.RESET;
}

func print_spaces(count: i32) {
    for let i = 0, i < count, i += 1 {
        sys.write(2, " ", 1uz);
    }
}

func format_code_4_digits(code: i32): std.String {
    let s = std.i32_to_string(code);
    let result = std.String.from("");
    let missing_zeros = 4uz - s.len();
    if s.len() < 4uz {
        for let i = 0uz, i < missing_zeros, i += 1 {
            result.append('0'.(u8));
        }
    }
    result.append(s);
    s.destroy();
    return result;
}
