#include "../../framework.h"
#include "../context.h"

using namespace veo;
using namespace testing;

test_func (Parser, ImplTraitForStruct) {
    ParserContext context;
    auto          res     = context.ParseFile ("veo/test_impl_trait_for_struct.veo");
    const auto   &snaphot = ParserContext::GetSnaphot (res);
    assert_eq (res.HasErrors, false);
    const auto &expectedSnaphot = "ImplStmt: TDisplayable for Point\n"
                                  "  Methods:\n"
                                  "    pub render ()\n"
                                  "      ExprStmt:\n"
                                  "        FuncCall: log\n"
                                  "          FieldExpr: x\n"
                                  "            From:\n"
                                  "              VarExpr: this\n";
    assert_snaphot_eq (snaphot, expectedSnaphot);
    return true;
}
