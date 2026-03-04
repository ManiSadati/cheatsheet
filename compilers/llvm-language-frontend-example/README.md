# LLVM Language Frontend Example (`codegen.cpp`)

This directory contains a single-file, Kaleidoscope-style language frontend and JIT example built with LLVM.

## What `codegen.cpp` does

`codegen.cpp` implements a tiny interactive language that:

- tokenizes input (lexer)
- parses expressions into an AST (parser)
- generates LLVM IR for:
  - numeric literals and variables
  - binary operators: `<`, `+`, `-`, `*`
  - function calls
  - `if/then/else`
  - `for/in`
  - function definitions (`def`) and extern declarations (`extern`)
- runs simple optimization passes (`InstCombine`, `Reassociate`, `GVN`, `SimplifyCFG`)
- JIT-compiles and executes top-level expressions with ORC JIT

It also exports two helper functions you can call from the toy language:

- `putchard(x)`
- `printd(x)`

## Prerequisites

- LLVM development packages installed (headers, libs, `llvm-config`)
- a C++ compiler (`clang++` or `g++`) with C++17 support
- `KaleidoscopeJIT.h` available next to `codegen.cpp`

Note: this folder currently has only `codegen.cpp`. You also need `KaleidoscopeJIT.h` from the LLVM Kaleidoscope tutorial sources for this to compile.

## Build

From this directory:

```bash
clang++ -std=c++17 codegen.cpp \
  $(llvm-config --cxxflags --ldflags --system-libs --libs core orcjit native passes) \
  -O2 -o kaleidoscope
```

If your LLVM install does not expose `orcjit` under that name, check available lib names with:

```bash
llvm-config --libs
```

## Run

Start the REPL:

```bash
./kaleidoscope
```

You should see prompts like:

```text
ready>
```

### Example session

```text
def add(x y) x + y
add(2, 3)

extern printd(x)
printd(42)

if 1 then 10 else 20

for i = 1, i < 5, 1 in printd(i)
```

Top-level expressions are JIT-executed and print `Evaluated to ...`.
