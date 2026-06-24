MPLC MOD54415 — NBEclipse starter project (MOD5441X)

Open in NBEclipse
-----------------
1. File -> Import -> Existing Projects into Workspace (or open folder as project).
2. Select this directory: platforms/netburner/mod54415/nb_project
3. Set platform to MOD5441X if prompted (nbeclipse-rules.json restricts to MOD5441X).

Command-line build
------------------
  C:\nburn\SetEnv.bat
  cd platforms\netburner\mod54415\nb_project
  make

Output: obj\release\MPLC_MOD54415.bin (deploy via NBEclipse debugger).

MPLC Studio (typical ST workflow)
---------------------------------
  Build or install MPLC Studio (tools\mplc-studio\README.md).
  Compile -> Upload -> Reboot; use Serial Monitor for boot logs (115200).

Replace the embedded PLC program
--------------------------------
  mplc compile myprogram.st -o myprogram.mplc
  python platforms\netburner\tools\mplc_to_carray.py myprogram.mplc src\embedded_mplc.c
  make

See README.md in this folder for pin map, HTTP upload, and task priority notes.
