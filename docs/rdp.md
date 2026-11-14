# Optional Windows Remote Desktop access

The normal PiOS image uses the Wayland desktop and authenticated HTTPS/noVNC.
The optional `fidan-rdp` helper configures an xrdp/Xorg/Openbox maintenance
desktop for Windows `mstsc`. It opens a file manager and terminal; it does not
mirror the physical Wayland session or load a second full desktop environment.
Use the browser portal when you want the PiOS panel and desktop itself.

No xrdp/Openbox packages are added to the standard image and no RDP process
runs by default. Actual Pi 5 RAM use is unmeasured. An active RDP session adds
Xorg, Openbox, xrdp and the applications it runs. Do not promise a fixed memory
figure without measuring it on the target device.

## Install and opt in

After device setup, enable SSH and install the optional components on the Pi.
Mask the two units **before** installing packages, so Debian's package scripts
cannot start the stock listener on all network interfaces:

```sh
sudo systemctl mask --now xrdp.service xrdp-sesman.service
sudo apt-get update
sudo apt-get install --no-install-recommends xrdp xorgxrdp openbox qterminal pcmanfm-qt
sudo fidan-rdp on YOUR_LOCAL_USERNAME
```

The helper stops the services, preserves the original configuration as
`*.before-fidan`, generates a private device TLS key, requires TLS and binds
only `127.0.0.1:3389`. Only users in `fidan-rdp-users` can sign in; the command
adds the selected regular local account to this group. Root login is disabled.
It permits one session, disconnects idle sessions after 20 minutes and closes
disconnected sessions after 60 seconds. Save work before disconnecting.
The helper then unmasks and enables the services. Repeated use is supported.

## Connect from Windows

Keep this PowerShell command running, replacing the local account and host:

```powershell
ssh -N -o ExitOnForwardFailure=yes -L 127.0.0.1:13389:127.0.0.1:3389 YOUR_LOCAL_USERNAME@fidan.local
```

In another PowerShell window:

```powershell
mstsc /v:127.0.0.1:13389
```

Choose **Xorg** in xrdp if offered, then use your local username/password.
The SSH tunnel authenticates the Pi. Its local TLS certificate is self-signed;
do not add a public firewall rule or forward port 3389 on your router.

```sh
fidan-rdp status
sudo fidan-rdp off
```

Disabling stops both services and prevents boot startup. Package files remain
installed. For memory acceptance, compare `free -h` before installation, while
services are idle, with an active session and after disconnection. Inspect
`ps -eo pid,user,rss,comm --sort=-rss` as well; systemd service memory alone
may omit processes moved to a user session cgroup. Record device RAM capacity,
screen resolution and opened apps with the results.

The config transformation and activation order are covered by isolated tests.
Real ARM64 xrdp/PAM/Xorg login, Windows input/clipboard and the RAM figures
still need Pi 5 acceptance. This is an optional experimental path.

Configuration semantics: [Debian xrdp.ini](https://manpages.debian.org/trixie/xrdp/xrdp.ini.5.en.html)
and [Debian sesman.ini](https://manpages.debian.org/trixie/xrdp/sesman.ini.5.en.html).
