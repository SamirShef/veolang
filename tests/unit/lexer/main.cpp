#include "all.h"
#include "comments.h"
#include "ids.h"
#include "literals.h"
#include "operators.h"

int
main (int argc, char **argv) {
    call_test (TestComments);
    call_test (TestLiterals);
    call_test (TestIds);
    call_test (TestOperators);
    call_test (TestAll);
    return 0;
}
