#include "../framework.h"

int
main (int argc, char **argv) {
    return TestRegistry::RunAll ("Parser") ? 0 : 1;
}
