<h1 align="center">The Veo Programming Language</h1>
<p align="center">
    <img src="lynx_logo.png" alg="Veo Mascot: Casper" width="25%" />
</p>

**Veo** - compiled, high-performance systems programming language designed for building reliable, secure, and resource-constrained software.
It combines zero-cost abstractions and bare-metal performance with modern safety guarantees, aiming to prevent common memory management and concurrency defects
without the overhead of a garbage collector.

## Example

```veo
import std.io;

func main() {
    let message = "Hello, Veo!";
    io.println(message);
}
```

## Getting Started

### Prerequisites

To build the compiler from source, you need a modern C++ toolchain (C++20/C++23), CMake, and LLVM 21 development libraries.

#### Linux (Ubuntu / Debian)

```bash
# Install build essentials and CMake
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build

# Install LLVM 21 (using the official LLVM repository if not available in default package manager)
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
sudo ./llvm.sh 21
sudo apt-get install -y llvm-21-dev libclang-21-dev lld-21
```

#### Linux (Arch Linux)

```bash
sudo pacman -Syu --needed base-devel cmake ninja llvm21 lld clang21
```

#### Windows (via MSYS2)

* Download and install [MSYS2](https://www.msys2.org/).
* Open the **MSYS2 UCRT64** terminal and run the following command to install the required toolchain and LLVM 21:

```bash
pacman -Syu --needed \
    mingw-w64-ucrt-x86_64-llvm-21 \
    mingw-w64-ucrt-x86_64-clang-21 \
    mingw-w64-ucrt-x86_64-cmake \
    mingw-w64-ucrt-x86_64-ninja
```

### Building from Source

#### On Linux / macOS

* Clone the repository:

```bash
git clone https://github.com/SamirShef/veolang.git
cd veolang
```

* Configure the project using CMake. It is recommended to use lld for faster linking:

```bash
cmake -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=lld" \
  -DCMAKE_SHARED_LINKER_FLAGS="-fuse-ld=lld"
```

> [!NOTE]
> If CMake cannot find LLVM, explicitly pass the path: -DLLVM_DIR="/usr/lib/llvm-21/lib/cmake/llvm".

* Compile the compiler and run the test suite:

```bash
# Build the project
cmake --build build --config Release --parallel $(nproc)

# Run tests
cd build
ctest --output-on-failure
```

#### On Windows (MSYS2 UCRT64)

* Open the **MSYS2 UCRT64** environment.
* Clone the repository and navigate into it:

```bash
git clone https://github.com/SamirShef/veolang.git
cd veolang
```

* Ensure the LLVM 21 binaries are in your environment path, then configure the build using Ninja and Clang:

```bash
export PATH="/ucrt64/opt/llvm-21/bin:$PATH"

cmake -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_DIR=/ucrt64/opt/llvm-21/lib/cmake/llvm \
  -DCMAKE_C_COMPILER=/ucrt64/opt/llvm-21/bin/clang.exe \
  -DCMAKE_CXX_COMPILER=/ucrt64/opt/llvm-21/bin/clang++.exe
```

* Compile and test:

```bash
# Build the project
cmake --build build --config Release --parallel $(nproc)

# Run tests
cd build
ctest --output-on-failure
```

### Verifying the Installation

You can test the compiler by creating and building a new boilerplate project:

```bash
# Create a new project template
./veoc new test_project
cd test_project

# Compile the newly created project using the veoc compiler
./../veoc build
```

> [!WARNING]
> On Windows, replace ./veoc with ./veoc.exe

## License

This project is licensed under the Apache License 2.0 - see the [LICENSE](LICENSE) file for details.
