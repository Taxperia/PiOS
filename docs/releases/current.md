PiOS 0.1.0-alpha.1 is the first public engineering alpha of the Fidan desktop.

- A customizable Wayland desktop with a dock, launcher, notifications and quick settings.
- Redesigned settings, display configuration and personalization.
- File manager, photo viewer, task manager and real APT/Flatpak application management.
- Signed updates delivered through GitHub Releases. Installed PiOS packages are checked and updated automatically every six hours; installation never forces a reboot or restarts the active desktop.

The assets include Debian 13 packages for **amd64** and **arm64**, a signed package manifest, SHA-256 checksums and the public release key. This release does **not** contain a bootable Raspberry Pi image; physical Pi 5 acceptance testing is still pending.

Install the packages for your architecture on a Debian 13 system. For an existing development desktop, install the artwork, shell, store, photos, task-manager, session, lock and updater packages. The `fidan-desktop` meta-package also installs the login/first-boot components and is intended for a dedicated desktop system.

Automatic updates cover installed PiOS packages. Debian base-system updates and third-party APT/Flatpak applications retain their own update mechanisms.

Release trust fingerprint: `1085 5324 032F 285A FB73 CCD4 C41F F807 5817 D3B6`.
