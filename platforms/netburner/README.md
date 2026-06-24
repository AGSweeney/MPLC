# NetBurner MOD54415 Platform

MPLC port for the NetBurner MOD54415 (NXP MCF54415, ColdFire big-endian) running **NNDK 3.x** and **uC/OS-II**. The on-target firmware uses the same **core runtime** (`runtime/`) and **ABI** (`schema/mplc_abi.h`) as desktop builds—not a forked VM.

| Item | Value |
|------|-------|
| Module | MOD54415 on MOD5441X platform |
| CPU | MCF54415 @ 250 MHz, **big-endian** |
| RTOS tick | **100 Hz** (10 ms) — see `nb_project/overload/nbrtos/include/constants-overload.h` |
| PLC scan | **10 ms** default — `MPLC_NB_DEFAULT_CYCLE_US` in `hal_netburner_config.h` |
| Carrier | MOD-DEV-70 (8 DIP + 8 LED) pin map in `hal_netburner_config.h` |

## Directory layout

```
platforms/netburner/
├── README.md
├── CMakeLists.txt                 Host HAL library + `mplc_netburner_test` smoke test
├── tools/
│   └── mplc_to_carray.py          Embed a `.mplc` file as `g_embedded_mplc[]`
└── mod54415/
    ├── hal_netburner.cpp          MOD-DEV-70 HAL (DIP ADC, LED GPIO, timing)
    ├── hal_netburner_config.h     Pin map, scan period, file-boot paths, debug flags
    ├── mplc_hal_netburner.h       NetBurner HAL / PLC helpers
    ├── mplc_plc_runtime.c         Runtime init, package load, cycle runner
    ├── mplc_task.cpp              uC/OS-II cyclic PLC task
    ├── mplc_nb_fs.c / .h          EFFS-STD file read for `mplc_startup.mplc`
    ├── embedded_stub.c            Empty package for CMake host test
    ├── embedded_sample.c          Tiny sample blob (host / reference)
    ├── host_test_main.c           CMake `mplc_netburner_test` entry
    ├── mod_dev70_dips_to_leds.st  DIP → LED mirror (bring-up test)
    ├── mod_dev70_dip_blink_*.st   DIP-gated LED blink examples
    └── nb_project/                NBEclipse / `make` firmware project
        ├── makefile               Pulls `runtime/core/*.c` from repo via `VPATH`
        ├── overload/              RTOS constants (100 Hz tick)
        └── src/
            ├── main.cpp           NNDK entry, start PLC task
            ├── web_update.cpp     HTTP upload + reboot handlers
            └── embedded_mplc.c      Fallback embedded package (rebuild via `mplc_to_carray.py`)
```

Detailed NBEclipse and HTTP notes: [mod54415/nb_project/README.md](mod54415/nb_project/README.md)

Broader architecture: [docs/architecture.md](../../docs/architecture.md)

## Core runtime vs NetBurner code

| Layer | Location | Notes |
|-------|----------|--------|
| VM, scheduler, loader, I/O, stdlib | `runtime/` | Compiled into firmware via `nb_project/makefile` `VPATH` |
| ST → `.mplc` compiler | `compiler/` | Used by `mplc.exe` or **MPLC Studio** (embedded) |
| MOD54415 HAL, task, HTTP, EFFS | `platforms/netburner/mod54415/` | Platform-only |

**Firmware reflash** is required when the **VM or ABI** changes (e.g. new opcodes such as `MOD`). **HTTP upload** only replaces the `.mplc` program on flash filesystem—it does not update the runtime on the module.

## Endianness

`.mplc` packages are **little-endian**. The MOD54415 is **big-endian**. The NNDK build defines `MPLC_BIG_ENDIAN`; the loader and VM use `schema/mplc_endian.h` when reading package headers and multi-byte fields.

## Workflows

### MPLC Studio (typical edit / deploy loop)

![MPLC Studio](../../docs/MPLC_Studio.png)

1. Build Studio (once) or install the Windows setup package:
   ```powershell
   cd tools\mplc-studio
   .\scripts\build-mplc-studio.ps1 -QtDir "C:\Qt\6.8.3\msvc2022_64"
   # Optional installer:
   .\scripts\build-mplc-studio-installer.ps1 -QtDir "C:\Qt\6.8.3\msvc2022_64"
   ```
   Installer output: `tools/mplc-studio/dist/MPLCStudio-0.1.0-Setup.exe`

2. Open or write `.st` in Studio, set target IP / optional HTTP credentials.
3. **Compile** (`Ctrl+B`) → produces `<source>.mplc` beside the `.st` file (in-process compiler).
4. **Upload** → writes `mplc_startup.mplc` on the module EFFS-STD filesystem.
5. **Reboot** → module boots the uploaded package.
6. **Serial Monitor** tab — debug UART (115200, DTR/RTS); close PuTTY before connecting.

Studio includes an ST instruction toolbox, device discovery, success dialogs for compile/upload/reboot, and an embedded compiler (no `mplc.exe` on PATH). Full details: [tools/mplc-studio/README.md](../../tools/mplc-studio/README.md).

ST sources support `//`, `(* *)`, and `#` line comments.

### Command-line compile (PC)

```powershell
cmake --build build_local --config Release --target mplc
build_local\compiler\Release\mplc.exe compile program.st -o program.mplc
```

### Embed program in firmware (reflash required)

```powershell
python platforms\netburner\tools\mplc_to_carray.py program.mplc platforms\netburner\mod54415\nb_project\src\embedded_mplc.c
cd platforms\netburner\mod54415\nb_project
C:\nburn\SetEnv.bat
make
```

Deploy `obj\release\MPLC_MOD54415.bin` via NBEclipse (debugger).

### HTTP upload (no firmware rebuild)

| URL | Purpose |
|-----|---------|
| `http://<ip>/mplc_update.html` | Upload form |
| POST `mplc_upload.html` | Stores file as `mplc_startup.mplc` |
| POST `mplc_reboot.html` | Reboot module |

Boot order: `mplc_startup.mplc` on EFFS-STD → fallback `embedded_mplc.c` in firmware. See `MPLC_NB_PACKAGE_FILE_*` in `hal_netburner_config.h`.

## Build firmware (NNDK)

```bat
C:\nburn\SetEnv.bat
cd platforms\netburner\mod54415\nb_project
make
```

Open the same folder in **NBEclipse** (platform **MOD5441X**). Required defines (in makefile): `MPLC_NETBURNER_NNDK`, `MPLC_BIG_ENDIAN`.

## MOD-DEV-70 I/O

Digital channels **0–7** map to DIP switches **1–8** (ADC via `simple_ad.cpp` / `ReadSwitch()`) and LEDs **1–8** (GPIO, active-low). Use `%IX0.0` … `%IX0.7` and `%QX0.0` … `%QX0.7` in ST.

Example programs in `mod54415/`:

- `mod_dev70_dips_to_leds.st` — LED follows DIP when switch is on
- `mod_dev70_dip_blink_250ms.st` — blink while DIP on (~250 ms toggle at 10 ms scans)

Adjust polarity and debug logging in `hal_netburner_config.h` (`MPLC_NB_DIP_BIT_SET_IS_ON`, `MPLC_NB_IO_DEBUG`).

## Scan timing in ST

At the default **100 Hz** RTOS tick and **10 ms** PLC cycle, incrementing a counter once per scan:

| Wall time | Scans (ticks) |
|-----------|----------------|
| 10 ms | 1 |
| 250 ms | 25 |
| 2150 ms | 215 |

## Host validation (no hardware)

```powershell
cmake --build build_local --config Release --target mplc_netburner_test
.\build_local\platforms\netburner\mplc_netburner_test.exe
```

Links `mplc_runtime` + NetBurner HAL against the generic HAL for a PC smoke test (`MPLC_NETBURNER_HOST_TEST`).

## Task model

```
NNDK main (src/main.cpp)
  └── mplc_netburner_plc_start_task()
        └── MplcTask (uC/OS, priority MPLC_NB_TASK_PRIORITY)
              └── mplc_netburner_run_one_cycle()  [~10 ms period]
```

Keep the PLC task at higher priority (lower uC/OS number) than HTTP and networking if scan jitter matters (`hal_netburner_config.h`).

## Configuration checklist

| File | What to edit |
|------|----------------|
| `hal_netburner_config.h` | Scan period, task priority, file-boot path, DIP polarity, IO debug |
| `hal_netburner.cpp` | Pin tables if not using MOD-DEV-70 |
| `nb_project/overload/.../constants-overload.h` | RTOS `TICKS_PER_SECOND` (default 100) |
