# Repair web setup on the 2026-11-11 Pi 5 image

This guide describes the legacy web setup repair. The current native desktop
setup requires matching `fidan-firstboot`, `fidan-session`, `fidan-shell` and
`fidan-remote` packages. The helper refuses to stage current native sources as
a web-only repair. Use the original repair checkout for that historical repair.

The privacy page can return HTTP 400 because Qt exports two `SetPrivacy` D-Bus
signatures for a default argument. dbus-python's introspection selects the
two-argument signature while the browser gateway supplies three arguments.
The corrected gateway passes the allowlisted wire signature explicitly.
A real Qt D-Bus/Python integration test reproduces the old failure and verifies
the fix. The HTML username pattern also escapes the hyphen for browsers using
Unicode-set regular expressions, and the UI explains the lowercase requirement.

For a device without working SSH, prepare a one-shot repair on its mounted FAT
BOOT partition from the matching source checkout. Close the browser, shut down
the Pi and connect its card to the Windows computer. Substitute its BOOT drive:

```powershell
python scripts/fidan-repair-setup --boot E:\
```

The helper checks the PiOS boot files and saves the exact original `cmdline.txt`
before adding a one-time `systemd.run` command. No setup password is needed for
this physical-card operation. The mounted card is not repartitioned or flashed.
It explicitly selects `systemd.unit=kernel-command-line.target` because this
image has a persistent `/etc/systemd/system/default.target` desktop override,
which takes precedence over the run generator's normal-priority default link.
Safely eject the card, insert it into the Pi and power on with Ethernet attached.
The repair installs `gateway.py`, `index.html`, `ui.js` and `style.css`, then reboots.
Refresh the browser with Ctrl+F5 and sign in with the existing one-time password.

The boot mechanism uses [systemd-run-generator](https://www.freedesktop.org/software/systemd/man/257/systemd-run-generator.html).
The repair validates payload SHA-256 values and accepts only the known image's
original file hashes or the already repaired versions. Original files are saved
under `/var/lib/fidan/repairs/setup-web-20261112/`. Accounts, password hashes,
saved setup progress and privacy choices are left untouched. A failed payload
check restores normal boot without changing installed files; installation errors
roll back files changed so far. This is a local maintenance repair, not a signed
PiOS update release, and does not install the missing Wi-Fi packages.

If the device does not return, reconnect the card to the computer and inspect
`fidan-repair/result.json`. It contains only success/file names or an exception
class, without passwords or provisioning data. The saved `cmdline.original`
can be copied back to `cmdline.txt` to restore normal boot if execution never
started. Preserve both files when reporting a failure. Do not overwrite the
original backup or prepare a second repair over an unfinished one.

Validation: 11 native first-boot tests, including the real D-Bus regression;
15 HTTPS/broker/remote tests; seven isolated repair tests on native and ARM64
Python; native and target systemd generator checks. On 2026-11-12 the physical
Pi served both repaired web files with matching SHA-256 values, and the owner
confirmed that Privacy → Continue reached System updates. Full desktop/hardware
acceptance remains separate.
