# MPLC — Portable IEC 61131 Runtime

MPLC is a host-compiled IEC 61131-3 toolchain and fixed-memory bytecode runtime written in C for embedded controllers and Linux targets. The runtime and HAL are portable across platforms; **MPLC Studio** currently provides an integrated edit, compile, deploy, and serial-debug workflow for NetBurner MOD54415 hardware.

## Features

- **Languages:** ST (working); IL, LD, FBD, SFC (experimental); PLCopen XML body import
- **Portable VM:** fixed-memory stack machine with native standard FB dispatch
- **HAL boundary:** GPIO/analog/time/retain abstracted per platform
- **Motion:** PLCopen-inspired motion runtime with native `MC_*` function blocks and portable hardware backends — see [docs/motion-hal.md](docs/motion-hal.md)
- **Package format:** versioned `.mplc` binary (no dynamic linking on target)
- **MPLC Studio:** ST editor; Ladder Diagram editor (**early development**); in-process compile, device discovery, HTTP upload/reboot, serial monitor — see [tools/mplc-studio/README.md](tools/mplc-studio/README.md)

## Example (ST)

```st
VAR
    Input  AT %IX0.0 : BOOL;
    Output AT %QX0.0 : BOOL;
END_VAR

PROGRAM Main
    Output := Input;
END_PROGRAM
```

Compile with `mplc compile program.st -o program.mplc` (see Quick start below).

## Quick start (toolchain)

```bash
cmake -B build
cmake --build build
./build/mplc compile tests/compiler/sample.st -o build/sample.mplc
./build/mplc inspect build/sample.mplc
ctest --test-dir build
```

On Windows (Visual Studio generator):

```powershell
cmake -B build_local
cmake --build build_local --config Release --target mplc
build_local\compiler\Release\mplc.exe compile tests\compiler\sample.st -o build_local\sample.mplc
```

ST sources may use `//`, `(* *)`, and `#` line comments (same as the embedded Studio compiler).

## MPLC Studio (NetBurner workflow)

![MPLC Studio — Structured Text](docs/MPLC_Studio.png)

![MPLC Studio — Ladder Diagram](docs/MPLC_Studio_Ladder.png)

The **ladder editor is in early development** — the UI, branch model, and LD compiler path are functional but still evolving; expect rough edges compared to Structured Text.

The ladder editor (`tools/mplc-studio/app/src/ladder/`) is a grid-based Qt Graphics View canvas: rungs hold contacts, coils, and function blocks; **Surround with Branch** / **+ Leg** build nested input OR branches; output branches fork parallel coils (with optional instructions before each coil). Programs persist as PLCopen TC6 XML (`.xml`) and compile through the LD frontend (`compiler/frontend/ld/`) into the same `.mplc` bytecode as ST. **View Generated ST** exports ladder logic for inspection.

```powershell
# Build the IDE
cd tools\mplc-studio
.\scripts\build-mplc-studio.ps1 -QtDir "C:\Qt\6.8.3\msvc2022_64"

# Optional: Windows installer (Inno Setup 6)
.\scripts\build-mplc-studio-installer.ps1 -QtDir "C:\Qt\6.8.3\msvc2022_64"
```

Output: `tools/mplc-studio/build/app/MPLCStudio.exe` or `tools/mplc-studio/dist/MPLCStudio-0.1.1-Setup.exe`

Typical loop: edit `.st` → **Compile** (writes `<source>.mplc` beside the file) → **Upload** → **Reboot** → watch boot logs on the **Serial Monitor** tab (115200, DTR/RTS on).

## Documentation

| Document | Description |
|----------|-------------|
| [docs/architecture.md](docs/architecture.md) | Compiler, VM, package format, ports |
| [docs/motion-hal.md](docs/motion-hal.md) | Motion HAL, units, backend integration examples |
| [docs/README.md](docs/README.md) | Documentation index |
| [docs/netburner-mod54415.md](docs/netburner-mod54415.md) | MOD54415 bring-up and deployment |
| [platforms/netburner/README.md](platforms/netburner/README.md) | NetBurner platform overview |
| [platforms/netburner/mod54415/nb_project/README.md](platforms/netburner/mod54415/nb_project/README.md) | NBEclipse / `make` firmware project |
| [tools/mplc-studio/README.md](tools/mplc-studio/README.md) | Studio features, build, installer |

## Layout

```
compiler/          Host compiler (ST/IL/LD/FBD/SFC → .mplc)
runtime/           Portable VM, scheduler, I/O, stdlib
schema/            Package ABI (mplc_abi.h)
hal/               HAL contract
platforms/         generic, linux, baremetal, netburner
tools/mplc-studio/ Qt IDE + Inno Setup installer
tests/             Compiler and runtime unit tests
docs/              Architecture and platform guides
```

## License

BSD 3-Clause License. See [LICENSE](LICENSE).
