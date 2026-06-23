# MPLC Architecture

MPLC is a portable IEC 61131-3 runtime written in C. A host-side compiler lowers
ST, IL, LD, FBD, and SFC sources (via PLCopen XML or native text) into a common
IR, then emits portable `.mplc` bytecode executed by a lightweight VM on MCUs
and Linux.

## Layers

1. **Compiler frontends** — language parsers produce typed IR (`compiler/common/ir.h`).
2. **Semantic analyzer** — symbol tables, type checks (`compiler/semantic/`).
3. **Codegen** — IR → stack bytecode (`compiler/codegen/`).
4. **Linker** — `.mplc` package builder (`compiler/linker/`).
5. **Runtime VM** — opcode interpreter (`runtime/core/vm.c`).
6. **Scheduler** — cyclic task execution (`runtime/core/scheduler.c`).
7. **I/O mapping** — `%I/%Q/%M` indices bound via HAL (`runtime/io/`).
8. **Standard library** — native IEC FBs (`runtime/stdlib/`).
9. **HAL** — platform contract (`hal/include/mplc_hal.h`).

## Package format

Binary `.mplc` files use little-endian layout defined in `schema/mplc_abi.h`:

- Header + section table
- CODE, DATA, SYMTAB (POU descriptors), IOMAP, TASKS, optional SFC/FBINST

## Ports

| Port | Path | Purpose |
|------|------|---------|
| Generic | `platforms/generic/` | Unit tests, simulation |
| Linux | `platforms/linux/` | POSIX reference runtime |
| Bare-metal | `platforms/baremetal/` | MCU template (RP2040-oriented stub) |
| NetBurner MOD54415 | `platforms/netburner/mod54415/` | NNDK 3.x / uC/OS-II (ColdFire) |

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build
```

## Tooling

```bash
build/mplc compile program.st -o program.mplc
build/mplc inspect program.mplc
build/mplc_linux program.mplc 100   # Linux only
```
