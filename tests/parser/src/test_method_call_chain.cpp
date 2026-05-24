#include "../../framework.h"
#include "../context.h"

using namespace veo;
using namespace testing;

test_func (Parser, MethodCallChain) {
    ParserContext context;
    auto          res     = context.ParseFile ("veo/test_method_call_chain.veo");
    const auto   &snaphot = ParserContext::GetSnaphot (res);
    assert_eq (res.HasErrors, false);
    const auto &expectedSnaphot = "VarDef: priv let result\n"
                                  "  MethodCall: build\n"
                                  "    From:\n"
                                  "      MethodCall: set_name\n"
                                  "        LiteralExpr: true\n"
                                  "        From:\n"
                                  "          MethodCall: set_id\n"
                                  "            LiteralExpr: 1\n"
                                  "            From:\n"
                                  "              VarExpr: builder\n";
    assert_snaphot_eq (snaphot, expectedSnaphot);
    return true;
}
