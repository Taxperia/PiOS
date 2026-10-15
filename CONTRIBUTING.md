# Contributing

Build changes with the `dev` CMake preset and run `ctest --preset dev` before
opening a change. UI changes must include before/after screenshots at 1280×720,
1920×1080 and 3840×2160 with 2× scale. Security-sensitive changes to the
greeter, lock screen, D-Bus policy, first-boot service or image hooks require a
second review and a physical-device test.

Use SPDX headers on source files. New visual assets must be SVG when practical
and must be recorded in `artwork/ASSET-LICENSES.md`.

