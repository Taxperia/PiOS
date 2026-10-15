# Fidan Raspberry Pi image

The image definition targets `rpi-image-gen` v2.8.0 and the Raspberry Pi OS
Trixie repositories. Build Debian packages first, then run:

```sh
./scripts/build-debs
RPI_IMAGE_GEN_VERSION=v2.8.0 ./scripts/build-image
```

`build-image` copies the locally built packages into `image/packages/`, invokes
the pinned upstream image generator with `-S image`, and produces a compressed
Pi 5 image with a FAT boot partition and growable ext4 root partition.

The first boot starts the local Fidan setup session. SSH, telemetry and the
regular greeter remain disabled until setup finishes.

