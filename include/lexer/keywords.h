#pragma once
#include <lexer/token_kind.h>
#include <string>
#include <unordered_map>

namespace veo {

static const std::unordered_map<std::string, TokenKind> keywords{
    { "false",    TokenKind::BoolLit  },
    { "true",     TokenKind::BoolLit  },
    { "let",      TokenKind::Let      },
    { "const",    TokenKind::Const    },
    { "bool",     TokenKind::Bool     },
    { "char",     TokenKind::Char     },
    { "i8",       TokenKind::I8       },
    { "u8",       TokenKind::U8       },
    { "i16",      TokenKind::I16      },
    { "u16",      TokenKind::U16      },
    { "i32",      TokenKind::I32      },
    { "u32",      TokenKind::U32      },
    { "i64",      TokenKind::I64      },
    { "u64",      TokenKind::U64      },
    { "i128",     TokenKind::I128     },
    { "u128",     TokenKind::U128     },
    { "isize",    TokenKind::ISize    },
    { "usize",    TokenKind::USize    },
    { "f32",      TokenKind::F32      },
    { "f64",      TokenKind::F64      },
    { "func",     TokenKind::Func     },
    { "return",   TokenKind::Ret      },
    { "if",       TokenKind::If       },
    { "else",     TokenKind::Else     },
    { "for",      TokenKind::For      },
    { "break",    TokenKind::Break    },
    { "continue", TokenKind::Continue },
    { "struct",   TokenKind::Struct   },
    { "pub",      TokenKind::Pub      },
    { "impl",     TokenKind::Impl     },
    { "trait",    TokenKind::Trait    },
    { "nil",      TokenKind::Nil      },
    { "new",      TokenKind::New      },
    { "del",      TokenKind::Del      },
    { "mod",      TokenKind::Mod      },
    { "import",   TokenKind::Import   },
    { "static",   TokenKind::Static   },
};

}
