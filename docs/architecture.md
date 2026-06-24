# MPLC Architecture

MPLC is a portable IEC 61131-3 runtime written in C. A host-side compiler lowers ST, IL, LD, FBD, and SFC sources (via PLCopen XML or native text) into a common IR, then emits portable `.mplc` bytecode executed by a lightweight VM on MCUs and Linux.

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

ST text supports `//` line comments, `(* block *)` comments, and `#` line comments (useful for file headers).

## Package format

Binary `.mplc` files use little-endian layout defined in `schema/mplc_abi.h`:

- Header + section table
- CODE, DATA, SYMTAB (POU descriptors), IOMAP, TASKS, optional SFC/FBINST

On big-endian targets (NetBurner MOD54415), the loader applies endian helpers from `schema/mplc_endian.h`.

## Ports

| Port | Path | Purpose |
|------|------|---------|
| Generic | `platforms/generic/` | Unit tests, simulation |
| Linux | `platforms/linux/` | POSIX reference runtime |
| Bare-metal | `platforms/baremetal/` | MCU template (RP2040-oriented stub) |
| NetBurner MOD54415 | `platforms/netburner/mod54415/` | NNDK 3.x / uC/OS-II (ColdFire) |

NetBurner firmware pulls `runtime/core/*.c` from the repo via the `nb_project` makefile `VPATH`—the on-target VM is the same sources as desktop builds, not a fork.

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build
```

Optional Qt IDE (requires Qt 6.8+):

```bash
cmake -B build_studio -DMPLC_BUILD_COMPILER=ON -DMPLC_BUILD_STUDIO=ON -DCMAKE_PREFIX_PATH=/path/to/Qt/6.8.3/msvc2022_64
cmake --build build_studio --config Release --target mplc_studio
```

Or use `tools/mplc-studio/scripts/build-mplc-studio.ps1` on Windows.

## Host tooling

### Command-line compiler

```bash
build/mplc compile program.st -o program.mplc
build/mplc inspect program.mplc
build/mplc_linux program.mplc 100   # Linux only
```

### MPLC Studio

Qt6 desktop app at `tools/mplc-studio/`:

- Embeds the same `compiler/` sources as `mplc.exe` (via `mplc_compiler` static library)
- Writes `<source>.mplc` next to the ST file on compile
- HTTP upload/reboot to NetBurner firmware (`mplc_upload.html`, `mplc_reboot.html`)
- UDP device discovery, serial monitor (Qt SerialPort), ST instruction toolbox

Windows installer: `tools/mplc-studio/scripts/build-mplc-studio-installer.ps1` (Inno Setup 6).

See [tools/mplc-studio/README.md](../tools/mplc-studio/README.md).

### NetBurner helpers

- `platforms/netburner/tools/mplc_to_carray.py` — embed a `.mplc` as `g_embedded_mplc[]` in C for firmware fallback

## Deployment models

| Model | Updates | When |
|-------|---------|------|
| HTTP upload | `.mplc` only | Normal ST program changes |
| Firmware reflash | Runtime + embedded fallback | VM/ABI changes, HAL, HTTP stack |
| Embed in firmware | Rebuild `embedded_mplc.c` + reflash | Default program baked into image |
