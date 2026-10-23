# Native CHOS Desktop and applications

The normal ISO starts the original desktop after about eight seconds of kernel
self-tests. No host operating system, JavaScript runtime, GUI toolkit, or firmware
boot service runs the desktop. A software backbuffer is copied to the GOP surface.
Rendering is event driven, with a one-second clock update; idle clients block.

Click a desktop icon, choose Start → Applications, or press Meta then 1–6:
Explorer, Notepad, Terminal, Control Panel, Task Manager, About. Alt-Tab cycles
visible windows. Drag a title to move; drag the lower-right corner to resize.
The title controls minimize, maximize/restore, and close; task buttons restore.
Start supports arrows/Enter and confirms logoff/restart/shutdown. Logoff closes
applications and opens a fresh About window; it is not an authentication boundary.
Search/Help/Run currently open Explorer/About/Terminal respectively, without a
separate search dialog, help browser or Run dialog.

**The desktop volume is RAM backed. All its files are lost on restart/shutdown.**
This is displayed by the desktop and apps. Ordinary images never format, mount or
write an attached disk. NVMe/CHFS persistence is independently tested only by the
separately gated storage-test image; it is not desktop persistence or installation.

- **Explorer:** directory navigation, Up, new folder, copy, move/rename, and
  confirmed deletion. Select a file, then C/M/D; N creates an absolute folder path;
  U goes up, R refreshes, Up/Down scroll the listing. Prompts take absolute paths.
- **Notepad:** plain ASCII text up to 4095 bytes, insertion, backspace, left/right
  cursor movement and scrolling. Ctrl-O opens; Ctrl-S saves; Ctrl-A opens Save As;
  Ctrl-N explicitly discards into a new document. Unsaved close requests confirm.
  Saves replace a complete file atomically using CHFS copy-on-write. An open/save
  failure retains the editor buffer; a rejected replacement retains the old file.
- **Terminal:** help, dir, cd, pwd, mkdir, copy, move, del, cat, cls, systeminfo,
  run, reboot and shutdown. Paths may be absolute or relative; no shell quoting,
  pipelines or scripting. Copy/cat are limited to 4095-byte files. Power commands
  confirm and use the ACPI FADT's PSCI conduit; unsupported firmware returns an error.
- **Control Panel:** changes the actual desktop color theme and reports the real
  framebuffer dimensions. Mode switching and persistent configuration are absent.
- **Task Manager:** lists real PIDs/app IDs, timer samples while each process was
  scheduled, allocated/managed conventional RAM, and context switches. Samples
  are not CPU percentage; numbers do not include every firmware/kernel reservation.
- **About:** actual architecture, development build, managed memory and an input
  swatch that exercises the same native event/drawing boundary as the apps.

Prompts accept Enter/Escape; Ctrl-U clears the input. Keyboard layout is basic US
with Shift/Ctrl/Alt, punctuation and navigation; no IME, Caps Lock or clipboard.
No font/artwork is copied from Microsoft. The compact original font is deliberately
simple. There is no acceleration, alpha compositing or image-file wallpaper loader.

All applications currently share one embedded static ELF, selected by entry role.
Each launch executes in its own private EL0 address space and owns its windows,
event queue, handles and memory. Kernel-controlled launch is bounded at eight live
processes. This is not an installed application/package loader.

## Native Setup and installed sessions

Meta-7 (or CHOS Setup) opens the original installer. Boot the ISO with a disposable
NVMe disk attached; accept the license, select the displayed serial/capacity,
enter a computer name and type `ERASE <exact serial>` only after reviewing the
warning. Wait for successful completion, detach the ISO and restart. Current
Setup configures the computer name only: user/password/timezone/layout/mode setup
are explicitly unfinished. There is no account security.

After a verified installed boot, the desktop uses the CHFS system partition and
file saves persist across cold boots. About shows the installed-volume status and
computer name. The earlier RAM-only description applies to live ISO/builder
sessions. A disk-booted Setup cannot erase its own boot medium. See milestone-6b.md
for tested geometry, safety/error limits and exact native-install evidence.
