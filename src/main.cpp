#include <driver/cli_options.h>

using namespace veo;

int
main (int argc, char **argv) {
    if (driver::ParseArguments (argc, argv) != 0) {
        return 0;
    }
    return 0;
}
