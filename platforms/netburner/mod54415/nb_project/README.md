# MPLC MOD54415 — NBEclipse Project

Ready-to-build NNDK 3.x application for the **MOD5441X** platform (MOD54415 module). Pulls the **shared MPLC runtime** from the repo root (`runtime/core/*.c`)—rebuild firmware after VM/ABI changes, not only after ST edits.

For day-to-day ST work, prefer **[MPLC Studio](../../../../tools/mplc-studio/README.md)** (compile, HTTP upload, reboot, serial monitor). Use this project when changing firmware, HAL, or the embedded fallback program.

## Open in NBEclipse

1. Launch NBEclipse (`C:\nburn\NBEclipse\NBEclipse.exe`).
2. **File → Open Projects from File System…**
3. Select: `platforms/netburner/mod54415/nb_project`
4. Confirm platform **MOD5441X** (`nbeclipse-rules.json`).
5. Build **Release**. Output: `obj/release/MPLC_MOD54415.bin`.

Deploy `.bin` via the NBEclipse **debugger** (this makefile does not produce `.s19` / AutoUpdate artifacts).

## Command-line build

```bat
C:\nburn\SetEnv.bat
cd platforms\netburner\mod54415\nb_project
make
```

## Project layout

| Path | Role |
|------|------|
| `makefile` | NNDK build; `.bin` only; `VPATH` → `runtime/core`, `io`, `stdlib` |
| `overload/nbrtos/include/constants-overload.h` | `TICKS_PER_SECOND` = 100 (10 ms RTOS tick) |
| `nbeclipse-rules.json` | IDE platform, includes, defines |
| `src/main.cpp` | NNDK `init()`, HTTP registration, start PLC task |
| `src/web_update.cpp` | `mplc_update.html`, upload, reboot handlers |
| `src/embedded_mplc.c` | Embedded `.mplc` fallback when no file on filesystem |
| `../hal_netburner.cpp` | MOD-DEV-70 HAL (DIP ADC, LEDs, scan timing) |
| `../mplc_task.cpp` | uC/OS-II PLC cyclic task |
| `../mplc_plc_runtime.c` | Runtime glue |

## Preprocessor defines (makefile)

| Define | Purpose |
|--------|---------|
| `MPLC_NETBURNER_NNDK` | NNDK GPIO, `TimeTick`, `OSTimeDly` code paths |
| `MPLC_BIG_ENDIAN` | ColdFire byte order (required on MOD54415) |

## Replace the embedded PLC program

```bat
build_local\compiler\Release\mplc.exe compile myprogram.st -o myprogram.mplc
python ..\..\tools\mplc_to_carray.py myprogram.mplc src\embedded_mplc.c
make
```

Or compile in **MPLC Studio**, then run `mplc_to_carray.py` on the generated `.mplc`.

## Update program without changing embedded firmware

Runtime boot order:

1. `mplc_startup.mplc` on EFFS-STD (`MPLC_NB_PACKAGE_FILE_PATH` in `hal_netburner_config.h`)
2. Embedded `src/embedded_mplc.c`

### MPLC Studio

Compile → **Upload** → **Reboot**. Success dialogs confirm each step. Use **Serial Monitor** (115200, DTR/RTS) for boot logs—close PuTTY first.

See [tools/mplc-studio/README.md](../../../../tools/mplc-studio/README.md).

### HTTP

| Page | Action |
|------|--------|
| `http://<device_ip>/mplc_update.html` | Upload `.mplc` |
| POST `mplc_upload.html` | Saves `mplc_startup.mplc` |
| POST `mplc_reboot.html` | Reboot |

Serial on success: `MPLC: loaded package file 'mplc_startup.mplc'`.

Notes:

- Default max upload size: 512 KB (`MPLC_NB_PACKAGE_FILE_MAX_BYTES`).
- Delete/rename `mplc_startup.mplc` on the filesystem to fall back to embedded program.
- Uploads use **EFFS-STD** (1 MB reserved at top of parallel flash via `COMPCODEFLAGS` in makefile).
- Upload errors **-10/-11** usually mean EFFS not mounted—reflash firmware built from this project.

## Scan rate

| Setting | Value |
|---------|--------|
| RTOS tick | 100 Hz (`constants-overload.h`) |
| PLC cycle | 10 ms (`MPLC_NB_DEFAULT_CYCLE_US` = 10000) |

In ST, one counter increment per scan ≈ 10 ms (e.g. 25 scans ≈ 250 ms).

## Task priority

PLC task priority defaults to **10** (`MPLC_NB_TASK_PRIORITY`). Use a **lower numeric** uC/OS priority than web/network tasks if scan jitter must be minimized.

## Pin map (MOD-DEV-70)

Channels **0–7** → DIP **1–8** (ADC) and LED **1–8** (GPIO, active-low). See `hal_netburner_config.h` and `hal_netburner.cpp`.

Quick hardware test:

```bat
cd platforms\netburner\mod54415\nb_project
..\..\..\..\build_local\compiler\Release\mplc.exe compile ..\mod_dev70_dips_to_leds.st -o mod_dev70_dips_to_leds.mplc
python ..\..\tools\mplc_to_carray.py mod_dev70_dips_to_leds.mplc src\embedded_mplc.c
make
```

Use `%IXn.0` / `%QXn.0` where `n` is MPLC channel 0..7. Flip DIP switches—the matching LEDs should follow.

Other samples: `../mod_dev70_dip_blink_250ms.st`, `../mod_dev70_dip_blink_25ms.st`.

## Serial console

Typical boot (115200 8N1 on the module debug UART):

```
Application: MPLC_MOD54415
MPLC MOD54415 runtime starting...
MPLC: upload endpoint http://<device_ip>/mplc_update.html
MPLC: PLC task started (priority 10)
MPLC: runtime started
```

With file boot: `MPLC: loaded package file 'mplc_startup.mplc'`.

Monitor from **MPLC Studio → Serial Monitor** or PuTTY on the NetBurner development kit COM port.

## Related docs

- [NetBurner platform overview](../README.md)
- [MOD54415 bring-up guide](../../../../docs/netburner-mod54415.md)
- [MPLC Studio](../../../../tools/mplc-studio/README.md)
