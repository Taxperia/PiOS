# Raspberry Pi 5 image tools

The image targets Raspberry Pi OS/Debian Trixie ARM64. The generator is pinned
to upstream v2.8.0, commit `262d4df5a9f9d4133370465399a7958a7c22cdc7`.
Prefer a native ARM64 Debian/Raspberry Pi OS build host with generator dependencies
installed and at least 20 GiB free on every destination filesystem. AMD64 Debian
can explicitly use `--allow-emulation` after ARM64 QEMU is enabled and the target
packages have been cross-built; see [the AMD64 build guide](../docs/image-build-amd64.md).
This is a
minimum prerequisite, not a maximum estimate. A WSL VHDX also needs enough free
space on its Windows host drive; its virtual free-space figure is insufficient.

```sh
./scripts/build-debs                 # Run on ARM64 to obtain ARM64 packages
python3 scripts/build-image          # JSON plan only; no downloads or build
python3 scripts/build-image --check  # Exit 1 for missing packages/tools/space
python3 scripts/build-image --build --work-dir /large-disk/fidan-work
```

`--packages-dir` and `--generator-dir` can also point to a larger build disk.
Only `--build` clones the pinned generator and copies verified local packages
to `image/packages/`. Package names, internal Debian metadata, architecture and
version must match `image/packages.list`. The build input hashes are saved in the
work directory. Upstream repositories can change, so this pin alone does not
promise byte-identical future builds.

The source customize hook installs the verified archives from `image/packages.list`
inside the target, enables both offered locales and prepares first boot. The cleanup hook
removes the upstream locked build account, checks that no human account remains,
locks root, switches networking to NetworkManager, disables SSH/normal login,
and enables the conditional setup/browser units. Hook executable modes are
restored by the builder when needed. Compression belongs to `deploy`, not
`image`, in the pinned generator configuration.

Cleanup removes SSH host keys, the system random seed and machine identity;
`fidan-ssh-hostkeys.service` regenerates missing SSH keys on the target device
before SSH can start. The first desktop login opens the per-account welcome
wizard after device/account setup. See [installation](../docs/pi5-kurulum.md).
The root filesystem uses standard 4 KiB ext4 blocks and has 60% installation
headroom in the image; a one-time,
image-only `fidan-grow-root.service` expands its last partition to the card's
capacity on the device. It refuses a different root, a non-ext4 filesystem or
a partition that is followed by another partition.

Before flashing, verify a generated uncompressed image without mounting it:

```sh
python3 scripts/verify-image /large-disk/fidan.img
# For an artifact covered by the project's signed checksum list:
python3 scripts/verify-image /large-disk/fidan.img --sums /large-disk/SHA256SUMS --keyring data/keys/fidan-release-keyring.gpg
```

The verifier checks partition bounds, overlap, FAT/ext superblocks, and optionally
the GPG signature and SHA-256. It does not prove firmware files or hardware boot.
For a compressed release, first verify the signed compressed artifact using
`gpgv` and `sha256sum`, then decompress on a disk with enough room and inspect
its raw layout. Do not label a raw decompressed file as covered by a checksum
entry that names only its compressed source.

See [release signing](../docs/release.md),
[headless setup](../docs/headless-management.md), and the
[hardware acceptance checklist](../docs/hardware-test-checklist.md).
The 2026-11-11 local build produced `fidan-0.1.0-alpha.3-pi5-arm64.img`
(4,236,247,040 bytes). Its partition layout, both embedded filesystem hashes,
35 offline defaults/firmware checks and an installed ARM64 desktop preview
passed. This local artifact is unsigned and physical Pi 5 acceptance remains
pending; see [the build record](../docs/installation-readiness.md).

The local 2026-11-11 artifact predates the 2026-11-12 web-privacy and Wi-Fi
dependency corrections. Use Ethernet and apply [the account-preserving setup
repair](../docs/setup-web-repair.md) when testing that existing file. Source is
corrected; the original local `.img` has not been rebuilt with those fixes.
