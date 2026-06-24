# NetBurner MOD54415 Bring-Up Guide

## Platform summary

| Item | Value |
|------|-------|
| Module | NetBurner MOD54415 |
| CPU | NXP MCF54415 @ 250 MHz (ColdFire, **big-endian**) |
| RAM | 64 MB DDR2 |
| Flash | 32 MB parallel flash |
| RTOS | uC/OS-II (NNDK 3.x), **100 Hz** tick (10 ms) |
| PLC scan | **10 ms** default (`MPLC_NB_DEFAULT_CYCLE_US`) |
| Carrier | MOD-DEV-70 (8 DIP + 8 LED) — HAL in `hal_netburner.cpp` |
| Toolchain | NBEclipse / GNU ColdFire (`make` in `nb_project/`) |

## Repository layout

```
platforms/netburner/
├── README.md
├── tools/mplc_to_carray.py
└── mod54415/
    ├── hal_netburner.cpp          MOD-DEV-70 HAL
    ├── hal_netburner_config.h     Pins, scan rate, upload paths
    ├── mplc_plc_runtime.c         Load package, run cycles
    ├── mplc_task.cpp              uC/OS-II PLC task
    ├── mod_dev70_*.st             Sample ST programs
    └── nb_project/                NNDK firmware (NBEclipse / make)
        ├── makefile               VPATH → runtime/core, io, stdlib
        └── src/
            ├── main.cpp
            ├── web_update.cpp     HTTP upload + reboot
            └── embedded_mplc.c    Fallback package
```

## Endianness

`.mplc` packages are **little-endian**. The MOD54415 is **big-endian**. The NNDK build defines `MPLC_BIG_ENDIAN`; loader and VM use `schema/mplc_endian.h`.

## Recommended workflow: MPLC Studio

For day-to-day ST development, use [MPLC Studio](../tools/mplc-studio/README.md):

1. Build Studio once (or install `dist/MPLCStudio-*-Setup.exe`).
2. Edit `.st`, set target IP.
3. **Compile** → `<source>.mplc` beside the ST file.
4. **Upload** → `mplc_startup.mplc` on module EFFS-STD.
5. **Reboot** → module runs uploaded program.
6. **Serial Monitor** — COM port at **115200**, DTR/RTS on (close PuTTY first).

Firmware reflash is **not** required for ST-only changes. Reflash when the VM/ABI or platform code changes.

Full platform notes: [platforms/netburner/README.md](../platforms/netburner/README.md)

Firmware project: [platforms/netburner/mod54415/nb_project/README.md](../platforms/netburner/mod54415/nb_project/README.md)

## HTTP endpoints (on flashed firmware)

| URL | Purpose |
|-----|---------|
| `http://<ip>/mplc_update.html` | Upload form |
| POST `/mplc_upload.html` | Multipart field `plcfile` → `mplc_startup.mplc` |
| POST `/mplc_reboot.html` | Reboot module |

Boot order: EFFS file `mplc_startup.mplc` → embedded `src/embedded_mplc.c`.

## Command-line compile (PC)

```powershell
cmake --build build_local --config Release --target mplc
build_local\compiler\Release\mplc.exe compile myprogram.st -o myprogram.mplc
```

## Embed program in firmware (reflash)

```powershell
python platforms\netburner\tools\mplc_to_carray.py myprogram.mplc platforms\netburner\mod54415\nb_project\src\embedded_mplc.c
cd platforms\netburner\mod54415\nb_project
C:\nburn\SetEnv.bat
make
```

Output: `obj/release/MPLC_MOD54415.bin` — deploy via NBEclipse **debugger** (not AutoUpdate; the project builds `.bin` only).

## Build firmware (NNDK)

```bat
C:\nburn\SetEnv.bat
cd platforms\netburner\mod54415\nb_project
make
```

Open the same folder in NBEclipse (platform **MOD5441X**).

## MOD-DEV-70 I/O in ST

Channels **0–7** map to DIP switches and LEDs **1–8**:

- Inputs: `%IX0.0` … `%IX0.7`
- Outputs: `%QX0.0` … `%QX0.7`

Sample programs in `platforms/netburner/mod54415/`:

- `mod_dev70_dips_to_leds.st` — mirror DIP to LED
- `mod_dev70_dip_blink_250ms.st` — blink while DIP on
- `mod_dev70_dip_blink_25ms.st` — faster blink pattern

## Serial console

Typical boot (debug UART, 115200 8N1):

```text
Application: MPLC_MOD54415
MPLC MOD54415 runtime starting...
MPLC: upload endpoint http://<device_ip>/mplc_update.html
MPLC: PLC task started (priority 10)
MPLC: runtime started
MPLC: loaded package file 'mplc_startup.mplc'
```

Use MPLC Studio **Serial Monitor** or PuTTY on the NetBurner development kit COM port.

## Task model

```
NNDK UserMain (src/main.cpp)
  └── mplc_netburner_plc_start_task()
        └── MplcTask (uC/OS, priority 10)
              └── mplc_netburner_run_one_cycle()  [~10 ms]
```

Keep the PLC task at higher priority (lower uC/OS number) than HTTP/network tasks.

## Retain memory

The MOD5441X **User Params** flash sector (`0xC0020000`, 128 KB) is the recommended location for retain variables. Wire `mplc_hal_retain_read/write` in the NNDK build when retain support is needed.

## Host validation (no hardware)

```powershell
cmake --build build_local --config Release --target mplc_netburner_test
.\build_local\platforms\netburner\Release\mplc_netburner_test.exe
```

Links NetBurner HAL + runtime for a PC smoke test.
