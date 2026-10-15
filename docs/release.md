# Releases and automatic updates

PiOS publishes Debian 13 packages from `Taxperia/PiOS`. GitHub Actions builds
and tests native `amd64` and `arm64` packages on GitHub-hosted runners. A
release is published only after both builds finish and its manifest and
SHA-256 checksums are signed. There is no dependency on a self-hosted runner
for package releases.

## Publish a version

Update `config/product.json`, the Debian changelog, release notes in
`docs/releases/current.md`, and the versioned image package paths together.
The tag must be `v` followed by the product version; package versions map
`0.1.0-alpha.1` to `0.1.0~alpha1-1`. Push the tag to run `.github/workflows/release.yml`.
The workflow can also be dispatched manually at the intended commit. It
creates a draft, uploads all signed assets, then publishes the complete release.
It refuses to overwrite a published release. Alpha, beta and RC tags are
prereleases; stable versions are normal releases.

`scripts/build-debs` builds packages without copying development VHDs or
build directories. `scripts/prepare-release` checks package versions and
creates `release/release.json`. `scripts/sign-update-release` signs all assets
with the `FIDAN_RELEASE_GPG_KEY` Actions secret. The secret contains only the
signing subkey. The primary certification key remains on the maintainer's
machine and must be backed up privately. Never commit secret keys.

Only the public trust key is tracked in `data/keys/fidan-release-keyring.gpg`.
Its primary fingerprint is
`10855324032F285AFB73CCD4C41FF8075817D3B6`. The updater package installs it at
`/etc/fidan/release-keyring.gpg`; it never downloads a new trust key from a
release before verification. Rotating this trust root requires an explicitly
trusted package update or manual administration.

## Installed desktop updates

Installing `fidan-updater` enables `fidan-auto-update.timer`. The system
service checks GitHub shortly after boot and every six hours, with a randomized
delay. A missed calendar run is picked up after the next boot. Offline failures
are logged in the system journal and the next scheduled run retries.

The updater follows the configured alpha/beta/RC/stable channel, chooses the
highest compatible version rather than relying on publication order, downloads
the signed manifest and verifies its GPG signature with the installed trust key.
It validates repository, version, filenames, architecture, SHA-256 checksums and
the Debian package's own metadata. The privileged installer snapshots metadata
and packages into a private staging directory and verifies them again. It
refuses rollbacks and uses APT with `--no-remove` to update installed PiOS
packages. New login/first-boot components are not installed into an existing
development session merely because they occur in a release.

An update does not reboot or restart the active desktop. New desktop binaries
take effect at the next session. Base Debian updates and third-party apps use
their own update mechanisms; this service does not run a general APT upgrade.
The system needs Internet access and must be running for a scheduled check.

Settings → System shows update status and an authorized automatic-update toggle.
Manual CLI equivalents:

```sh
fidan-update check
fidan-update download
fidan-update install
sudo systemctl enable --now fidan-auto-update.timer
sudo systemctl disable --now fidan-auto-update.timer
journalctl -u fidan-auto-update.service
```

A checkout installed only with `cmake --install` should first be migrated to
the released `.deb` packages; APT tracks which PiOS components are installed.

## Raspberry Pi images

Package releases are not bootable images. Building and publishing a Pi 5 image
still requires `scripts/build-image`, the pinned image generator and the
physical hardware acceptance checklist. Do not describe package-only alpha
releases as hardware-qualified OS images.
