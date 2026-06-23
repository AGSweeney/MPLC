# MPLC — Portable IEC 61131 Runtime

MPLC is a host-compiled, bytecode-executed IEC 61131-3 runtime in C, targeting
bare-metal microcontrollers and Linux devices.

## Features

- **Languages:** ST, IL, LD, FBD, SFC (via native parsers + PLCopen XML)
- **Portable VM:** fixed-memory stack machine with native standard FB dispatch
- **HAL boundary:** GPIO/analog/time/retain abstracted per platform
- **Package format:** versioned `.mplc` binary (no dynamic linking on target)

## Quick start

```bash
cmake -B build
cmake --build build
./build/mplc compile tests/compiler/sample.st -o build/sample.mplc
./build/mplc inspect build/sample.mplc
ctest --test-dir build
```

## Layout

See [docs/architecture.md](docs/architecture.md) for the full design.

Target platform guides:
- [NetBurner MOD54415](docs/netburner-mod54415.md)

## License

MIT (add license file as needed for your deployment).
