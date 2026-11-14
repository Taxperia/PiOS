# Local HTTPS setup and browser desktop

The `fidan-remote` package implements a local HTTPS portal with an unprivileged
gateway, a PAM/session broker and noVNC. Hardware qualification is still pending.
Neither SSH nor post-setup browser access is enabled by default.

## Prepare an individual card

Flash a verified Pi image using Raspberry Pi Imager, mount its FAT boot partition,
then run the helper on Linux:

```sh
python3 scripts/fidan-provision --boot /media/owner/bootfs --hostname fidan
```

The helper prompts twice for a unique 12–63 UTF-8-byte setup password. It writes
a salted scrypt verifier and WPA2 PSK, never the clear password, to
`fidan-setup.json`. Treat that file/card as a credential until setup completes;
the WPA2 PSK itself grants access to the temporary network. The helper refuses
to overwrite existing provisioning data and never writes a raw block device.
There is no factory password or embedded TLS private key.

Card preparation also works with Python 3 on Windows:
`python scripts/fidan-provision --boot E:\ --hostname fidan` (replace `E:` with the
mounted boot partition). The Pi-side services themselves require Linux.
Wait for `Prepared https://fidan.local` before removing the card. A rejected
password does not produce provisioning data. Windows installations without
the optional `py` launcher can use `python` directly.

The 2026-11-11 local image lacks `wpasupplicant` and `dnsmasq-base`; use Ethernet
for its first setup. After account setup, install those packages with APT before
using Wi-Fi or the temporary access point. Package/image dependencies have been
corrected in source for subsequent builds; the existing local image was not
rebuilt with this correction.

On boot, the root bootstrap service consumes and removes the FAT file, sets the
hostname and generates a unique self-signed TLS certificate. Connect Ethernet,
then visit `https://fidan.local` on the same LAN. If Ethernet/Wi-Fi is unavailable,
join the printed `Fidan-Setup-<suffix>` WPA2 network using your setup password and
visit `https://192.168.50.1`. AP mode depends on the physical Wi-Fi adapter and
regulatory environment and needs Pi testing.

Browsers warn about the self-signed certificate. Verify its SHA-256 fingerprint
against the device using a trusted local console before accepting it:

```sh
sudo openssl x509 -in /var/lib/fidan/tls/cert.pem -noout -sha256 -fingerprint
```

If you have no trusted way to compare the fingerprint, first-contact identity
is not independently verified. A changed certificate on a later connection
should be investigated rather than accepted automatically.

Sign in with the one-time password and complete language/keyboard/time zone,
account/password confirmation, network, SSH/browser preferences and update steps.
Progress survives a service/device restart; account and Wi-Fi passwords are
excluded from setup history. Wi-Fi may disconnect the temporary AP while joining
the selected network: reconnect your computer to that network and reopen the
hostname to resume. Completing setup removes the temporary profile and verifier,
enables the login service and schedules reboot. If reboot fails, restart manually.

## Browser desktop after setup

Choose browser access during setup or enable it in Settings → Privacy. An
administrator can also run `sudo fidan-remote configure on` or `off`. This is
separate from SSH. Sign in with the local username/password, authenticated by
PAM; the setup password no longer works. Sessions expire after 20 minutes.

The browser opens with a startup screen followed by sign-in. The one-time setup
password authenticates to a broker token restricted to the `fidan-setup` account.
The browser then streams the real Labwc desktop and shell from that account,
with the shell's device welcome wizard open on the normal wallpaper. A connected
monitor and the browser share the same setup session. There is no HTML installer
or simulated desktop. The shell saves setup through its privileged
D-Bus service; the web gateway cannot call setup operations.

Setup tokens cannot access a regular user's desktop and are revoked at setup
completion. After reboot, the local account password opens the user's own
desktop, if browser access was enabled. Setup choices are saved to that account,
so its welcome dialog does not repeat the completed setup. Existing accounts
without completed preferences still receive the personal desktop welcome.
The stream fills the browser viewport in both cases.

The old purple native installer remains in source and installed packages for
recovery. It is disabled by default; explicitly set `FIDAN_LEGACY_FIRSTBOOT_UI=1`
in the setup session environment to use it. The portal's former purple artwork
is retained behind `data-legacy-portal="true"` on the HTML body. Its old completion
page is also retained behind `data-legacy-completion-screen="true"`; normally,
setup completion returns to the local-account login after reboot.
The connected desktop has no browser controls overlay. A small recovery panel
appears only while disconnected; transient failures reconnect with bounded
backoff (five attempts). The browser's normal full-screen command, such as F11
on Windows, remains available. The stream fills the browser viewport.
noVNC scales the complete desktop to the browser while preserving the shared
display's resolution, including the native installer's position and size.

For local UI regression checks without a device, run
`python3 tests/serve_remote_ui.py` and open `http://localhost:8765/checks`.
This loopback-only fixture tests startup, login, setup transport, responsive sizing and
reconnection with a stub RFB client. It does not test PAM or actual VNC transport;
`python3 tests/test_remote.py` covers the gateway/broker boundaries separately.

For a real compositor and noVNC review on the Debian development host, build
`fidan-shell`, `fidan-firstboot` and `fidan-firstboot-tests`, then run
`sh scripts/verify-native-setup` and open `http://localhost:8766/` using
`fixture-password`. This temporary local test streams a real Wayland desktop
and native installer; setup operations use the isolated test service and never
create host accounts. Artifacts are saved in `artifacts/native-setup/`. Stop the
script after the review. Install all matching desktop packages on a device;
the legacy web-only repair cannot deploy this architecture change.

SSH can be changed after setup in Settings → Privacy, or using
`sudo fidan-ssh on` / `off`. Every card generates its own SSH host keys.
For Windows Remote Desktop, see the [optional RDP guide](rdp.md).

WayVNC 0.9 or newer uses a private raw RFB Unix socket in `/run/user/<uid>/`.
The same user's capture unit runs websockify between that socket and a second,
private WebSocket Unix socket. Both sockets use owner-only permissions; neither
process opens a TCP listener. WayVNC's `-w` and `-u` options cannot be combined.
The root broker authorizes each session and passes the connected socket FD
to the gateway. The gateway runs as `fidan-gateway` and exposes only HTTPS 443.
A monitor-free session defaults to 1600×900 at 30 Hz with 125% output scale;
capture is capped at 25 FPS. This reduces the pixel/refresh workload and makes
all application windows and text larger. Confirmed display profiles and physical
monitor settings are preserved. An accessible DRM render node is used for
headless rendering when available, with a software fallback. Actual Pi latency
and GPU behavior still require device verification. See the
[wlroots environment documentation](https://sources.debian.org/src/wlroots/0.19.3-1/docs/env_vars.md)
and [WayVNC frame-rate option](https://github.com/any1/wayvnc/blob/master/wayvnc.scd).

Visible, recently used browser desktops renew their authenticated session once
a minute. Idle sessions still expire after approximately twenty minutes; every
sign-in has an eight-hour maximum lifetime. Renewal requires the existing cookie,
CSRF token and live broker token. Logout, completed setup and disabled browser
access revoke renewal and existing streams. Renewal keeps the original stream
identity and does not require resending the password. Older running gateways do
not receive keepalive requests until restarted after the package update.

The portal rejects nonlocal clients, invalid Host/Origin, missing CSRF tokens,
oversized requests and expired/revoked sessions. Cookies are Secure/HttpOnly/
SameSite=Strict. Login attempts, workers and sessions are bounded. The services
do not log request bodies/passwords. No UPnP, port forwarding or cloud relay is
configured. Only enable it on a trusted local network.

```sh
journalctl -u fidan-remote-bootstrap -u fidan-gateway -u fidan-remote-broker
systemctl status fidan-firstbootd
```
