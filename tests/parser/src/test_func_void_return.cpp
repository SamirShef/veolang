#include "../../framework.h"
#include "../context.h"

using namespace veo;
using namespace testing;

test_func (Parser, FuncVoidReturn) {
    ParserContext context;
    auto          res     = context.ParseFile ("veo/test_func_void_return.veo");
    const auto   &snaphot = ParserContext::GetSnaphot (res);
    assert_eq (res.HasErrors, false);
    const auto &expectedSnaphot = "FuncDef: pub log_message (id: u64)\n"
                                  "  Return:\n";
    assert_snaphot_eq (snaphot, expectedSnaphot);
    return true;
}
