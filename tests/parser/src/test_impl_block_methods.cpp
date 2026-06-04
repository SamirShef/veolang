#include "../../framework.h"
#include "../context.h"

using namespace veo;
using namespace testing;

test_func (Parser, ImplBlockMethods) {
    ParserContext context;
    auto          res     = context.ParseFile ("veo/test_impl_block_methods.veo");
    const auto   &snaphot = ParserContext::GetSnaphot (res);
    assert_eq (res.HasErrors, false);
    const auto &expectedSnaphot = "ImplStmt: Point\n"
                                  "  Methods:\n"
                                  "    pub reset (): noth\n"
                                  "      ExprStmt:\n"
                                  "        AsgnExpr: =\n"
                                  "          FieldExpr: x\n"
                                  "            From:\n"
                                  "              VarExpr: this\n"
                                  "          LiteralExpr: 0.0\n"
                                  "      ExprStmt:\n"
                                  "        AsgnExpr: =\n"
                                  "          FieldExpr: y\n"
                                  "            From:\n"
                                  "              VarExpr: this\n"
                                  "          LiteralExpr: 0.0\n";
    assert_snaphot_eq (snaphot, expectedSnaphot);
    return true;
}
