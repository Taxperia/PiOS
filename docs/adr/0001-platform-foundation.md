# ADR 0001: Raspberry Pi OS Trixie, Labwc and Qt Quick

Status: accepted

The first hardware target is Raspberry Pi 5, but the desktop should remain
portable. Raspberry Pi OS Trixie supplies the board kernel, firmware and device
integration. Labwc supplies a lightweight standards-based Wayland compositor.
Qt Quick provides the custom visual system and hardware-accelerated animation.

Project Fidan therefore owns the shell and session components while retaining
upstream responsibility for the kernel, compositor, authentication stack,
network stack and audio graph. Future boards add image/device layers rather
than forks of the desktop.

