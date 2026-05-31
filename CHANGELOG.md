# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.17.1] - 2026.05.31

### Added

- Implicit `this` has pointer to structure type yet

## [0.17.0] - 2026.05.31

### Added

- Pointers and `nil`
- Runtime check for `nil`
- Implicit dereferencing pointers when accessing members

## [0.16.3] - 2026.05.31

### Added

- Hexadecimal, octal and binary numbers lexing

## [0.16.2] - 2026.05.31

### Added

- Constant folding for ternary operators with constant condition

## [0.16.1] - 2026.05.31

### Added

- Dynamic global variables initialization

## [0.16.0] - 2026.05.29

### Added

- Explicit cast operator
- Explicit cast
- Implicit cast

## [0.15.0] - 2026.05.29

### Added

- Bitwise operators: `&`, `|`, `^` and `~`

## [0.14.6] - 2026.05.28

### Added

- '.' operator

### Fixed

- Incorrect operators precedence

### Changed

- Trigger for parse ExprStmt

## [0.14.5] - 2026.05.25

### Added

- Class veo::Mangler for mangling symbols

### Changed

- Removed lld libs for linking in CMakeLists.txt

## [0.14.4] - 2026.05.25

### Changed

- Incorrect some options description

## [0.14.3] - 2026.05.25

### Added

- Option `--target` was implemented

## [0.14.2] - 2026.05.25

### Added

- Option `--emit-asm` for emitting LLVM Module to Assembly Language

### Fixed

- Compilation error on Windows when implicit converting std::filesystem::path to std::string

## [0.14.1] - 2026.05.25

### Added

- Optimization options: -O0, -O1, -O2, -O3, -Os, -Oz

## [0.14.0] - 2026.05.22

### Added

- Parsing and generating struct definition
- Parsing and generating user's types
- Parsing and generating fields accessing
- Parsing and generating methods calling
- Parsing and generating implementation definition

## [0.13.0] - 2026.05.17

### Added

- For loop

## [0.12.1] - 2026.05.17

### Added

- Statement scope validation

## [0.12.0] - 2026.05.17

### Added

- If-else statements

## [0.11.0] - 2026.05.17

### Added

- Assignment of variables

## [0.10.2] - 2026.05.16

### Changed

- Prefix `_B` replaced to `_V` in mangling (i set `_B` by mistake)

## [0.10.1] - 2026.05.16

### Added

- Checking expression of global variables and constants (checking to be constant)

## [0.10.0] - 2026.05.16

### Added

- examples/ directory
- Implicit function declarations

## [0.9.0] - 2026.05.16

### Added

- Function calling

## [0.8.0] - 2026.05.16

### Added

- Class CodeGen for generating LLVM IR from HIR tree

## [0.7.0] - 2026.05.12

### Added

- HIR tree and builder of HIR
- Semantic analyzer of AST
- Lowering AST to HIR in Sema with builder
- AST node for 'return' statement

## [0.6.0] - 2026.05.10

### Added

- Dumps AST to file or stderr
- Compiler option for dump AST (--dump-ast=<none/file/term>)
- Converting types to string (virtual std::string Type::ToString() method)

## [0.5.0] - 2026.05.09

### Added

- Base parsing of variables and functions definition
- Unit tests for parser

## [0.4.0] - 2026.05.03

### Added

- Base lexing with throwing errors
- Unit tests for lexer

### Changed

- Added testing project in build action
- Deleted macOS from build action

## [0.3.0] - 2026.05.02

### Added

- Render diagnostic (errors and warnings)

## [0.2.0] - 2026.05.02

### Added

- Executing some command line arguments (except build or package manager commands)

## [0.1.0] - 2026.05.01

### Added

- Driver for parsing command line arguments
- .clangd file

### Changed

- Case style of functions in .clang-tidy (now it is CamelCase)
- Removed 'k' prefix for constants in .clang-tidy
- Disable `cert-err58-cpp` rule in .clang-tidy
- Added include_directories for include/ in the root of project
- Linking LLVM in CMakeLists.txt

## [0.0.1] - 2026.05.01

### Added

- Workflow for cross platform build project
- Formatting of .yml files in .editorconfig

## [0.0.0] - 2026.04.30

### Added

- README file
- LICENSE file
- CHANGELOG file
- .clang-format file
- .clang-tidy file
- .editorconfig file
- .gitignore file
- Simple CMakeLists.txt file with cross platform build
- Simple entry point
