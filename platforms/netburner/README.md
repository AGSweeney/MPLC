# NetBurner MOD54415 Platform

MPLC port for the NetBurner MOD54415 (NXP MCF54415, NNDK 3.x, uC/OS-II).

## Directory layout

```
platforms/netburner/
├── README.md
├── CMakeLists.txt              Host-side library + smoke test (no NNDK required)
├── mod54415/
│   ├── hal_netburner.c         HAL implementation
│   ├── hal_netburner_config.h  GPIO/ADC channel → carrier pin map (edit this)
│   ├── mplc_hal_netburner.h    NetBurner-specific HAL helpers
│   ├── mplc_task.cpp           uC/OS PLC cyclic task
│   ├── user_main.cpp           NNDK application entry template
│   ├── embedded_stub.c         Empty .mplc blob for host builds
│   └── nb_project/
│       └── README.md           NBEclipse project setup steps
└── tools/
    └── mplc_to_carray.py       Embed compiled .mplc as a C array
```

Full bring-up guide: [docs/netburner-mod54415.md](../../docs/netburner-mod54415.md)

## Quick workflow

1. Compile on PC:
   ```bash
   mplc compile program.st -o program.mplc
   python platforms/netburner/tools/mplc_to_carray.py program.mplc mod54415/embedded_mplc.c
   ```

2. Add MPLC runtime sources + `mod54415/` files to your NNDK MOD5441X project.

3. Define `MPLC_NETBURNER_NNDK` and `MPLC_BIG_ENDIAN` in the NNDK project.

4. Edit `hal_netburner_config.h` for your MOD-DEV-70 (or custom carrier) pin map.

5. Build and deploy with NBEclipse / AutoUpdate.
