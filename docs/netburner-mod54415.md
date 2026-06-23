# NetBurner MOD54415 Bring-Up Guide

## Platform summary

| Item | Value |
|------|-------|
| Module | NetBurner MOD54415 |
| CPU | NXP MCF54415 @ 250 MHz (ColdFire, **big-endian**) |
| RAM | 64 MB DDR2 |
| Flash | 32 MB parallel flash |
| RTOS | uC/OS-II (NNDK 3.x) |
| Toolchain | NBEclipse / GNU ColdFire (not CMake on-target) |

## Repository layout

```
platforms/netburner/
├── mod54415/           HAL, uC/OS task, NNDK entry template
├── tools/              .mplc → C array converter
└── CMakeLists.txt      Host-side HAL library + smoke test
```

## Endianness

`.mplc` packages are **little-endian**. The MOD54415 is **big-endian**. Define `MPLC_BIG_ENDIAN` when building for the module and ensure the loader/VM use `schema/mplc_endian.h` helpers for multi-byte fields.

## Build workflow

### 1. Compile IEC program on PC

```powershell
build\compiler\Release\mplc.exe compile myprogram.st -o myprogram.mplc
python platforms\netburner\tools\mplc_to_carray.py myprogram.mplc platforms\netburner\mod54415\embedded_mplc.c
```

### 2. Create NNDK project

Follow [platforms/netburner/mod54415/nb_project/README.md](../platforms/netburner/mod54415/nb_project/README.md).

### 3. Configure I/O

Edit `hal_netburner_config.h` and pin tables in `hal_netburner.c` for your carrier board.

### 4. Implement NNDK GPIO/ADC hooks

In `hal_netburner.c`, replace the `nb_read_digital_pin`, `nb_write_digital_pin`, and ADC/DAC stubs with your MOD-DEV-70 or custom board drivers.

### 5. Deploy

Build in NBEclipse and load via debugger or AutoUpdate.

## Retain memory

The MOD5441X **User Params** flash sector (`0xC0020000`, 128 KB) is the recommended location for retain variables. Wire `mplc_hal_retain_read/write` to that region in the NNDK build.

## Task model

```
UserMain (NNDK)
  └── mplc_netburner_plc_start_task()
        └── MplcTask (uC/OS, priority 10)
              └── mplc_runtime_run_cycle()  [every task period]
```

Keep networking and web UI in separate tasks at lower priority than the PLC scan task.

## Host validation (no hardware)

```powershell
cmake --build build --config Release --target mplc_netburner_test
```

This verifies the NetBurner platform tree links against the MPLC runtime on your PC.

## Next steps

1. Wire real GPIO on MOD-DEV-70 and run `sample.st` (`Input → Output`).
2. Add big-endian loader support in `runtime/core/loader.c`.
3. Persist retain data to User Params flash.
4. Optional: HTTP upload endpoint for `.mplc` field updates.
