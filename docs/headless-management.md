# Headless management

Project Fidan must be installable and usable without connecting a monitor to
the Raspberry Pi. Headless management stays on the local network and does not
require a cloud account or an Internet connection.

## First boot

The image-writing helper asks the owner to choose a one-time setup password and
writes the provisioning data to the FAT boot partition. There is no shared or
factory-default password.

1. Connect the Pi to the router with Ethernet and power it on.
2. Open `https://fidan.local` on a computer on the same network.
3. Enter the one-time setup password and complete the existing first-boot
   wizard: language, keyboard, time zone, user account, Wi-Fi and privacy.
4. After successful setup, the provisioning endpoint deletes its secrets and
   disables itself.

If Ethernet is unavailable, the device creates a temporary WPA2 network named
`Fidan-Setup-<device suffix>`. The setup password protects this network and its
local portal. The access point is removed after the Pi joins the selected Wi-Fi
network.

## Browser desktop

After setup, `https://fidan.local` opens a browser-based remote desktop. The
gateway authenticates against the created local account through PAM. noVNC
provides the browser client, websockify carries the VNC connection, and WayVNC
captures a virtual Labwc output named `NOOP-fidan` when no physical output is
connected.

The raw VNC service listens only on loopback. The HTTPS gateway is the only
network-facing desktop endpoint. Remote desktop access can be disabled from
settings; SSH remains a separate, opt-in feature.

## Security boundaries

- No default human account, fixed password or reusable setup credential exists.
- The setup API is available only until the first-boot completion marker is
  committed.
- Device TLS material and the provisioning secret are generated per image.
- Passwords and Wi-Fi secrets are never written to logs.
- Virtual keyboard and pointer input terminate at the compositor session.
- The alpha provides no Internet relay, vendor cloud or automatic port
  forwarding.

