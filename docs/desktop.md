# Native CHOS Desktop and applications

The optical ISO now starts [dedicated CHOS Setup](setup-environment.md) after the
kernel self-tests. Press L or F3 at its welcome page to start Live Desktop. An
installed disk boots the regular desktop directly. Setup and the desktop run on
the same original kernel, software renderer and native EL0 application environment.
No host UI framework or firmware boot service runs either environment.

Click a desktop icon, choose Start → Applications, or press Meta then 1–7:
Explorer, Notepad, Terminal, Control Panel, Task Manager, About, Setup. Alt-Tab
cycles visible windows. Drag a title to move; drag the lower-right corner to
resize. Title controls minimize, maximize/restore and close; task buttons restore.
The maximized title now shows the restore glyph. Start uses pointer release,
hover and arrows/Enter; it confirms logoff/restart/shutdown. Logoff closes apps and
opens About; it is not an authenticated session. Search/Help/Run currently open
Explorer/About/Terminal, without separate search/help/run services.

**Live ISO files are RAM backed and lost on restart/shutdown. Installed sessions
mount persistent CHFS.** About, app status and the live desktop identify the
storage mode. Inventory and rejected/cancelled setup do not write disks. Only
an explicitly confirmed native installation can replace an eligible target.

- **Explorer:** real directory navigation, Up, new folder, copy, move/rename and
  confirmed deletion. Shared toolbar controls require a selection for file
  actions. Select a file then C/M/D; N creates a folder, U goes up and R refreshes.
  Up/Down and the scrollbar operate on actual directory contents.
- **Notepad:** ASCII text up to 4095 bytes, insertion, backspace, cursor movement
  and scrolling. Ctrl-O opens, Ctrl-S saves, Ctrl-A opens Save As, Ctrl-N starts
  a new document. A native File menu invokes the same operations. Shared modal
  fields/buttons support pointer and keyboard. Unsaved close requires confirmation.
  Whole-file CHFS replacement retains the old file until commit; failed final
  flushes have uncertain old/new durability, so full rollback is not promised.
- **Terminal:** help, dir, cd, pwd, mkdir, copy, move, del, cat, cls, systeminfo,
  run, reboot and shutdown through original APIs. Paths may be absolute or relative;
  no quoting/pipelines/scripting. Copy/cat cap files at 4095 bytes. Power commands
  confirm and use the ACPI FADT's PSCI conduit; unsupported platforms return errors.
- **Control Panel:** Appearance, Clock and System tabs apply theme, clock offset/
  seconds and computer name. Save/Ctrl-S persists on an installed mounted volume;
  live Apply is explicitly volatile. Invalid settings do not write. It reports
  actual display dimensions; graphical mode/layout selection is unfinished.
- **Task Manager:** real PIDs, app IDs, scheduled timer samples, managed/allocated
  conventional RAM, context switches and paint counters. Timer samples are not
  CPU percentage; memory figures exclude firmware/unmanaged reservations.
- **About:** actual ARM64/development build, managed RAM, computer/volume status
  and native input swatch. Installed sessions have no permanent desktop overlay.
- **Setup:** dedicated full-screen blue selection/confirmation and Royale
  configuration/progress/completion. Launching it hides desktop chrome and routes
  input exclusively to Setup. Exiting safely restores existing apps. An active
  writer cannot be exited/cancelled; no password or repair features are simulated.

Shared controls provide release/cancel behavior, disabled/hover/focus states,
Tab/Shift-Tab navigation, bounded text fields and basic menus. Ctrl-U clears fields.
Input is basic US ASCII with Shift/Ctrl/Alt and navigation; no Caps Lock/IME/
clipboard. Small ordinary windows can still clip toolbar controls; keyboard
shortcuts remain available. Rich text, full radio-group arrow semantics, file
picker browsing, accessibility and menu outside-dismiss need further work.

Royale retains original blue wallpaper, icons, taskbar and two-column Start menu,
with cached anti-aliased Noto text under SIL OFL. No Microsoft assets/fonts/code
are used. Supported GOP surfaces are bounded between 640×480 and 1920×1200, chosen
before ExitBootServices. No runtime mode switch, GPU acceleration, general alpha
compositor or wallpaper-file loader exists. Setup is exercised at 640×480; desktop
mode and small-window limitations are not claims of complete responsive layout.

Built-ins share one static embedded AArch64 ELF selected by role; each launch has
private EL0 mappings/state, owned window, handles and memory. Eight process slots
and 128 drawing commands per window bound the implementation. There is no general
package/executable loader, login authentication or per-user filesystem authority.

See [configuration](configuration.md), [toolkit](gui-toolkit.md),
[dedicated Setup](setup-environment.md) and [current status](status.md).
Historical visual results remain in [M6C](milestone-6c.md) and
[M6D widgets/settings](milestone-6d.md).
