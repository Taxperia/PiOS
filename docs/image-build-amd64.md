# Build a Pi 5 image on AMD64 Debian

The preferred upstream environment is native ARM64 Debian/Raspberry Pi OS. The
pinned generator also has a QEMU path. PiOS requires an explicit
`--allow-emulation` option so a source-only host does not silently become an
image builder. This builds ARM64 user space; it does not simulate Pi 5 hardware
or establish that a physical device boots successfully.

Use a Linux filesystem for the source snapshot, package build, temporary root
filesystem and image assembly output. Creating ext4 images directly on WSL's
`/mnt/c` or `/mnt/d` can be very slow because it performs many small reads and
writes. Copy completed images to the Windows disk afterward. A chroot also
requires Linux ownership, modes and device nodes. Check Windows physical free
space as well as `df` inside WSL.

Required components include `crossbuild-essential-arm64`, target Qt 6 development
libraries, the native Qt tools, `qemu-user-static`, `binfmt-support`, `arch-test`,
`mmdebstrap`, `podman`, `uidmap`, `zip` and the pinned generator's `depends` list.
Enable `qemu-aarch64` using `update-binfmts`. Its registration must be enabled
and include the `F` flag so the interpreter remains available inside the target.
The generator builds checksum-pinned bdebstrap/genimage host tools itself;
a separately installed bdebstrap command is not a prerequisite.

## Target packages

In a fresh Linux source snapshot with the matching ARM64 Qt headers/libraries:

```sh
# Native Qt tools are selected through QT_HOST_PATH by the Debian rules.
sh scripts/build-arm64-debs
# Recompile changed native sources before resuming packaging:
cmake --build obj-aarch64-linux-gnu --parallel 2
sh scripts/build-arm64-debs --resume
```

The helper creates `debian/` from `packaging/debian`, uses the ARM64 compiler
and produces all thirteen packages in `dist/`. `nocheck` disables executing
target tests during cross compilation. Run the native source/UI suite separately,
then inspect the target ELF architecture and smoke-test the ARM64 binaries
under QEMU. These are distinct checks.
When restoring corrected files from a timestamp-normalized archive into an
existing build, refresh their modification times or use a clean build so Ninja
does not mistake the corrected sources for older inputs.

`liblayershellqtinterface-dev` is not coinstallable across architectures. To
preserve an existing AMD64 development setup, download the ARM64 development
and runtime archives with `apt-get download`, extract them with `dpkg-deb -x`
into a dedicated target prefix and set `CMAKE_PREFIX_PATH` to its `usr/`.
The ARM64 runtime library and other target Qt dependencies must still be
available for dependency generation and target execution. The helper uses
`dpkg-buildpackage -d` because this deliberate host-tools/target-prefix setup
does not satisfy Debian's automatic build-dependency architecture check;
successful CMake configuration, linking, shlibdeps and target smoke tests are
required before using the results.

## Image

Clone the pinned generator on Linux, with LF line endings. Its checkout must
be clean at `262d4df5a9f9d4133370465399a7958a7c22cdc7`.

```sh
sudo python3 scripts/build-image --check --allow-emulation \
  --generator-dir /linux-disk/rpi-image-gen \
  --work-dir /linux-disk/pios-work \
  --output-dir /large-output-disk/PiOS

sudo python3 scripts/build-image --build --allow-emulation --cache-packages \
  --generator-dir /linux-disk/rpi-image-gen \
  --work-dir /linux-disk/pios-work \
  --output-dir /large-output-disk/PiOS
```

The output directory holds the raw `.img` and intermediate filesystem images;
`release/` contains compressed deployment assets. The work directory records
package input hashes. Optional package caching permits retries without
redownloading every target archive. Local PiOS archives are installed by the
customize hook because bdebstrap does not expand environment/anchor expressions
in the package list.

If only image assembly fails, `--images-only` resumes from the completed target
filesystem. It requires the same package hashes and generator commit recorded
by the full build. Image layout settings can then be corrected without repeating
all package installation. The Pi 5 kernel uses 16 KiB memory pages; the image
deliberately uses 4 KiB ext4 blocks to avoid excessive allocation for small
desktop/icon files.

Run `scripts/verify-image` on the raw image, inspect its firmware/root filesystem
and generate SHA-256 sums. A checksum from a local build is not a maintainer
signature. Complete [physical acceptance](hardware-test-checklist.md) before
calling the image hardware-qualified.
