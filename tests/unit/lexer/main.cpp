#include "all.h"
#include "comments.h"
#include "ids.h"
#include "literals.h"
#include "operators.h"

int
main (int argc, char **argv) {
    call_test (TestComments, argv[1]);
    call_test (TestLiterals, argv[1]);
    call_test (TestIds, argv[1]);
    call_test (TestOperators, argv[1]);
    call_test (TestAll, argv[1]);
    return 0;
}
