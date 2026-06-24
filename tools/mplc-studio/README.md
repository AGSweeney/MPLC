# MPLC Studio

Qt6 desktop IDE for editing Structured Text, compiling in-process to `.mplc` packages, deploying to NetBurner MOD54415 targets, and monitoring the module debug UART.

![MPLC Studio screenshot](../../docs/MPLC_Studio.png)

## Features

### Editor

- Create, open, save, and save-as `.st` files
- Syntax highlighting for Structured Text
- **ST Instruction Toolbox** (right panel) — searchable snippets for structure, variables, control flow, logic/math, and comments; click to insert at the cursor
- Remembers last open file, toolbox expand/collapse, and window layout via `QSettings`

### Build and deploy

- **Compile** (`Ctrl+B`) — runs the embedded MPLC compiler (no external `mplc.exe`); writes `<source>.mplc` in the same folder as the ST file
- Success and failure dialogs for compile, upload, and reboot
- **Discover** — UDP broadcast on the LAN; lists NetBurner devices and probes for MPLC HTTP upload support
- **Upload** — POST to `http://<device_ip>/mplc_upload.html` (multipart field `plcfile` → `mplc_startup.mplc` on device flash)
- **Reboot** — POST to `http://<device_ip>/mplc_reboot.html`
- Target IP, optional HTTP user/password in the connection row above the editor

### Output and serial

- **Output** tab — compile logs, upload/reboot status, discovery messages; **Clear** on the left (`Ctrl+L` from the app menu)
- **Serial Monitor** tab — COM port picker, baud rate (default 115200), DTR/RTS (on by default, like PuTTY), connect/disconnect, live RX log, send line with optional CR+LF
- Close PuTTY (or any other app) before connecting Studio to the same COM port

## Requirements

- Qt **6.8+** with **Widgets**, **Network**, and **SerialPort** (MSVC 2022 64-bit kit recommended)
- Visual Studio 2022 build tools
- CMake 3.21+
- NetBurner MOD54415 firmware built from [`platforms/netburner/mod54415/nb_project`](../../platforms/netburner/mod54415/nb_project) (HTTP upload + PLC runtime)

Install **Qt SerialPort** via Qt Maintenance Tool if the Serial Monitor tab shows an install hint instead of port controls.

## Build

```powershell
cd tools\mplc-studio
.\scripts\build-mplc-studio.ps1 -QtDir "C:\Qt\6.8.3\msvc2022_64"
```

The script configures Release, builds `MPLCStudio.exe`, and runs `windeployqt` so Qt DLLs sit next to the executable.

Output: `tools/mplc-studio/build/app/MPLCStudio.exe`

## Windows installer (Inno Setup)

Requires [Inno Setup 6](https://jrsoftware.org/isinfo.php). The installer script uses `app/resources/icons/win32ui.ico` for the setup wizard; Start Menu and desktop shortcuts use the icon embedded in `MPLCStudio.exe`.

```powershell
cd tools\mplc-studio
.\scripts\build-mplc-studio-installer.ps1 -QtDir "C:\Qt\6.8.3\msvc2022_64"
```

This builds Studio, stages the windeployqt output (exe, Qt DLLs, plugins), and runs `installer\MPLCStudio.iss`.

Output: `tools/mplc-studio/dist/MPLCStudio-0.1.0-Setup.exe`

Use `-SkipStudioBuild` to reuse an existing `build/app` tree, or `-AppVersion` to change the installer version label.

From the MPLC repo root:

```bat
cmake -S . -B build_studio -DMPLC_BUILD_COMPILER=ON -DMPLC_BUILD_STUDIO=ON -DCMAKE_PREFIX_PATH=C:\Qt\6.8.3\msvc2022_64
cmake --build build_studio --config Release --target mplc_studio
```

## Typical workflow

1. Open or write ST in the editor (sample programs: [`platforms/netburner/mod54415/*.st`](../../platforms/netburner/mod54415/)).
2. Set **Target / IP** (and credentials if required).
3. **Compile** — confirm **Compile Complete**; package path is logged in the Output tab and status bar.
4. **Upload** — confirm **Upload Complete**.
5. **Reboot** — confirm **Reboot Requested**; watch **Serial Monitor** for boot text.

On successful file boot, serial output includes:

```text
MPLC: loaded package file 'mplc_startup.mplc'
```

Firmware reflash is only needed when the VM/runtime ABI changes—not for ST-only program updates.

## Keyboard shortcuts

| Action | Shortcut |
|--------|----------|
| New | `Ctrl+N` |
| Open | `Ctrl+O` |
| Save | `Ctrl+S` |
| Save As | `Ctrl+Shift+S` |
| Compile | `Ctrl+B` |
| Clear Output | `Ctrl+L` |
| Quit | `Ctrl+Q` |

## Related docs

- [NetBurner platform overview](../../platforms/netburner/README.md)
- [MOD54415 bring-up guide](../../docs/netburner-mod54415.md)
- [Architecture](../../docs/architecture.md)
- [Documentation index](../../docs/README.md)
