#include "../../framework.h"
#include "../context.h"

using namespace veo;
using namespace testing;

test_func (Parser, ComplexBinaryExpr) {
    ParserContext context;
    auto          res     = context.ParseFile ("veo/test_complex_binary_expr.veo");
    const auto   &snaphot = ParserContext::GetSnaphot (res);
    assert_eq (res.HasErrors, false);
    const auto &expectedSnaphot = "VarDef: priv let val\n"
                                  "  BinaryExpr: /\n"
                                  "    BinaryExpr: *\n"
                                  "      BinaryExpr: +\n"
                                  "        VarExpr: a\n"
                                  "        VarExpr: b\n"
                                  "      VarExpr: c\n"
                                  "    BinaryExpr: %\n"
                                  "      VarExpr: d\n"
                                  "      VarExpr: e\n";
    assert_snaphot_eq (snaphot, expectedSnaphot);
    return true;
}
