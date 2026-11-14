# Installation readiness

The latest local image is `0.1.0-alpha.4`, built on 2026-11-14:
`D:\PiOS\pios-0.1.0-alpha.4-20261114\fidan-0.1.0-alpha.4-pi5-arm64.img`.
It is 4,194,304,000 bytes, SHA-256
`d9ac4117bfc5ff553cf7c206679854fee72f1884d0b67739798b0e265ca10853`.
All 13 matching ARM64/all packages are installed. Browser access now streams
the real Labwc desktop and native first-boot window. The greeter's QML backend
and greetd authentication/session handoff were corrected and tested.

All 23 native CTest groups passed. Installed ARM64 shell, first-boot and greeter
applications passed QEMU user-space smoke checks without QML runtime errors.
Both embedded filesystems match their standalone copies; FAT/ext4 checks and
51 offline checks passed. The final audit caught a missing initramfs after a
resumed build; the installed kernel's own `update-initramfs` generated it, its
storage-binder contents were checked, and the updated image was reverified.
The 892,885,485-byte compressed image decompresses to the raw image's exact
length and SHA-256. Adjacent verification, package and build reports record
these checks. This alpha is unsigned and has not been booted on a physical
Pi 5. The earlier image's device follow-up below does not qualify this one.

The 2026-11-11 additions provide a separate per-account welcome wizard at the
first desktop login, SSH settings/host-key generation, Windows card preparation
and an optional RDP maintenance session. See [the installation guide](pi5-kurulum.md)
and [the scoped security review](security-review-2026-11-11.md).
The older validation record below is historical; it does not qualify the new
ARM64 hardware paths.

Validation on 2026-11-11: the Debian 13 / AMD64 WSL build and all 21 CTest
groups passed. The new welcome flow also passed Turkish dark/light screenshots,
900×650 and 600×500 layouts and saved-progress checks. The shell module loaded
without runtime warnings; QML lint returned success with context/type warnings.
Windows provisioning and staged install/package mappings passed.

The local image build subsequently completed on AMD64 Debian WSL with ARM64
cross compilation and QEMU. All 13 ARM64/all Debian packages were rebuilt and
installed into the Raspberry Pi OS/Debian Trixie root filesystem. The raw image
is `D:\PiOS\pios-0.1.0-alpha.3-20261111\fidan-0.1.0-alpha.3-pi5-arm64.img`,
4,236,247,040 bytes, SHA-256
`03b7ea07c289810286e150ac90656ad3da70cf3417e874edea6ad5675c5c8a6b`.
The adjacent `verification.json` records partition layout, matching embedded
boot/root filesystem hashes and 35 passing offline checks. FAT and ext4 checks
passed; Pi 5 firmware/kernel/initramfs files are present. All ten installed
Fidan executables are AArch64 binaries. The installed desktop rendered under
QEMU in offscreen mock mode without runtime warnings; this does not exercise
the actual graphical login or hardware. After the final locale fix, the ten
native first-boot tests and nine image-tool tests passed.

The compressed `release/fidan-0.1.0-alpha.3-pi5-arm64.img.zst` is 880,622,819
bytes. A streaming decompression reproduced the verified raw image's exact
byte count and SHA-256. `compressed-verification.json` records this check;
`SHA256SUMS` includes both raw and compressed artifacts.

The image has no factory user/password, cloned machine ID or SSH host keys.
First boot and one-time root partition expansion are enabled; SSH and browser
desktop access require opt-in. TTY1 is reserved for the graphical setup service.
The local checksum is not a release signature. Physical boot had not been
tested at the end of the image build; the follow-up below records device testing.
See [the AMD64 image build guide](image-build-amd64.md) to reproduce the build.

Physical follow-up on 2026-11-12: the owner's Pi booted on Ethernet and served
the first-boot HTTPS portal. Privacy exposed a Qt/dbus-python signature conflict;
the [one-shot setup repair](setup-web-repair.md) was applied to that card without
changing account/setup state. Both repaired public files matched source hashes,
and the owner confirmed that the privacy step reached System updates. The
original local image is unchanged and predates this fix and the source Wi-Fi
dependency correction. Full hardware/desktop acceptance remains pending.

Implemented in source:

- Async native wizard, also available through the HTTPS desktop stream:
  Turkish/English, keyboard/time zone,
  password confirmation, Back, Wi-Fi scan/manual SSID/offline path and saved
  stage checkpoints. Saved JSON is private and contains no passwords.
  The selected keyboard becomes the desktop default after setup; the running
  setup keyboard-test field uses the compositor's current layout.
- The normal greeter is conditional on successful setup. Account and login
  service checks happen before atomically committing completion. A failed
  account-password step can be retried after restarting the setup service.
- First-boot signed PiOS update check/download/install, with explicit offline
  skip. The privileged transient update unit is checked before finishing so
  a daemon restart cannot cause setup to reboot during an active update.
- Before updates, `dpkg-repack` checkpoints the installed PiOS package set.
  Settings can restore that checkpoint or repair an interrupted dpkg transaction.
  This is package recovery; it is not a full disk snapshot or Debian dependency
  rollback. Recovery/install can still require repository access for dependencies.
  Failed-install retries retain the original baseline; at most two checkpoint
  directories are kept after creating a new checkpoint.
- Files supports transfer cancellation and up to 20 undo groups per process.
  Partial copies are cleaned up; completed items can be undone. Undo refuses
  name conflicts or replaced items and puts copied/created items in Trash.
  Cross-filesystem moves enter a noncancelable source-removal phase only after
  the full copy succeeds. Permanent deletion has no undo.
- Déjà Dup is included in the desktop package/image and opened from Settings →
  System → Backup. The owner must choose a destination and schedule and verify
  a restore. No backup has been created automatically.
- One-time card provisioning, native setup over local HTTPS/noVNC, opt-in PAM desktop,
  pinned image build plan, raw image verifier and hardware report collector.

Historical validation (2026-10-18):

Validation on 2026-10-18: the WSL/Debian source build and all 19 CTest groups
passed, including 26 updater checks, 12 remote provisioning/HTTPS/Unix socket
checks and 5 image-tool checks. Turkish setup screenshots were inspected at
1920×1080; the wizard also passed 1280×720, 3840×2160 and 2× scale runs.
`all_qmllint` exited successfully with existing context/type warnings; upstream
v2.8.0 metadata lint accepted the image layer. Shell hook syntax, portal JavaScript
syntax and repository metadata checks passed. These checks do not run real APT
installs, create host accounts or reconfigure the development machine's network.

At that time no Raspberry Pi was connected and the Windows host had insufficient
free space for image construction. The 2026-11-11 build above resolves image
generation; flashing and physical boot remain pending.
Linux source/UI tests and temporary HTTPS/Unix socket tests do not establish
Wi-Fi AP behavior, HDMI/audio/Bluetooth support, real PAM login, or compositor
capture on physical Pi hardware. Complete the acceptance checklist before
calling this a hardware-tested OS.

```sh
# On the Pi, from its graphical user session:
python3 scripts/test-pi-hardware --output pi5-evidence.json
# Optional, intentionally applies CPU load for ten minutes:
python3 scripts/test-pi-hardware --stress --output pi5-load-evidence.json
```

The collector reports unavailable probes and leaves functional tests marked
`not-performed`; it never infers full acceptance from service state. Running it
on a non-Pi returns exit 2. Existing evidence files are never overwritten.
