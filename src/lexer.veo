import std.math;
import llvm.smloc;
import basic;
import std.mem;
import std;
import std.sys;
import llvm.source_mgr;

pub const TOK_ID         =                  0;
pub const TOK_BOOL       = TOK_ID         + 1;
pub const TOK_CHAR       = TOK_BOOL       + 1;
pub const TOK_I8         = TOK_CHAR       + 1;
pub const TOK_I16        = TOK_I8         + 1;
pub const TOK_I32        = TOK_I16        + 1;
pub const TOK_I64        = TOK_I32        + 1;
pub const TOK_ISIZE      = TOK_I64        + 1;
pub const TOK_U8         = TOK_ISIZE      + 1;
pub const TOK_U16        = TOK_U8         + 1;
pub const TOK_U32        = TOK_U16        + 1;
pub const TOK_U64        = TOK_U32        + 1;
pub const TOK_USIZE      = TOK_U64        + 1;
pub const TOK_F32        = TOK_USIZE      + 1;
pub const TOK_F64        = TOK_F32        + 1;
pub const TOK_LET        = TOK_F64        + 1;
pub const TOK_CONST      = TOK_LET        + 1;
pub const TOK_FUNC       = TOK_CONST      + 1;
pub const TOK_RET        = TOK_FUNC       + 1;
pub const TOK_IF         = TOK_RET        + 1;
pub const TOK_ELSE       = TOK_IF         + 1;
pub const TOK_FOR        = TOK_ELSE       + 1;
pub const TOK_BREAK      = TOK_FOR        + 1;
pub const TOK_CONT       = TOK_BREAK      + 1;
pub const TOK_STRUCT     = TOK_CONT       + 1;
pub const TOK_PUB        = TOK_STRUCT     + 1;
pub const TOK_IMPL       = TOK_PUB        + 1;
pub const TOK_TRAIT      = TOK_IMPL       + 1;
pub const TOK_NIL        = TOK_TRAIT      + 1;
pub const TOK_MOD        = TOK_NIL        + 1;
pub const TOK_IMPORT     = TOK_MOD        + 1;
pub const TOK_STATIC     = TOK_IMPORT     + 1;
pub const TOK_EXTERN     = TOK_STATIC     + 1;
pub const TOK_SIZEOF     = TOK_EXTERN     + 1;
pub const TOK_BOOL_LIT   = TOK_SIZEOF     + 1;
pub const TOK_CHAR_LIT   = TOK_BOOL_LIT   + 1;
pub const TOK_I8_LIT     = TOK_CHAR_LIT   + 1;
pub const TOK_I16_LIT    = TOK_I8_LIT     + 1;
pub const TOK_I32_LIT    = TOK_I16_LIT    + 1;
pub const TOK_I64_LIT    = TOK_I32_LIT    + 1;
pub const TOK_ISIZE_LIT  = TOK_I64_LIT    + 1;
pub const TOK_U8_LIT     = TOK_ISIZE_LIT  + 1;
pub const TOK_U16_LIT    = TOK_U8_LIT     + 1;
pub const TOK_U32_LIT    = TOK_U16_LIT    + 1;
pub const TOK_U64_LIT    = TOK_U32_LIT    + 1;
pub const TOK_USIZE_LIT  = TOK_U64_LIT    + 1;
pub const TOK_F32_LIT    = TOK_USIZE_LIT  + 1;
pub const TOK_F64_LIT    = TOK_F32_LIT    + 1;
pub const TOK_INT_LIT    = TOK_F64_LIT    + 1;
pub const TOK_STR_LIT    = TOK_INT_LIT    + 1;
pub const TOK_SEMI       = TOK_STR_LIT    + 1;
pub const TOK_COMMA      = TOK_SEMI       + 1;
pub const TOK_DOT        = TOK_COMMA      + 1;
pub const TOK_LPAREN     = TOK_DOT        + 1;
pub const TOK_RPAREN     = TOK_LPAREN     + 1;
pub const TOK_LBRACE     = TOK_RPAREN     + 1;
pub const TOK_RBRACE     = TOK_LBRACE     + 1;
pub const TOK_LBRACKET   = TOK_RBRACE     + 1;
pub const TOK_RBRACKET   = TOK_LBRACKET   + 1;
pub const TOK_TILDE      = TOK_RBRACKET   + 1;
pub const TOK_QUESTION   = TOK_TILDE      + 1;
pub const TOK_COLON      = TOK_QUESTION   + 1;
pub const TOK_EQ         = TOK_COLON      + 1;
pub const TOK_AMP        = TOK_EQ         + 1;
pub const TOK_PIPE       = TOK_AMP        + 1;
pub const TOK_AMPAMP     = TOK_PIPE       + 1;
pub const TOK_PIPEPIPE   = TOK_AMPAMP     + 1;
pub const TOK_PLUS       = TOK_PIPEPIPE   + 1;
pub const TOK_MINUS      = TOK_PLUS       + 1;
pub const TOK_STAR       = TOK_MINUS      + 1;
pub const TOK_SLASH      = TOK_STAR       + 1;
pub const TOK_PERCENT    = TOK_SLASH      + 1;
pub const TOK_PLUS_EQ    = TOK_PERCENT    + 1;
pub const TOK_MINUS_EQ   = TOK_PLUS_EQ    + 1;
pub const TOK_STAR_EQ    = TOK_MINUS_EQ   + 1;
pub const TOK_SLASH_EQ   = TOK_STAR_EQ    + 1;
pub const TOK_PERCENT_EQ = TOK_SLASH_EQ   + 1;
pub const TOK_AMP_EQ     = TOK_PERCENT_EQ + 1;
pub const TOK_PIPE_EQ    = TOK_AMP_EQ     + 1;
pub const TOK_CARET_EQ   = TOK_PIPE_EQ    + 1;
pub const TOK_BANG       = TOK_CARET_EQ   + 1;
pub const TOK_BANG_EQ    = TOK_BANG       + 1;
pub const TOK_EQ_EQ      = TOK_BANG_EQ    + 1;
pub const TOK_GT         = TOK_EQ_EQ      + 1;
pub const TOK_LT         = TOK_GT         + 1;
pub const TOK_GT_EQ      = TOK_LT         + 1;
pub const TOK_LT_EQ      = TOK_GT_EQ      + 1;
pub const TOK_CARET      = TOK_LT_EQ      + 1;
pub const TOK_UNKNOWN    = TOK_CARET      + 1;
pub const TOK_EOF        = TOK_UNKNOWN    + 1;

pub struct Token {
    pub kind: i32;
    pub val: std.StringView;
    pub range: basic.Span;
}

impl Token {
    pub static func new(kind: i32, val: std.StringView, range: basic.Span): Token {
        return Token { kind: kind, val: val, range: range };
    }

    pub static func new(kind: i32, range: basic.Span): Token {
        return Token { kind: kind, val: std.StringView.from(""), range: range };
    }
}

impl std.ToString for Token {
    pub func to_string(alloc: mem.Allocator): std.String {
        let s: std.String;
        let kind_str = std.i32_to_string(alloc, this.kind);
        s.append(alloc, kind_str);
        s.append(alloc, '('.(u8));
        s.append(alloc, this.val);
        s.append(alloc, ')'.(u8));
        kind_str.destroy(alloc);
        return s;
    }
}

pub struct OptionToken {
    has_val: bool;
    val: Token;
}

impl OptionToken {
    pub static func some(val: Token): OptionToken {
        return OptionToken { has_val: true, val: val };
    }

    pub static func none(): OptionToken {
        return OptionToken { has_val: false };
    }

    pub func has_val(): bool {
        return this.has_val;
    }

    pub func unwrap(): Token {
        if !this.has_val {
            std.panic("Called unwrap() on a 'None' value (Option is empty)");
        }
        return this.val;
    }

    pub func unwrap_or(err_val: Token): Token {
        if !this.has_val {
            return err_val;
        }
        return this.val;
    }
}

pub struct Lexer {
    buf_start: *u8;
    buf_end: *u8;
    cur: *u8;
}

impl Lexer {
    pub static func new(mgr: source_mgr.SourceMgr, id: usize): Lexer {
        let raw_buf = mgr.get_buffer(id);
        if !raw_buf.has_val() {
            sys.write(2, "veo panic: ", 11uz);
            sys.write(2, "buffer with id ", 15uz);
            sys.__veo_print_u64(2, id.(u64));
            sys.write(2, "does not loaded\n", 16uz);
            sys.write(2, "\naborting execution...\n", 23uz);
            sys.exit(1);
        }
        let buf = raw_buf.unwrap();
        return Lexer {
            buf_start: buf.data(),
            buf_end: buf.data() + buf.len(),
            cur: buf.data()
        };
    }

    pub func next_token(): OptionToken {
        if this.at_end() {
            return OptionToken.some(
                Token.new(
                    TOK_EOF,
                    basic.Span.new(
                        smloc.SMLoc.new(this.cur)
                    )
                )
            );
        }
        let c = this.peek().(char);
        if std.is_ascii_letter(c) || c == '_' || c == '@' {
            return this.tokenize_id();
        }
        if std.is_ascii_whitespace(c) {
            return skip_whitespace(this);
        }
        if c == '/' && (this.peek(1uz) == '/'.(u8) || this.peek(1uz) == '*'.(u8)) {
            this.skip_comments();
            return next_token(this);
        }
        if std.is_ascii_digit(c)
            || c == '.' && std.is_ascii_digit(this.peek(1uz).(char))
            || c == '-' && std.is_ascii_digit(this.peek(1uz).(char)) {
            return this.tokenize_num_lit();
        }
        if c == '"' {
            return this.tokenize_str_lit();
        }
        if c == '\'' {
            return this.tokenize_char_lit();
        }
        return this.tokenize_op();
    }

    func tokenize_id(): OptionToken {
        let start = this.cur;
        for std.is_ascii_letter_or_digit(this.peek().(char))
            || this.peek() == '_'.(u8) || this.peek() == '@'.(u8) {
            this.advance();
        }
        let val = std.StringView.from(start, this.cur.(usize) - start.(usize));
        return OptionToken.some(
            Token.new(
                TOK_ID,
                val,
                basic.Span.new(
                    smloc.SMLoc.new(start),
                    smloc.SMLoc.new(this.cur)
                )
            )
        );
    }

    func tokenize_num_lit(): OptionToken {
        let start = this.cur;
        let has_dot = false;
        for !this.at_end()
            && (std.is_ascii_digit(this.peek().(char))
                || this.peek() == '.'.(u8)
                || this.peek() == '-'.(u8)
                || this.peek() == '_'.(u8)) {
            if this.peek() == '.'.(u8) {
                if std.is_ascii_digit(this.peek(1uz).(char)) {
                    if has_dot {
                        break;
                    }
                    has_dot = true;
                } else {
                    break;
                }
            }
            this.advance();
        }
        let c = this.peek().(char);
        if c == 'i' {
            this.advance();
            let c = this.peek().(char);
            if c == '8' {
                this.advance();
                return OptionToken.some(
                    Token.new(
                        TOK_I8_LIT,
                        std.StringView.from(start, this.cur.(usize) - start.(usize) - 2uz),
                        basic.Span.new(
                            smloc.SMLoc.new(start),
                            smloc.SMLoc.new(this.cur)
                        )
                    )
                );
            } else if c == '1' && this.peek(1uz) == '6'.(u8) {
                this.advance();
                this.advance();
                return OptionToken.some(
                    Token.new(
                        TOK_I16_LIT,
                        std.StringView.from(start, this.cur.(usize) - start.(usize) - 3uz),
                        basic.Span.new(
                            smloc.SMLoc.new(start),
                            smloc.SMLoc.new(this.cur)
                        )
                    )
                );
            } else if c == '3' && this.peek(1uz) == '2'.(u8) {
                this.advance();
                this.advance();
                return OptionToken.some(
                    Token.new(
                        TOK_I32_LIT,
                        std.StringView.from(start, this.cur.(usize) - start.(usize) - 3uz),
                        basic.Span.new(
                            smloc.SMLoc.new(start),
                            smloc.SMLoc.new(this.cur)
                        )
                    )
                );
            } else if c == '6' && this.peek(1uz) == '4'.(u8) {
                this.advance();
                this.advance();
                return OptionToken.some(
                    Token.new(
                        TOK_I64_LIT,
                        std.StringView.from(start, this.cur.(usize) - start.(usize) - 3uz),
                        basic.Span.new(
                            smloc.SMLoc.new(start),
                            smloc.SMLoc.new(this.cur)
                        )
                    )
                );
            } else if c == 'z' {
                this.advance();
                return OptionToken.some(
                    Token.new(
                        TOK_ISIZE_LIT,
                        std.StringView.from(start, this.cur.(usize) - start.(usize) - 2uz),
                        basic.Span.new(
                            smloc.SMLoc.new(start),
                            smloc.SMLoc.new(this.cur)
                        )
                    )
                );
            }
            this.cur = this.cur - 1uz;
        } else if c == 'u' {
            this.advance();
            let c = this.peek().(char);
            if c == '8' {
                this.advance();
                return OptionToken.some(
                    Token.new(
                        TOK_U8_LIT,
                        std.StringView.from(start, this.cur.(usize) - start.(usize) - 2uz),
                        basic.Span.new(
                            smloc.SMLoc.new(start),
                            smloc.SMLoc.new(this.cur)
                        )
                    )
                );
            } else if c == '1' && this.peek(1uz) == '6'.(u8) {
                this.advance();
                this.advance();
                return OptionToken.some(
                    Token.new(
                        TOK_U16_LIT,
                        std.StringView.from(start, this.cur.(usize) - start.(usize) - 3uz),
                        basic.Span.new(
                            smloc.SMLoc.new(start),
                            smloc.SMLoc.new(this.cur)
                        )
                    )
                );
            } else if c == '3' && this.peek(1uz) == '2'.(u8) {
                this.advance();
                this.advance();
                return OptionToken.some(
                    Token.new(
                        TOK_U32_LIT,
                        std.StringView.from(start, this.cur.(usize) - start.(usize) - 3uz),
                        basic.Span.new(
                            smloc.SMLoc.new(start),
                            smloc.SMLoc.new(this.cur)
                        )
                    )
                );
            } else if c == '6' && this.peek(1uz) == '4'.(u8) {
                this.advance();
                this.advance();
                return OptionToken.some(
                    Token.new(
                        TOK_U64_LIT,
                        std.StringView.from(start, this.cur.(usize) - start.(usize) - 3uz),
                        basic.Span.new(
                            smloc.SMLoc.new(start),
                            smloc.SMLoc.new(this.cur)
                        )
                    )
                );
            } else if c == 'z' {
                this.advance();
                return OptionToken.some(
                    Token.new(
                        TOK_USIZE_LIT,
                        std.StringView.from(start, this.cur.(usize) - start.(usize) - 2uz),
                        basic.Span.new(
                            smloc.SMLoc.new(start),
                            smloc.SMLoc.new(this.cur)
                        )
                    )
                );
            }
            this.cur = this.cur - 1uz;
        } else if c == 'f' {
            this.advance();
            let c = this.peek().(char);
            if c == '3' && this.peek(1uz) == '2'.(u8) {
                this.advance();
                this.advance();
                return OptionToken.some(
                    Token.new(
                        TOK_F32_LIT,
                        std.StringView.from(start, this.cur.(usize) - start.(usize) - 3uz),
                        basic.Span.new(
                            smloc.SMLoc.new(start),
                            smloc.SMLoc.new(this.cur)
                        )
                    )
                );
            } else if c == '6' && this.peek(1uz) == '4'.(u8) {
                this.advance();
                this.advance();
                return OptionToken.some(
                    Token.new(
                        TOK_F64_LIT,
                        std.StringView.from(start, this.cur.(usize) - start.(usize) - 3uz),
                        basic.Span.new(
                            smloc.SMLoc.new(start),
                            smloc.SMLoc.new(this.cur)
                        )
                    )
                );
            }
            this.cur = this.cur - 1uz;
        }

        if has_dot {
            return OptionToken.some(
                Token.new(
                    TOK_F64_LIT,
                    std.StringView.from(start, this.cur.(usize) - start.(usize)),
                    basic.Span.new(
                        smloc.SMLoc.new(start),
                        smloc.SMLoc.new(this.cur)
                    )
                )
            );
        } else {
            return OptionToken.some(
                Token.new(
                    TOK_INT_LIT,
                    std.StringView.from(start, this.cur.(usize) - start.(usize)),
                    basic.Span.new(
                        smloc.SMLoc.new(start),
                        smloc.SMLoc.new(this.cur)
                    )
                )
            );
        }
    }

    func tokenize_str_lit(): OptionToken {
        let start = this.cur;
        this.advance(); // skip "
        for !this.at_end() && this.peek() != '"'.(u8) {
            this.advance();
        }
        if this.at_end() {
            // TODO: report error (unclosed string literal)
        }
        this.advance(); // skip "
        return OptionToken.some(
            Token.new(
                TOK_STR_LIT,
                std.StringView.from(start, this.cur.(usize) - start.(usize)),
                basic.Span.new(
                    smloc.SMLoc.new(start),
                    smloc.SMLoc.new(this.cur)
                )
            )
        );
    }

    func tokenize_char_lit(): OptionToken {
        let start = this.cur;
        this.advance(); // skip '
        for !this.at_end() && this.peek() != '\''.(u8) {
            this.advance();
        }
        if this.at_end() {
            // TODO: report error (unclosed character literal)
        }
        this.advance(); // skip '
        return OptionToken.some(
            Token.new(
                TOK_CHAR_LIT,
                std.StringView.from(start, this.cur.(usize) - start.(usize)),
                basic.Span.new(
                    smloc.SMLoc.new(start),
                    smloc.SMLoc.new(this.cur)
                )
            )
        );
    }

    func tokenize_op(): OptionToken {
        let start = this.cur;
        let c = this.peek().(char);
        let kind = -1;

        if c == ';' {
            kind = TOK_SEMI;
            this.advance();
        } else if c == ',' {
            kind = TOK_COMMA;
            this.advance();
        } else if c == '.' {
            kind = TOK_DOT;
            this.advance();
        } else if c == '(' {
            kind = TOK_LPAREN;
            this.advance();
        } else if c == ')' {
            kind = TOK_RPAREN;
            this.advance();
        } else if c == '{' {
            kind = TOK_LBRACE;
            this.advance();
        } else if c == '}' {
            kind = TOK_RBRACE;
            this.advance();
        } else if c == '[' {
            kind = TOK_LBRACKET;
            this.advance();
        } else if c == ']' {
            kind = TOK_RBRACKET;
            this.advance();
        } else if c == '~' {
            kind = TOK_TILDE;
            this.advance();
        } else if c == '?' {
            kind = TOK_QUESTION;
            this.advance();
        } else if c == ':' {
            kind = TOK_COLON;
            this.advance();
        } else if c == '=' {
            this.advance();
            if this.peek() == '='.(u8) {
                kind = TOK_EQ_EQ;
                this.advance();
            } else {
                kind = TOK_EQ;
            }
        } else if c == '&' {
            this.advance();
            if this.peek() == '&'.(u8) {
                kind = TOK_AMPAMP;
                this.advance();
            } else if this.peek() == '='.(u8) {
                kind = TOK_AMP_EQ;
                this.advance();
            } else {
                kind = TOK_AMP;
            }
        } else if c == '|' {
            this.advance();
            if this.peek() == '|'.(u8) {
                kind = TOK_PIPEPIPE;
                this.advance();
            } else if this.peek() == '='.(u8) {
                kind = TOK_PIPE_EQ;
                this.advance();
            } else {
                kind = TOK_PIPE;
            }
        } else if c == '+' {
            this.advance();
            if this.peek() == '='.(u8) {
                kind = TOK_PLUS_EQ;
                this.advance();
            } else {
                kind = TOK_PLUS;
            }
        } else if c == '-' {
            this.advance();
            if this.peek() == '='.(u8) {
                kind = TOK_MINUS_EQ;
                this.advance();
            } else {
                kind = TOK_MINUS;
            }
        } else if c == '*' {
            this.advance();
            if this.peek() == '='.(u8) {
                kind = TOK_STAR_EQ;
                this.advance();
            } else {
                kind = TOK_STAR;
            }
        } else if c == '/' {
            if this.peek() == '='.(u8) {
                kind = TOK_SLASH_EQ;
                this.advance();
            } else {
                kind = TOK_SLASH;
            }
            this.advance();
        } else if c == '%' {
            this.advance();
            if this.peek() == '='.(u8) {
                kind = TOK_PERCENT_EQ;
                this.advance();
            } else {
                kind = TOK_PERCENT;
            }
        } else if c == '^' {
            this.advance();
            if this.peek() == '='.(u8) {
                kind = TOK_CARET_EQ;
                this.advance();
            } else {
                kind = TOK_CARET;
            }
        } else if c == '!' {
            this.advance();
            if this.peek() == '='.(u8) {
                kind = TOK_BANG_EQ;
                this.advance();
            } else {
                kind = TOK_BANG;
            }
        } else if c == '>' {
            this.advance();
            if this.peek() == '='.(u8) {
                kind = TOK_GT_EQ;
                this.advance();
            } else {
                kind = TOK_GT;
            }
        } else if c == '<' {
            this.advance();
            if this.peek() == '='.(u8) {
                kind = TOK_LT_EQ;
                this.advance();
            } else {
                kind = TOK_LT;
            }
        }

        if kind == -1 {
            kind = TOK_UNKNOWN;
        }

        let val = std.StringView.from(start, this.cur.(usize) - start.(usize));
        return OptionToken.some(
            Token.new(
                kind,
                val,
                basic.Span.new(
                    smloc.SMLoc.new(start),
                    smloc.SMLoc.new(this.cur)
                )
            )
        );
    }

    func skip_comments() {
        this.advance();
        let is_multiline = this.peek() == '*'.(u8);
        this.advance();
        if is_multiline {
            for !this.at_end() && (this.peek() != '*'.(u8) || this.peek(1uz) != '/'.(u8)) {
                this.advance();
            }
            this.advance();
            this.advance();
        } else {
            for !this.at_end() && this.peek() != '\n'.(u8) {
                this.advance();
            }
            this.advance();
        }
    }

    func at_end(): bool {
        return this.cur >= this.buf_end;
    }

    func peek(): u8 {
        return this.peek(0uz);
    }

    func peek(rpos: usize): u8 {
        if this.cur + rpos >= this.buf_end {
            return '\0'.(u8);
        }
        return *(this.cur + rpos);
    }

    pub func advance() {
        this.cur = this.cur + 1uz;
    }
}

func skip_whitespace(this: *Lexer): OptionToken {
    this.advance();
    return next_token(this);
}

func next_token(this: *Lexer): OptionToken {
    return this.next_token();
}
