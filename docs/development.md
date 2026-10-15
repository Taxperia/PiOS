# Development setup

## Windows host

Install WSL2 with Debian, then install the Linux build dependencies inside the
distribution. WSLg can run the real shell in a nested Labwc compositor. Final
hardware acceptance still runs on a physical Raspberry Pi 5.

```powershell
wsl --install --distribution Debian
```

Use a normal Debian account for setup and the desktop. Electron applications
such as VS Code reject a root session. An imported distribution that only has
root needs a normal account first (the first command asks you to choose its
password):

```powershell
wsl -d Debian -u root -- adduser fidan
wsl -d Debian -u root -- usermod -aG sudo fidan
wsl --manage Debian --set-default-user fidan
```

Run the remaining commands as that account. On WSL, `run-nested-session` can
hand an existing root terminal over to `fidan`; set `FIDAN_DESKTOP_USER` if the
normal account has a different name. Development preferences are stored in that
account's `~/.config`, rather than shared in the repository's build directory.

```sh
./scripts/setup-debian-dev
./scripts/build-labwc-dev-runtime
cmake --preset dev
cmake --build --preset dev
./scripts/run-nested-session
ctest --preset dev
```

The nested session uses Layer Shell surfaces and starts real Wayland
applications. The dock opens QTerminal and PCManFM-Qt; the terminal has a real
PTY and tab support, and the file manager performs real filesystem operations.
Host hardware controls are enabled only when their system service and device are
available.

## Install real applications

`setup-debian-dev` installs APT/PolicyKit, Flatpak and the GTK desktop portal,
and configures Flathub for the user running the script. Run it as the same
Debian user who will start Fidan. The nested session starts the PolicyKit
authentication agent and exports the Flatpak application and icon directories.

Open **Fidan Store**, choose an application and click **Install**. Debian
packages use `pkexec apt-get` with administrator authentication; Flatpak apps
install into the current user's account. Source preparation runs asynchronously,
and installation failures appear in the store's details panel. Reopen the application
launcher after installing to refresh its entries; no desktop restart is needed.

You can also install applications directly from QTerminal:

```sh
sudo apt-get update
sudo apt-get install mousepad
flatpak install --user flathub org.gimp.GIMP
```

The Fidan portal configuration selects the GTK backend for sandboxed file
dialogs and other supported desktop services. See the
[Flatpak desktop integration conventions](https://docs.flatpak.org/en/latest/conventions.html)
and [portal configuration documentation](https://flatpak.github.io/xdg-desktop-portal/docs/configuration-file.html).

To open the terminal and file manager automatically during a smoke test:

```sh
FIDAN_SESSION_TEST_APPS=1 ./scripts/run-nested-session
```

To run keyboard/window regressions and capture every settings page with
isolated preferences, then exercise the real nested session:

```sh
ctest --preset dev --output-on-failure
./scripts/verify-desktop-ui
./scripts/test-nested-desktop
```

The visual review writes images and logs to `artifacts/desktop-review`.
The real-session test uses temporary sample files and checks application
windows, launcher search/Enter, minimize/restore, Alt+F4 and QTerminal's PTY.
See the [desktop review record](desktop-review.md) for results and remaining
hardware checks. On WSL, also check free space on the Windows drive holding
the distribution: the virtual Linux capacity does not represent host free space.

## Visual-only preview

```sh
FIDAN_MOCK_SYSTEM=1 ./build/dev/src/shell/fidan-shell --preview --mock
```

This mode is a resizable design gallery. It varies CPU, RAM and temperature
within deterministic bounds, but it does not authenticate, create an account,
change hardware state, restart or shut down the computer. Use the nested Labwc
session when testing behavior.

## Image host

The supported image build path is native Debian/Raspberry Pi OS arm64. The
self-hosted GitHub Actions runner labeled `fidan-image-builder` executes the
same scripts. Local non-arm64 builds may work through QEMU but are not the
release qualification path.

