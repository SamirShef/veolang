#pragma once
#include "../boilerplate.h"

using namespace veo;
namespace fs = std::filesystem;

// NOLINTBEGIN(readability-function-cognitive-complexity)
// NOLINTBEGIN(readability-simplify-boolean-expr)
inline void
TestVarDef (const fs::path &root) {
    init (root, "var_def.veo");
    Lexer  lexer (diag, mgr, bufferId);
    Parser parser (diag, lexer);
    auto   res = parser.Parse ();
    assert_eq (res.HasErrors, false);
    assert_eq (res.Count, 6);
    assert_var_def_with_type (res.Nodes[0], "a", false, true);
    assert_var_def_without_type (res.Nodes[1], "b", false, true);
    assert_var_def_with_type (res.Nodes[2], "c", false, false);
    assert_var_def_with_type (res.Nodes[3], "d", true, true);
    assert_var_def_without_type (res.Nodes[4], "e", true, true);
    assert_var_def_with_type (res.Nodes[5], "f", true, false);
}
// NOLINTEND(readability-simplify-boolean-expr)
// NOLINTEND(readability-function-cognitive-complexity)
