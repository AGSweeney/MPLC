# MPLC documentation

| Document | Audience |
|----------|----------|
| [../README.md](../README.md) | Project overview, quick start, Studio summary |
| [architecture.md](architecture.md) | Compiler, VM, package format, tooling |
| [motion-hal.md](motion-hal.md) | Motion HAL contract, units, backend examples |
| [netburner-mod54415.md](netburner-mod54415.md) | MOD54415 bring-up, HTTP upload, firmware |
| [MPLC_Studio.png](MPLC_Studio.png) | Studio screenshot (also in Studio README) |

## Platform guides

| Path | Description |
|------|-------------|
| [../platforms/netburner/README.md](../platforms/netburner/README.md) | NetBurner MOD54415 platform |
| [../platforms/netburner/mod54415/nb_project/README.md](../platforms/netburner/mod54415/nb_project/README.md) | NNDK firmware project |

## Tools

| Path | Description |
|------|-------------|
| [../tools/mplc-studio/README.md](../tools/mplc-studio/README.md) | MPLC Studio IDE, build, Windows installer |

## CMake options (root)

| Option | Default | Purpose |
|--------|---------|---------|
| `MPLC_BUILD_COMPILER` | ON | Host `mplc` CLI |
| `MPLC_BUILD_RUNTIME` | ON | Portable runtime library |
| `MPLC_BUILD_TESTS` | ON | Unit tests |
| `MPLC_BUILD_STUDIO` | OFF | Qt MPLC Studio (`tools/mplc-studio`) |
