#include <driver/cli_options.h>

using namespace veo;

int
main (int argc, char **argv) {
    if (!driver::ParseArguments (argc, argv)) {
        return 0;
    }
    driver::ExecuteArguments ();

    return 0;
}
