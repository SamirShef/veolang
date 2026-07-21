#include "../../framework.h"
#include "../context.h"

using namespace veo;
using namespace testing;

test_func (Parser, StructInstanceCreation) {
    ParserContext context;
    auto          res     = context.ParseFile ("veo/test_struct_instance_creation.veo");
    const auto   &snaphot = ParserContext::GetSnaphot (res);
    assert_eq (res.HasErrors, false);
    const auto &expectedSnaphot = "VarDef: priv let origin\n"
                                  "  StructInstance: Point\n"
                                  "    Fields:\n"
                                  "      x:\n"
                                  "        LiteralExpr: 0.0\n"
                                  "      y:\n"
                                  "        LiteralExpr: 0.0\n";
    assert_snaphot_eq (snaphot, expectedSnaphot);
    return true;
}
