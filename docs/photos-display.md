# Photos and display settings

Fidan Photos opens local PNG, JPEG, WebP, BMP, GIF and TIFF images. Other raster
formats depend on the installed Qt image plugins. Animated images show their
first frame. The viewer supports folder navigation, zoom, scrolling, fullscreen,
drag/drop, clipboard copy and setting the desktop wallpaper. Rotation changes
the view only; copying includes the current rotation and preserves the original
file. EXIF orientation is respected. Images over 64 megapixels are rejected.

Shortcuts: Ctrl+O opens a file, Left/Right browse the folder, +/− zoom, 0 fits,
1 shows actual size, R rotates, Ctrl+C copies, F11 toggles fullscreen and Escape
leaves fullscreen. Ctrl+wheel zooms. Fidan's default image associations use Photos;
explicit user associations still take precedence.

Settings → Display lists actual enabled outputs using `wlr-randr --json`. It
offers advertised resolutions/rates, output scaling, rotation and numbered
identification overlays. Apply performs a compositor dry run, then asks for
confirmation for 15 seconds. Revert, timeout, or closing the application restores
the previous output mode. Keep saves the selected output to
`~/.config/fidan/displays.json`; session startup restores only modes still
advertised by connected monitors. Monitor output scaling and shell interface
scaling are separate controls.

Brightness uses the existing system backlight service. Night Light uses a
per-output `wlsunset` process, with preferences in
`~/.config/fidan/night-light.json`. Fidan stops only its own recorded processes.
The nested WSLg display has neither a controllable backlight nor usable gamma
control; these controls are disabled there. Physical display behavior still
requires testing on the target hardware.

Validation on Debian 13 / WSLg: all four CTest groups pass. New tests cover local
image navigation, EXIF orientation, rotated clipboard copy without changing the
source, keyboard/drop actions, display validation, confirmation, timeout/close
rollback, saved-profile restoration and scrolling in a small settings window.
The running desktop was also checked for PNG opening from Files, fullscreen,
rotation, monitor identification and access to the lower settings panels.
