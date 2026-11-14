# PiOS

PiOS is the home of Project Fidan, the desktop and operating-system image
developed in this repository. Internal binaries and packages use the `fidan`
name.

Project Fidan is an open-source, lightweight Wayland desktop and reproducible
Raspberry Pi 5 operating-system image. The first alpha uses Raspberry Pi OS
Trixie packages, Labwc, Qt 6/QML and `rpi-image-gen`.

The repository currently contains:

- a Layer Shell desktop with panel, dock, app launcher, quick settings,
  notifications, a category-based Start menu with persistent pinned apps and
  recent files, an integrated software-center experience and live system
  metrics;
- a single-window preview mode with deterministic mock hardware data;
- a `greetd` login UI and a themed `gtklock` PAM/session-lock flow;
- a live `/proc` based task manager, a Flatpak/APT application store, desktop
  application shortcuts and signed GitHub release update checks;
- a privilege-separated first-boot wizard for locale, account and Wi-Fi setup;
- a specified monitorless setup and local browser-desktop flow at
  `fidan.local`;
- Debian source packaging, a signed APT repository generator, a Pi 5
  `rpi-image-gen` layer, CI, release signing hooks and hardware acceptance
  documentation;
- an original 4K wallpaper with prompt and license provenance.

## Real desktop development session

On Debian 13, as a normal user with Qt 6.8 development packages installed:

```sh
./scripts/setup-debian-dev
./scripts/build-labwc-dev-runtime
cmake --preset dev
cmake --build --preset dev
./scripts/run-nested-session
ctest --preset dev
```

This starts a nested Labwc compositor. The shell uses real Layer Shell surfaces;
QTerminal provides a real PTY and tabs. Fidan Files supports multiple selection,
background copy/move, clipboard and drag/drop, properties, hidden files, sorting,
Trash and restoration. Name conflicts preserve existing files and report an
error. PCManFM-Qt remains an optional fallback. Desktop controls call the same
system services used by the Pi image. Available
hardware still depends on the host; for example, WSL normally exposes no Bluetooth
adapter or display backlight.

Real application installation is available from **Fidan Store** using Debian
APT packages and per-user Flathub apps. The development setup installs the
authorization agent and desktop portal, and configures Flathub. Newly installed
apps appear when the launcher is reopened. See
[application installation](docs/development.md#install-real-applications).
See [daily desktop usage](docs/daily-desktop.md) for shortcuts and file management.
See [the Start menu](docs/start-menu.md) for categories, pins and recent files.
PiOS includes a native PDF viewer, calculator, clock and local event calendar.
See [built-in applications](docs/builtin-apps.md) and the
[VirtualBox fresh-install guide (Turkish)](docs/virtualbox.md).
Fidan Photos provides local image viewing, zoom, rotation and folder navigation.
See [photos and display settings](docs/photos-display.md) for shortcuts, monitor
configuration and hardware-dependent controls.
See [settings and personalization](docs/settings-redesign.md) for the shared page
design, appearance options and system integration.

The first real desktop login opens a resumable welcome wizard for per-user
language/date/keyboard, appearance, panel layout, privacy/access and updates.
See the [Pi 5 installation guide (Turkish)](docs/pi5-kurulum.md) for accounts,
Raspberry Pi Imager, Windows SSH and browser access. An
[optional RDP maintenance desktop](docs/rdp.md) is available through an SSH
tunnel; its dependencies are excluded from the standard image.

## GitHub updates

PiOS releases provide signed Debian 13 packages for `amd64` and `arm64`.
Installing `fidan-updater` enables automatic updates of installed PiOS packages
from [GitHub Releases](https://github.com/Taxperia/PiOS/releases) every six hours.
The updater verifies the pinned release key, signed manifest, package identity
and SHA-256 checksums before APT installs an update. It never forces a reboot
or restarts the active desktop. Settings → System provides a manual check and
an authorized automatic-update toggle. See [release maintenance](docs/release.md).

Third-party applications and the Debian base system retain their own updates.

On Windows WSLg, launch the nested session from PowerShell using:

```powershell
wsl -d Debian -u root --cd /mnt/c/path/to/fidan -- ./scripts/run-nested-session
```

Replace the path with your checkout's Linux path. The launcher prepares an
isolated X11 socket mount, opens a PAM session and immediately switches to
the normal `fidan` account. Both Wayland and X11 applications run as that user.

## Visual preview

```sh
FIDAN_MOCK_SYSTEM=1 ./build/dev/src/shell/fidan-shell --preview --mock
```

Visual preview is only a resizable QML design gallery. Its hardware values and
controls are simulated, it does not create accounts, and it does not authenticate,
restart or shut down the computer. Do not use it to validate operating-system
behavior.

## Debian packages and Pi image

```sh
./scripts/build-debs
python3 scripts/build-image          # Read-only plan; lists missing prerequisites
python3 scripts/build-image --check  # Nonzero exit until ready
# On an ARM64 build host with at least 20 GiB free:
python3 scripts/build-image --build
```

See [development setup](docs/development.md), [architecture](docs/architecture.md),
[headless management](docs/headless-management.md),
[store catalog management](docs/store-catalog.md),
[expanded settings and wallpaper setup](docs/settings-expanded.md),
[release process](docs/release.md), and the
[Pi 5 acceptance checklist](docs/hardware-test-checklist.md) before building an
image.

## Status

Version `0.1.0-alpha.4` is an engineering alpha. On 2026-11-14, all 13 ARM64/all
packages and a 4,194,304,000-byte Raspberry Pi 5 image were built under AMD64
WSL with QEMU. Browser setup streams the real Wayland desktop and its native
setup window. The image includes the kernel, matching initramfs, graphical
login and resumable first boot. All 23 native CTest groups, 51 offline image
checks, FAT/ext4 checks and installed ARM64 application smoke tests passed.
The compressed image decompresses to the verified raw image's exact checksum.
The provisioning helper prepares an already mounted boot partition; it does
not flash disks. Physical acceptance of this new image and release signing
remain pending. See
[installation and image readiness](docs/installation-readiness.md).

License: GPL-3.0-or-later.
