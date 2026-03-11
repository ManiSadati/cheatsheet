# LLVM Pass Example

## Goal

This experiment shows a simple LLVM fault-injection workflow:

- `Assignment.cpp` implements the LLVM `ModulePass` named `faultinjection`.
- `FaultInjection.cpp` provides the runtime helpers that the pass inserts into the program.

## Idea

The pass looks for a few specific operations and rewrites them so faults can be injected at runtime.

- `malloc` calls are replaced with `mallocFI(size, counter)`.
- `calloc` calls are replaced with `callocFI(nmemb, size, counter)`.
- `sub` instructions are wrapped with a conditional check so the runtime can selectively turn one subtraction into an addition.
- A call to `setRandomInjection(...)` is inserted into `main` to choose which allocation or subtraction will be faulted for the current run.

## Files

### `Assignment.cpp`

This is the LLVM instrumentation pass.

- It scans every function in the module.
- It replaces `malloc` and `calloc` with instrumented wrappers.
- It counts `sub` instructions and injects a `shouldInjectSub(...)` check before each one.
- For a selected subtraction, it returns `a + b` instead of `a - b`.
- It inserts one setup call in `main` so the runtime knows how many candidate instructions exist and which fault mode to use.

### `FaultInjection.cpp`

This is the runtime support library used by the transformed program.

- It stores the selected fault type and the randomly chosen target instruction index.
- `setRandomInjection(...)` chooses one target for the current execution.
- `mallocFI(...)` and `callocFI(...)` either behave normally or inject memory faults.
- `shouldInjectSub(...)` tells the instrumented subtraction whether it should flip from `sub` to `add`.

Supported fault modes:

- `0`: `BUF_OVERFLOW` by undersizing an allocation.
- `1`: `MEM_EXHAUST` by returning `nullptr` from an allocation wrapper.
- `2`: `LOGIC_FAULT` by changing one subtraction into an addition.

## How to Run

Build the pass and runtime:

```bash
cd compilers/llvm-pass-example

clang++ -fPIC -shared Assignment.cpp -o Assignment.so \
  `llvm-config --cxxflags --ldflags --system-libs --libs core analysis passes support`

clang++ -fPIC -shared FaultInjection.cpp -o FaultInjection.so
```

Generate LLVM IR for the sample program:

```bash
clang++ -O0 -g -S -emit-llvm -fno-discard-value-names \
  -o TestCases/Quicksort/Quicksort.ll \
  TestCases/Quicksort/Quicksort.cpp
```

Run the pass and produce instrumented IR:

```bash
opt -S -enable-new-pm=0 \
  -load ./Assignment.so \
  -faultinjection -fi-type=2 \
  < TestCases/Quicksort/Quicksort.ll \
  > TestCases/Quicksort/Quicksort_Instr.ll
```

Link the instrumented IR with the runtime and run it:

```bash
clang++ TestCases/Quicksort/Quicksort_Instr.ll ./FaultInjection.so \
  -o TestCases/Quicksort/Quicksort_fi

./TestCases/Quicksort/Quicksort_fi
```

## Example

To test a logic fault on Quicksort, use `-fi-type=2`.

```bash
cd compilers/llvm-pass-example

clang++ -O0 -g -S -emit-llvm -fno-discard-value-names \
  -o TestCases/Quicksort/Quicksort.ll \
  TestCases/Quicksort/Quicksort.cpp

opt -S -enable-new-pm=0 \
  -load ./Assignment.so \
  -faultinjection -fi-type=2 \
  < TestCases/Quicksort/Quicksort.ll \
  > TestCases/Quicksort/Quicksort_Instr.ll

clang++ TestCases/Quicksort/Quicksort_Instr.ll ./FaultInjection.so \
  -o TestCases/Quicksort/Quicksort_fi

./TestCases/Quicksort/Quicksort_fi
```

For allocation faults, change `-fi-type=2` to:

- `-fi-type=0` for undersized allocations
- `-fi-type=1` for simulated allocation failure

## Results

The transformed binary behaves like the original program except when the randomly selected target is hit for the chosen fault mode.
