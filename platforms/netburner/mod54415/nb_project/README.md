# NBEclipse Project Setup (MOD54415)

Create a new **MOD5441X** application in NNDK 3.x and add the following source groups.

## MPLC runtime (from repo root)

```
schema/mplc_abi.h
schema/mplc_endian.h
runtime/core/loader.c
runtime/core/vm.c
runtime/core/scheduler.c
runtime/core/runtime.c
runtime/io/io.c
runtime/stdlib/stdlib.c
runtime/include/mplc/*.h
hal/include/mplc_hal.h
```

## MOD54415 platform (this directory's parent)

```
platforms/netburner/mod54415/hal_netburner.c
platforms/netburner/mod54415/hal_netburner_config.h
platforms/netburner/mod54415/mplc_plc_runtime.c
platforms/netburner/mod54415/mplc_task.c
platforms/netburner/mod54415/user_main.cpp
platforms/netburner/mod54415/embedded_mplc.c   ← generated from .mplc
```

## Preprocessor defines

| Define | Purpose |
|--------|---------|
| `MPLC_NETBURNER_NNDK` | Select NNDK GPIO/timer/RTOS code paths |
| `MPLC_BIG_ENDIAN` | ColdFire byte order; required on MOD54415 |
| `MPLC_NO_FPU` | Optional: soft-float if not using double |

## Include paths

Add to the NNDK project:

```
$(MPLC_ROOT)/schema
$(MPLC_ROOT)/hal/include
$(MPLC_ROOT)/runtime/include
$(MPLC_ROOT)/platforms/netburner/mod54415
```

## Embed a program

On your development PC:

```bash
mplc compile myprogram.st -o myprogram.mplc
python platforms/netburner/tools/mplc_to_carray.py myprogram.mplc embedded_mplc.c
```

Copy `embedded_mplc.c` into the NNDK project and rebuild.

## Task priority

The PLC task defaults to priority `10` (`hal_netburner_config.h`). Set it **higher** (lower numeric value in uC/OS) than web server and networking tasks if scan jitter must be minimized.

## Pin map

Edit `hal_netburner_config.h` and the `g_digital_in_map` / `g_digital_out_map` tables in `hal_netburner.c` to match your MOD-DEV-70 or custom carrier wiring.
