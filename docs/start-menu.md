# Start menu

The Start menu uses a two-column layout: the account and application categories
on the left, and search, pinned applications, recommendations and recent files
on the right. It follows the desktop's dark/light theme, accent and interface
scale, and scales down to fit smaller displays.

- Open **All Apps** or a category to browse installed desktop applications.
  Search matches names, descriptions, desktop IDs and commands within that
  category. Matching recent files also appear below application search results.
- Right-click an application to pin/unpin it or add a desktop shortcut. Pins
  are stored separately from dock pins in `launcher/pinnedApps` in
  `~/.config/fidan/shell.conf`. The home view shows the first eight pins;
  **Pinned Apps** in the sidebar shows the complete list.
- **Recent Files** reads local, readable files from the standard
  `~/.local/share/recently-used.xbel` history (or `$XDG_DATA_HOME`). Only
  applications that record this history contribute entries. Private entries,
  missing files, remote URLs and desktop launchers are omitted. Up to twenty
  entries are shown, ordered by last visit/modification time.
- Recommendations open Settings, Fidan Store and Task Manager. Power provides
  session locking and confirmation before restart or shutdown.
- Type to search, use Down and the arrow keys to select an application, and
  Enter to launch it. Escape clears the search, then closes the menu. Tab moves
  through controls; the Menu key opens an application's context menu.

Run `ctest --preset dev -R fidan-launcher-ui-tests` for category, pin persistence,
recent-file, keyboard and layout checks. Set `FIDAN_LAUNCHER_CAPTURE_DIR` when
running `build/dev/tests/fidan-launcher-ui-tests renders` to save dark/light
screenshots at 1280×720, 1920×1080 and 3840×2160 with 2× interface scale.
