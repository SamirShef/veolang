#include <diagnostic/engine.h>
#include <driver/cli_options.h>
#include <llvm/Support/raw_ostream.h>

using namespace veo;

int
main (int argc, char **argv) {
    if (driver::ParseArguments (argc, argv) != 0) {
        return 0;
    }
    driver::ExecuteArguments ();

    return 0;
}
