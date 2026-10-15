# Settings redesign

All settings pages share translucent cards, symbolic icons, readable light/dark
palettes, keyboard-accessible buttons and a scroll area that works over nested
panels. The sidebar uses Turkish labels, with stable internal page identifiers
for existing desktop shortcuts. Pages load on demand.

Personalization includes eight bundled wallpapers (the original mountain photo
and seven original SVG illustrations), custom local files, a desktop preview,
light/dark/automatic appearance, preset or custom accent colors, interface
density, dock position and icon size. Automatic appearance uses light mode from
08:00 to 19:00. Dock options apply to the real Layer Shell surface; large docks
scale down to fit the available screen. The default remains on the left.
Interface density controls the existing desktop panel/menu scale, not monitor
resolution. Lock wallpaper is stored separately and used by the real gtklock
launcher; an empty selection follows the desktop wallpaper.

System and About read host, CPU, memory, OS, kernel and session information from
the running machine. Storage reads mounted filesystems using QStorageInfo and
distinguishes used space from space available to an ordinary user. WSL's large
virtual root capacity and the Windows host drive are shown separately.

The network page asynchronously queries NetworkManager through nmcli. It
supports scanning, connecting with a password or saved credentials,
disconnecting and showing actual IPv4 data. Static IP/DNS editing opens
NetworkManager's connection editor. Missing Wi-Fi/Bluetooth hardware is reported
as unavailable; no example network or Raspberry Pi specifications are displayed
as real system data.

Existing sound, power, keyboard, application-default and privacy controls remain
connected to their system services. Disk management, disk usage analysis and
backup configuration open installed native tools. GNOME Disks, Disk Usage
Analyzer and Déjà Dup are included in the store catalog for optional installation.
Automatic backups are not enabled by this redesign. Administrative actions keep
their normal system authorization.

Tests cover page loading in both themes, actual UI theme selection, small-window
scrolling, preference synchronization and validation, actual storage metadata,
escaped network names, connection failure/retry and lock-wallpaper generation.
Physical Wi-Fi/Bluetooth hardware still requires testing on a physical machine.
