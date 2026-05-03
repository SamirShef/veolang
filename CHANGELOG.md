# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.4.0] - 2026.05.03

### Added

- Base lexing (without reporting errors)
- Unit tests of lexer

### Changed

- Added testing project in build action

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
