# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

xdrzcc is a specialized C compiler that transforms XDR (External Data Representation) definitions into high-performance C code. It's designed for zero-copy data serialization, primarily used for parsing Network File System (NFS) traffic.

## Build Commands

```bash
# Default build (release + tests)
make

# Specific builds
make build_release    # Release build
make build_debug      # Debug build
make test_release     # Run tests (release)
make test_debug       # Run tests (debug)
make clean           # Clean build artifacts

# Direct CMake usage
mkdir -p build && cd build
cmake .. -GNinja -DCMAKE_BUILD_TYPE=Release
ninja
```

## Running Tests

```bash
# Run all tests
make test_release

# Run specific test manually
./build/xdrzcc test/test.x test_out.c test_out.h
# Then compile: cc -Wall -o test_prog test_out.c test_main.c

# Test with valgrind
valgrind --leak-check=full ./build/xdrzcc test/test.x out.c out.h
```

## Architecture

### Compilation Pipeline
1. **Lexer** (`lexer.l`) - Tokenizes XDR input using Flex
2. **Parser** (`parser.y`) - Builds AST using Bison
3. **AST** (`ast.h/c`) - Abstract syntax tree representation
4. **Code Generator** (`codegen.c`) - Emits C code with zero-copy optimizations

### Key Components

- **Zero-Copy Design**: Uses I/O vectors (`codegen_runtime.c`) to avoid memory copies
- **Memory Management**: Arena allocators (`arenaalloc.h`) for efficient memory usage
- **Type System**: Handles XDR types (int, bool, string, arrays, unions, structs, enums)
- **Dependency Resolution**: Topological sort ensures correct type ordering

### Generated Code Features
Each XDR type generates:
- `marshall_*` - Serialize to wire format
- `unmarshall_*` - Deserialize from wire format
- `len_*` - Calculate serialized size
- `str_*` - Debug string representation

## Usage Example

```bash
# Generate C code from XDR definition
./xdrzcc input.x output.c output.h

# Use generated code
#include "output.h"
// marshall_YourType(), unmarshall_YourType(), etc.
```

## Development Guidelines

- Maintain ANSI C compatibility in generated code
- No external runtime dependencies (only libc)
- All runtime support must be embedded in generated files
- Follow existing patterns for error handling (return codes, not exceptions)
- Test with various XDR specifications from `test/` directory