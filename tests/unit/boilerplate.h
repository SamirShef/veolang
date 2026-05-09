#pragma once
#include <ast/stmts/var_def.h>
#include <filesystem>
#include <iostream>
#include <lexer/lexer.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_os_ostream.h>
#include <parser/parser.h>

#define init(root, file)                                                                 \
    llvm::SourceMgr              mgr;                                                    \
    diagnostic::DiagnosticEngine diag (mgr);                                             \
    fs::path                     filePath = root / fs::path (file);                      \
    auto bufferOrErr = llvm::MemoryBuffer::getFile (filePath.string ());                 \
    if (std::error_code ec = bufferOrErr.getError ()) {                                  \
        std::cerr << filePath.string () << ": " << ec.message () << '\n';                \
        exit (1);                                                                        \
    }                                                                                    \
    unsigned bufferId = mgr.AddNewSourceBuffer (std::move (*bufferOrErr), llvm::SMLoc ());

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

#define assert_eq(lhs, rhs)                                                              \
    assert_fatal ((lhs) == (rhs), "equals " + std::to_string (lhs))

#define assert_tokens_count(tokens, count)                                               \
    assert_fatal (                                                                       \
        tokens.size () == (count),                                                       \
        "wrong arguments count (" + std::to_string (tokens.size ()) + ")");

#define call_test(name, rootPath) name (fs::path (rootPath));

#define assert_node_kind(node, kind)                                                     \
    assert_fatal (node != nullptr, "node is null");                                      \
    assert_fatal (                                                                       \
        node->Kind () == NodeKind::kind,                                                 \
        "wrong node kind: " + std::to_string ((int) node->Kind ()));

#define assert_var_def_with_type(node, name, isConst, hasInit)                           \
    do {                                                                                 \
        assert_node_kind (node, VarDef);                                                 \
        auto *vds = llvm::cast<veo::ast::VarDef> (node);                                 \
        assert_fatal (vds->Name ().Val == name, "wrong name: " + vds->Name ().Val);      \
        assert_eq (vds->Type () != nullptr, true);                                       \
        assert_eq (vds->IsConst (), isConst);                                            \
        assert_eq (hasInit ? vds->Init () != nullptr : vds->Init () == nullptr, true);   \
    } while (false);

#define assert_var_def_without_type(node, name, isConst, hasInit)                        \
    do {                                                                                 \
        assert_node_kind (node, VarDef);                                                 \
        auto *vds = llvm::cast<veo::ast::VarDef> (node);                                 \
        assert_fatal (vds->Name ().Val == name, "wrong name: " + vds->Name ().Val);      \
        assert_eq (vds->IsConst (), isConst);                                            \
        assert_eq (hasInit ? vds->Init () != nullptr : vds->Init () == nullptr, true);   \
    } while (false);
