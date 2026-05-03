#pragma once
#include <filesystem>
#include <iostream>
#include <lexer/lexer.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_os_ostream.h>

#define init(root, file)                                                                 \
    llvm::SourceMgr mgr;                                                                 \
    fs::path        filePath    = root / fs::path (file);                                \
    auto            bufferOrErr = llvm::MemoryBuffer::getFile (filePath.string ());      \
    if (std::error_code ec = bufferOrErr.getError ()) {                                  \
        std::cerr << filePath.string () << ": " << ec.message () << '\n';                \
        exit (1);                                                                        \
    }                                                                                    \
    unsigned bufferId                                                                    \
        = mgr.AddNewSourceBuffer (std::move (*bufferOrErr), llvm::SMLoc ());             \
    Lexer lexer (mgr, bufferId);

#define assert_fatal(cond, message)                                                      \
    if (!(cond)) {                                                                       \
        llvm::errs ().changeColor (llvm::raw_fd_ostream::RED)                            \
            << "[FATAL]: (" << #cond << " in " << __FILE__ << ") " << message            \
            << llvm::raw_fd_ostream::RESET << '\n';                                      \
        exit (1);                                                                        \
    }

#define assert_tok(token, kind, val)                                                     \
    assert_fatal (                                                                       \
        token.Kind == TokenKind::kind && token.Val == val,                               \
        "unexpected token (got " + std::to_string (static_cast<int> (token.Kind)) + " "  \
            + token.Val + ")")

#define assert_tokens_count(tokens, count)                                               \
    assert_fatal (                                                                       \
        tokens.size () == (count),                                                       \
        "wrong arguments count (" + std::to_string (tokens.size ()) + ")");

#define call_test(name) name (fs::path (argv[1]));
