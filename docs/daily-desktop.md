# Daily desktop usage

The left dock remains the desktop's taskbar. These changes add file and
application management without moving it to the bottom of the screen.

## Desktop selection and dragging

- Drag on empty desktop space with the left button to draw a selection box.
  Ctrl-click toggles icons; Shift-click selects a range; Ctrl+A selects all
  desktop entries after clicking the desktop. Escape or a plain empty click
  clears the selection. Sorting and refresh preserve selection by file path.
- Drag an icon or the selected group onto a desktop folder to move the actual
  files or application shortcuts. Hold Ctrl while dragging to copy instead.
  Dragging a selected group keeps the whole group together.
- Files can also be dropped from another file manager onto the desktop or a
  desktop folder. Existing names, recursive folder moves and remote URLs are
  rejected; transfers reuse the Files background operation service.
- Desktop icons stay fixed while selecting and dragging; the background does
  not pan or flick. The icon area uses the available desktop width and respects
  the dock position and interface scale.

## Files

- Click to select, Ctrl-click to toggle an item, Shift-click to select a range.
  Ctrl+A selects the current folder's visible items. Arrow keys navigate;
  Shift+arrow extends the selection. Escape clears it.
- Ctrl+C copies, Ctrl+X cuts, Ctrl+V pastes. The toolbar and context menu provide
  the same actions. Clipboard URLs use the KDE and GNOME cut markers for
  interoperability. Desktop background Paste uses the same operation backend.
- Drag files onto a folder or into another Files window. The default action is
  copy; a move action from the drag source is also supported. Only local file
  URLs are accepted.
- Folder trees, hidden entries and symbolic links are copied. Relative link
  targets remain relative. Folder links are never followed recursively.
- Copies and moves run off the UI thread. Existing names are never overwritten;
  conflicts and partial failures appear in the error area. A failed copy keeps
  its original. A partial destination may remain, as stated in the error.
- Delete moves selected items to Trash. Shift+Delete asks before permanently
  deleting them. Trash has a Restore selected action; restoration never
  overwrites a current file. Missing original parent folders are reported.
- Alt+Enter opens Properties. F2 renames. Ctrl+H toggles hidden files; the view
  menu sorts by name, size or modification time. External folder changes refresh
  the listing automatically.
- Ctrl+L focuses the address; Enter navigates and returns keyboard focus to the
  files. Ctrl+F searches the current folder. Alt+Left/Right navigate history;
  Alt+Up goes to the parent, F5 refreshes, Ctrl+N creates a file and Ctrl+Shift+N
  creates a folder.

There is no automatic overwrite/merge, transfer cancellation or undo for
permanent deletion. Keep a Files window open until its active operation ends.
Trash restoration currently covers the user's home Trash, not separate
per-volume Trash directories on removable drives.

## Application management

The Store checks installed packages in the background. Installed graphical
applications have Open and Remove buttons; non-graphical packages such as GPIO
Tools do not claim to have an application window. Remove asks for confirmation
and retains personal application data. Package operations keep the Store open
until they finish.

The operation panel displays live output and download percentages when the
package service reports them. It uses an activity indicator otherwise. Failure
details remain selectable and a Retry action repeats the failed operation.
Successful installation/removal is checked against the actual package database
before a success message is shown. Updates are limited to catalog applications.

APT requires a working PolicyKit authentication session and an administrator
password. On this WSL development profile, `fidan` was created without a password;
the user can set it interactively from PowerShell:

```powershell
wsl -d Debian -u root -- passwd fidan
```

Per-user Flatpak operations work without that administrator password. A physical
Pi login session's authentication flow still needs hardware acceptance testing.

## Appearance and defaults

Files, Store and Task Manager share a light/dark palette and refresh preferences
while open. Theme icons switch between Papirus and Papirus-Dark. Settings and the
shell use the same appearance preference. Third-party applications retain their
own supported themes.

Settings → Applications can choose installed handlers for the web, text, PNG,
MP4 and MP3. Browser changes apply to both HTTP and HTTPS. Files respects the
user's MIME default; FeatherPad is a fallback when no text handler is registered.

## Desktop shortcuts

| Shortcut | Action |
| --- | --- |
| Win+Space | Application launcher |
| Win+E | Files |
| Win+I | Settings |
| Ctrl+Shift+Escape | Task Manager |
| Ctrl+Alt+T | Terminal |
| Alt+Tab / Alt+Shift+Tab | Switch windows |
| Alt+F4 | Close window |
| Print Screen | Save screenshot and copy image to clipboard |
| Win+Shift+S | Select screenshot region |

Screenshots are saved in Pictures/Screenshots. `grim`, `slurp` and `wl-clipboard`
are included in development setup and image dependencies. Cancelling a region
selection removes the empty temporary image.

Notifications expose application-provided action buttons. Replaced notifications
update their existing history entry. Expiration, dismissal and application close
signals use their respective reasons; actions cannot be invoked after closing.

## WSLg and X11 applications

The development Labwc/wlroots runtime builds with Xwayland. WSLg's read-only
`/tmp/.X11-unix` mount cannot hold the nested compositor's sockets. Launching the
development script through WSL root creates a private mount namespace, opens a
PAM login session and drops to `fidan` before starting any desktop application.
This lets the PolicyKit authentication agent register with the session.
The original WSLg mount is
unchanged. On normal Debian/Pi systems the usual writable X11 socket directory
does not require this WSL-specific setup.
## Application and file icons

PiOS applications use their own embedded artwork in launchers and window identities.
The desktop and Files share file-type icons for text, PDF, images, audio, video,
archives, source code and office documents. Application shortcuts inside folders
keep the icon from their `.desktop` entry. Icons with absolute paths and Flatpak
exported icons are supported; newly installed icons are read when the view refreshes.

