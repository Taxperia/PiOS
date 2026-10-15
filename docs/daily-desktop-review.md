# Daily desktop acceptance review — 2026-10-15

Implemented the requested file management, appearance, Store reliability and
daily-use changes while retaining the left dock.

## Automated verification

`ctest --preset dev --output-on-failure`: both test executables passed.
QtTest totals are 34 core and 13 Files UI checks, including initialization and
cleanup. Repository structure validation and `git diff --check` passed.

Regression coverage includes directory trees and hidden files, preserving
relative symbolic links, collision protection, copy/move clipboard operations,
rejecting self-copy and remote drops, non-overwriting Trash restoration,
clipboard access without keyboard focus, multiple selection with keyboard copy,
rejecting FIFOs instead of waiting on them,
properties, actual incoming QML drop events, notification replacement timers and
actions, MIME defaults, failed downloads, unverified installation and retry.

## Actual desktop checks

- Copied two files between real folders using Ctrl+A/C/L/V; compared contents.
- Copied a separate review fixture from Files to the desktop through the desktop
  context menu. This exposed and fixed a Layer Shell clipboard focus issue.
- Deleted that fixture to real Trash, restored it through the Restore selected
  button, compared contents, and removed only that fixture through the permanent
  deletion confirmation. No user files were deleted by these tests.
- Changed dark/light appearance through Settings and checked Files, Store and
  Task Manager. Fixed missing copy glyphs and theme icons with poor contrast.
  Restored the original dark preference.
- Removed and reinstalled the existing review installation of VLC through Store,
  retaining application data. Verified the package state and opened VLC from
  the Store. Added Xwayland to the nested runtime when this exposed missing X11
  support; observed VLC's real window inside Labwc.
- Verified Win+E, Win+I and Ctrl+Shift+Escape. Print Screen produced a PNG in
  Pictures/Screenshots and an image/png clipboard selection. Win+Shift+S opened
  the region selector; Escape removed the empty temporary file.
- Sent a local action notification with `notify-send`, clicked Open Files,
  verified the client received `open`, and observed Files launch.
- Checked Settings' real installed MIME handlers, including GIMP and VLC.
- Opened the PolicyKit password prompt using `pkexec /usr/bin/true`, verified it
  identified `fidan`, then cancelled without entering credentials. The previous
  “Cannot create unix session” agent failure is resolved by the PAM launcher.
- Verified the WSLg X11 mount outside the desktop remains mode 777/read-only;
  only the desktop's private mount uses its own writable mode-1777 directory.

Screenshots are under `artifacts/desktop-review/usability-*.png`; build and test
logs are under `build/usability-*.log`.

## Remaining acceptance boundaries

- `fidan` still needs a user-chosen password for authenticated APT operations.
  The agent/prompt and cancellation are verified; no password was chosen or
  authentication bypass added. Per-user Flatpak install/remove/open was tested.
- Native incoming drag/drop is covered through QML event delivery; the complete
  physical pointer drag gesture between external applications was not exercised
  by the available live input tool. Region selection was opened and cancelled;
  an arbitrary drawn region was not captured in this review.
- Third-party applications retain their own themes. Removable-volume Trash,
  recursive conflict merging, transfer cancellation and full undo are outside
  the current Files implementation.
- Pi hardware networking/audio/backlight, real login/lock and power operations
  still require the existing physical-hardware acceptance checklist.
