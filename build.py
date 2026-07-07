import sys
import subprocess
import platform
from pathlib import Path


# --- ANSI Escape Sequences for Colored Diagnostics ---
class Log:
    RESET = "\033[0m"
    BOLD = "\033[1m"
    RED = "\033[31m"
    GREEN = "\033[32m"
    YELLOW = "\033[33m"
    BLUE = "\033[34m"
    PURPLE = "\033[35m"
    CYAN = "\033[36m"
    WHITE = "\033[37m"

    @staticmethod
    def info(msg):
        print(f"{Log.CYAN}[*] {msg}{Log.RESET}")

    @staticmethod
    def success(msg):
        print(f"{Log.BOLD}{Log.GREEN}[+] {msg}{Log.RESET}")

    @staticmethod
    def warn(msg):
        print(f"{Log.YELLOW}[!] {msg}{Log.RESET}")

    @staticmethod
    def error(msg):
        print(
            f"{Log.BOLD}{Log.RED}[-] CRITICAL ERROR: {msg}{Log.RESET}", file=sys.stderr
        )

    @staticmethod
    def trace(msg):
        print(f"{Log.WHITE}    {msg}{Log.RESET}")

    @staticmethod
    def step(num, msg):
        print(f"\n{Log.BOLD}{Log.PURPLE}=== STEP {num}: {msg} ==={Log.RESET}")


def get_llvm_host_target():
    try:
        result = subprocess.run(
            ["llvm-config", "--host-target"], capture_output=True, text=True, check=True
        )
        return result.stdout.strip()
    except (subprocess.CalledProcessError, FileNotFoundError):
        Log.error("'llvm-config' was not found or failed to query target triple.")
        Log.trace(
            "Please verify that LLVM is properly installed and exported to your PATH."
        )
        sys.exit(1)


def get_llvm_flags():
    try:
        # Dynamic linking flags targeting system libLLVM.so configuration
        cmd = ["llvm-config", "--libs", "--system-libs", "--ldflags"]
        result = subprocess.run(cmd, capture_output=True, text=True, check=True)
        return result.stdout.replace("\n", " ").split()
    except (subprocess.CalledProcessError, FileNotFoundError):
        Log.error("Failed to extract compiler flags using 'llvm-config'.")
        sys.exit(1)


# --- ENVIRONMENT CONFIGURATION ---
TARGET_TRIPLE = get_llvm_host_target()
TARGET_OS = platform.system().lower()

SRC_DIR = Path("src")
BUILD_DIR = Path("build") / "targets" / TARGET_TRIPLE
OBJ_DIR = BUILD_DIR / "obj"
OUTPUT_EXE = BUILD_DIR / (
    "veoc-stage1.exe" if TARGET_OS == "windows" else "veoc-stage1"
)
OBJ_EXT = ".obj" if TARGET_OS == "windows" else ".o"

STAGE0_COMPILER = (
    Path("stage0/build/veoc.exe")
    if TARGET_OS == "windows"
    else Path("stage0/build/veoc")
)


def main():
    # --- VERBOSE DEBUG TELEMETRY DUMP ---
    print(
        f"{Log.BOLD}{Log.BLUE}===================================================={Log.RESET}"
    )
    print(
        f"{Log.BOLD}{Log.BLUE}          Veo Compiler Stage1 Build Pipeline        {Log.RESET}"
    )
    print(
        f"{Log.BOLD}{Log.BLUE}===================================================={Log.RESET}"
    )
    Log.info("Bootstrap Environment Metadata:")
    Log.trace(f"Host Triple   : {Log.YELLOW}{TARGET_TRIPLE}")
    Log.trace(f"Operating Sys : {TARGET_OS}")
    Log.trace(f"Stage0 Binary : {STAGE0_COMPILER}")
    Log.trace(f"Object Output : {OBJ_DIR}")
    Log.trace(f"Target Binary : {OUTPUT_EXE}")

    # Ensure output tree initialization
    OBJ_DIR.mkdir(parents=True, exist_ok=True)

    # --- STEP 1: COMPILATION ---
    Log.step(1, "Invoking Stage0 Driver")

    # Execution Pass: Clean
    clean_cmd = [str(STAGE0_COMPILER), "clean"]
    Log.info(f"Executing tree sanitation: {' '.join(clean_cmd)}")
    try:
        subprocess.run(clean_cmd, check=True, capture_output=True)
    except subprocess.CalledProcessError as e:
        Log.error("Stage0 cache clearance routine crashed.")
        Log.trace(e.stderr.decode() if e.stderr else str(e))
        sys.exit(1)

    # Execution Pass: Build
    build_cmd = [str(STAGE0_COMPILER), "build", "--no-link"]
    Log.info(f"Executing translation pass: {' '.join(build_cmd)}")
    try:
        subprocess.run(build_cmd, check=True)
    except subprocess.CalledProcessError:
        Log.error("Stage0 failed to parse and generate AST/HIR representation.")
        sys.exit(1)

    # --- STEP 2: OBJECT PROCESSING ---
    Log.step(2, "Resolving Module Topological Order")
    manifest_path = OBJ_DIR / "modules.txt"

    if manifest_path.exists():
        Log.info(
            f"Discovered compilation manifest block at: {Log.YELLOW}{manifest_path}"
        )
        with open(manifest_path, "r") as f:
            object_files = [line.strip() for line in f if line.strip()]
        Log.success("Successfully ingested safe linkage topology directly from Stage0.")
    else:
        Log.warn(
            "Compilation manifest missing. Falling back to unchecked filesystem discovery (rglob)."
        )
        object_files = [str(p) for p in OBJ_DIR.rglob(OBJ_EXT)]

    if not object_files:
        Log.error(
            f"No translation units detected ({OBJ_EXT}) in compilation context '{OBJ_DIR}'"
        )
        sys.exit(1)

    Log.info(
        f"Discovered {Log.BOLD}{len(object_files)}{Log.RESET} compilation modules scheduled for linkage:"
    )
    for idx, obj in enumerate(object_files, 1):
        Log.trace(f"  [{idx:02d}] -> {obj}")

    # --- STEP 3: LINKING ---
    Log.step(3, "Executing Final Executable Linkage")
    Log.info("Fetching LLVM Library dependencies via llvm-config...")
    llvm_flags = get_llvm_flags()

    # Build linker argument vector via native driver architecture fallback
    linker_cmd = ["clang++"] + object_files + ["-o", str(OUTPUT_EXE)] + llvm_flags

    if TARGET_OS == "linux":
        linker_cmd += ["-lrt", "-ldl", "-lpthread", "-lm"]
    elif TARGET_OS == "darwin":
        linker_cmd += ["-lresolv"]

    Log.info("Dispatching Linker Argument Array:")
    # Pretty-print long strings by grouping raw flags smoothly
    Log.trace(f"{Log.WHITE}{' '.join(linker_cmd)}")

    try:
        subprocess.run(linker_cmd, check=True, capture_output=True)
        print(
            f"\n{Log.BOLD}{Log.BLUE}===================================================={Log.RESET}"
        )
        Log.success("Stage1 Self-Hosted Core Compiled Successfully!")
        Log.trace(f"Artifact Location: {Log.BOLD}{Log.GREEN}{OUTPUT_EXE}{Log.RESET}")
        print(
            f"{Log.BOLD}{Log.BLUE}===================================================={Log.RESET}"
        )
    except subprocess.CalledProcessError as e:
        Log.error(
            "Linker subsystem generation mismatch (Undefined references present)."
        )
        if e.stderr:
            print(f"{Log.RED}{e.stderr.decode()}{Log.RESET}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
