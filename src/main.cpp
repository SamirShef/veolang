#include <diagnostic/codes.h>
#include <diagnostic/engine.h>
#include <diagnostic/span.h>
#include <driver/cli_options.h>
#include <fstream>
#include <llvm/Support/raw_ostream.h>

using namespace veo;

int
main (int argc, char **argv) {
    if (driver::ParseArguments (argc, argv) != 0) {
        return 0;
    }
    driver::ExecuteArguments ();

    std::ofstream testFile ("test.veo");
    testFile << "func main(): i32 {\n    return 0\n}\n";
    testFile.close ();

    llvm::SourceMgr mgr;
    auto            memBufOrErr = llvm::MemoryBuffer::getFile ("test.veo");
    if (std::error_code ec = memBufOrErr.getError ()) {
        llvm::errs () << "error\n";
        return 1;
    }
    unsigned buffer = mgr.AddNewSourceBuffer (
            std::move (*memBufOrErr),
            llvm::SMLoc ());
    diagnostic::DiagnosticEngine diag (mgr);
    const char *bufStart = mgr.getMemoryBuffer (buffer)->getBufferStart ();
    diag.Report (
                diagnostic::DiagCode::EUnexpectedToken,
                "expected ';'",
                diagnostic::Severity::Error)
            .AddSpan (
                    diagnostic::Span (
                            llvm::SMLoc::getFromPointer (bufStart + 23),
                            llvm::SMLoc::getFromPointer (bufStart + 28)),
                    "unexpected token",
                    false)
            .AddSpan (
                    diagnostic::Span (
                            llvm::SMLoc::getFromPointer (bufStart + 32),
                            llvm::SMLoc::getFromPointer (bufStart + 32)),
                    "unexpected token",
                    true)
            .AddNote ("add ';' at the end of line");

    diag.Report (
                diagnostic::DiagCode::EUnexpectedToken,
                "expected ';'",
                diagnostic::Severity::Error)
            .AddSpan (
                    diagnostic::Span (
                            llvm::SMLoc::getFromPointer (bufStart + 23),
                            llvm::SMLoc::getFromPointer (bufStart + 28)),
                    "unexpected token",
                    false)
            .AddSpan (
                    diagnostic::Span (
                            llvm::SMLoc::getFromPointer (bufStart + 32),
                            llvm::SMLoc::getFromPointer (bufStart + 32)),
                    "unexpected token",
                    true)
            .AddNote ("add ';' at the end of line");

    diag.Render ();
    fs::remove ("test.veo");

    return 0;
}
