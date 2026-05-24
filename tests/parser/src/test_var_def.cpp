#include "../../framework.h"
#include "../context.h"

#include <ast/stmts/var_def.h>
#include <lexer/lexer.h>
#include <parser/parser.h>

using namespace veo;
using namespace testing;

test_func (Parser, VarDef) {
    ParserContext context;
    auto          res     = context.ParseFile ("veo/test_var_def.veo");
    const auto   &snaphot = ParserContext::GetSnaphot (res);
    assert_eq (res.HasErrors, false);
    const auto &expectedSnaphot = "VarDef: priv let x\n"
                                  "  LiteralExpr: 10\n"
                                  "\n"
                                  "VarDef: priv let y\n"
                                  "  BinaryExpr: +\n"
                                  "    VarExpr: x\n"
                                  "    LiteralExpr: 2\n";
    assert_snaphot_eq (snaphot, expectedSnaphot);
    return true;
}
