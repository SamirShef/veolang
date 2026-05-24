#include "../../framework.h"
#include "../context.h"

using namespace veo;
using namespace testing;

test_func (Parser, ForInfiniteLoop) {
    ParserContext context;
    auto          res     = context.ParseFile ("veo/test_for_infinite_loop.veo");
    const auto   &snaphot = ParserContext::GetSnaphot (res);
    assert_eq (res.HasErrors, false);
    const auto &expectedSnaphot = "ForLoopStmt:\n"
                                  "  Body:\n"
                                  "    BreakContinue: Break\n";
    assert_snaphot_eq (snaphot, expectedSnaphot);
    return true;
}
