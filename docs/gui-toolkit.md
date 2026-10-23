# Native CHOS GUI toolkit

M6D preserves the kernel's window manager, private ELF processes, command lists,
client clipping and software framebuffer. The original C library in
`userspace/libraries/gui/widgets.c` centralizes widget rendering and interaction;
it is linked into the native application ELF, not a browser or an external GUI
framework. Palette, spacing and control colors live in `include/royale.h`.

| Shared component | Native use |
|---|---|
| Button with hover/pressed/disabled/focus states | Explorer/Notepad toolbars, dialogs, Control Panel Save, Setup Back/Continue/Restart |
| Bounded text field with caret, Ctrl-U and Left/Right | Modal paths/confirmations, computer name and clock offset |
| Labels | Shared proportional body/bold/small fonts |
| Checkbox and radio button | Setup and Control Panel clock/theme configuration |
| Scrollbar with arrow steps and thumb dragging | Explorer's actual directory entries |
| Selectable list rows | Explorer real file/directory selection |
| Menu panel and keyboard/pointer items | Notepad File menu, actual file operations |
| Dialog panel and modal hit registry | Open/save/copy/move/delete and destructive confirmation |
| Progress bar | Actual native installation progress |
| Tabs | Control Panel Appearance/Clock/System |
| Toolbar | Explorer/Notepad, with shared button geometry |

Each application owns one bounded `ui_context` (64 registered controls). It supplies
a draw-command callback. Each paint rebuilds the hit registry using stable control
IDs. Application data and actions remain in applications; the library owns colors,
shapes, focus, hit testing, text editing and activation. No paint-time allocation
or unbounded widget tree is used. Old syscall numbers and draw operations remain.

Buttons, checkboxes, radios, tabs and menu items activate on release inside the
pressed control; leaving it cancels activation. File-list selection occurs on press,
preserving existing Explorer behavior and allowing immediate keyboard operations.
Tab/Shift-Tab traverses enabled controls; Enter/Space activates; menu Up/Down moves
focus. Disabled controls never activate. Text fields are ASCII and bounded; IME,
clipboard, accessibility infrastructure and richer editing remain future work.

Modal fields bind input when the dialog opens, before its first paint, and clear
that binding when it closes. This prevents queued keys from mixing legacy append
editing with a stale caret. Layout changes force a paint before further event
draining. Normal events are drained in batches of up to 32 before repainting.
The nonblocking EVENT flag makes this possible without busy polling at idle.

The window manager adds client pointer/release events, coalesces only consecutive
pointer events of the same type, captures release outside the window and preserves
release/close through a full event queue. Per-window queues hold 128 events. They
remain bounded; unlimited input floods are not guaranteed lossless. Independent input queues have no shared timestamp: each poll batch delivers
pointer movement/button changes before keyboard input, preserving order within
each stream. This makes click-then-type target the selected control. Virtio devices
have 32 descriptors; automated bursts respect that hardware capacity.

The approved wallpaper, fonts, icons, two-column menu and blue taskbar are retained.
Shell buttons now also cancel a press released outside. The All Applications menu
has a Back action and Left/Right navigation. Active/hovered task buttons and desktop
icon selection are distinct. The installed desktop has no permanent session overlay;
About reports volume/computer information. Search/Help still use the previously
documented Explorer/About shortcuts; dedicated services remain unfinished.

Shadows omit the opaque covered center. A portable pixel comparison checks the
visible result against the previous full-area algorithm. This reduces blending
work without allocating shadow caches or changing the approved appearance.

Validation combines host ASan/UBSan state-machine tests with real QEMU usage:
queued pre-paint text, press cancellation, disabled/focus/scroll bounds, multiple
native windows, actual menus/dialogs/files, a bounded fast input stream checked
in Notepad and reread by Terminal, and installation. Screenshots are lossless guest
framebuffer captures. See [M6D](milestone-6d.md).

Limitations: the library is small and immediate-mode, not a complete general GUI
application framework. Popup dismissal, full keyboard radio groups, file-picker
browsing, column sorting, clipboard and responsive toolbar overflow remain work.
At very small window sizes some toolbar controls clip; keyboard file shortcuts
remain available. No full translucent compositor or runtime display scaling exists.
