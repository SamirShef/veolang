#include "../framework.h"
#include "get_file_path.h"

#include <ast/stmts/var_def.h>
#include <lexer/lexer.h>
#include <parser/parser.h>

using namespace veo;

test_func (Parser, VarDef) {
    auto                         path = GetFilePath ("test_var_def.veo");
    llvm::SourceMgr              mgr;
    diagnostic::DiagnosticEngine diag (mgr);
    auto bufferOrErr = llvm::MemoryBuffer::getFile (path.string ());
    if (std::error_code ec = bufferOrErr.getError ()) {
        std::cerr << "Could not open file: " << ec.message () << '\n';
        return false;
    }
    unsigned bufferId = mgr.AddNewSourceBuffer (std::move (*bufferOrErr), llvm::SMLoc ());

    basic::TypePool pool;
    Lexer           lex (diag, mgr, bufferId);
    Parser          parser (diag, lex, pool);
    ParseResult     parseRes = parser.Parse ();
    assert_eq (parseRes.HasErrors, false);
    assert_eq (parseRes.Count, 2);
    auto *stmt1 = llvm::cast<ast::Stmt> (parseRes.Nodes[0]);
    assert_eq ((int) stmt1->Kind (), (int) ast::NodeKind::VarDef);
    auto *var1 = llvm::cast<ast::VarDef> (stmt1);
    assert_eq (var1->Name ().Val, "x");

    auto *stmt2 = llvm::cast<ast::Stmt> (parseRes.Nodes[1]);
    assert_eq ((int) stmt2->Kind (), (int) ast::NodeKind::VarDef);
    auto *var2 = llvm::cast<ast::VarDef> (stmt2);
    assert_eq (var2->Name ().Val, "y");
    return true;
}
