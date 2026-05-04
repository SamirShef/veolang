#include <diagnostic/engine.h>
#include <driver/cli_options.h>
#include <lexer/lexer.h>
#include <llvm/Support/raw_ostream.h>

using namespace veo;

int
main (int argc, char **argv) {
    if (driver::ParseArguments (argc, argv) != 0) {
        return 0;
    }
    driver::ExecuteArguments ();

    llvm::SourceMgr              mgr;
    diagnostic::DiagnosticEngine diag (mgr);

    diag.Render ();

    return 0;
}
