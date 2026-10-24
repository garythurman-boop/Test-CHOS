# CHOS roadmap after M7I

M7I is the preserved input baseline. The user confirms actual Fusion/M1 normal
ISO boot, keyboard, mouse/trackpad, Setup and Live Desktop. This establishes that
input works in the reported configuration, not universal device support.

M8A corrects inconsistent NVMe serial-padding handling, distinguishes every
recheck failure and uses disk/session-bound ERASE-only confirmation. Target
persistent-installation correction still needs a Fusion retest.

M8B moves all disk preparation/copy/verification into blue Setup and resumes
persisted graphical configuration after a real disk-only reboot. M8C improves
cursor and desktop gestures/menus with saved layout. M8D exposes actual firmware
modes using reboot trials and confirmation. M8E retains Royale and refines native
menus/properties/application interactions. M8F integrates the complete prior
regression suite, failure tests, sanitizers, clean-source reproduction and a
versioned development ISO. See the evidence gates in [Milestone 8](milestone-8.md).

Before CHOS 0.1, independently verify the complete installation/configuration /
file-persistence/resolution sequence on Fusion, expand hardware evidence, fix
remaining defects and measure target memory/CPU use. Authentication, account
permissions, broader filesystem capacity/recovery, application associations,
USB hotplug re-enumeration and additional cursor variants remain future work.
Networking, SMP and accelerated graphics follow reliable boot/storage/input.
