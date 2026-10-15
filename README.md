# PiOS

PiOS is the home of Project Fidan, the desktop and operating-system image
developed in this repository. Internal binaries and packages use the `fidan`
name.

Project Fidan is an open-source, lightweight Wayland desktop and reproducible
Raspberry Pi 5 operating-system image. The first alpha uses Raspberry Pi OS
Trixie packages, Labwc, Qt 6/QML and `rpi-image-gen`.

The repository currently contains:

- a Layer Shell desktop with panel, dock, app launcher, quick settings,
  notifications, an integrated software-center experience and live system
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
Fidan Photos provides local image viewing, zoom, rotation and folder navigation.
See [photos and display settings](docs/photos-display.md) for shortcuts, monitor
configuration and hardware-dependent controls.
See [settings and personalization](docs/settings-redesign.md) for the shared page
design, appearance options and system integration.

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
RPI_IMAGE_GEN_VERSION=v2.8.0 ./scripts/build-image
```

See [development setup](docs/development.md), [architecture](docs/architecture.md),
[headless management](docs/headless-management.md),
[store catalog management](docs/store-catalog.md),
[release process](docs/release.md), and the
[Pi 5 acceptance checklist](docs/hardware-test-checklist.md) before building an
image.

## Status

Version `0.1.0-alpha.1` is an engineering alpha. The core source and build
pipeline are implemented. The browser gateway and image-writing provisioning
helper described in the headless specification are still implementation work,
and a release image must pass the physical Pi 5 checklist before it is
presented as hardware-qualified.

License: GPL-3.0-or-later.
