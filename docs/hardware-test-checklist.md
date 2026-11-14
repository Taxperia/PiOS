# Raspberry Pi 5 alpha acceptance checklist

Record the board revision, RAM size, power supply, storage model and display for
every run. The release candidate must pass on a 4 GB Pi 5 with a fast A2 microSD.

No physical Pi was available for the current changes; these checks remain open.
Run `python3 scripts/test-pi-hardware --output pi5-evidence.json` from the Pi's
desktop session to collect read-only evidence. Optional `--stress` applies CPU
load for ten minutes. Unavailable probes and manual functional checks remain
explicitly unverified; a JSON report alone is not an acceptance pass.

## Image and first boot

- Verify SHA-256 and detached signature before flashing.
- Boot from a clean card; confirm root filesystem expansion.
- Boot two separately flashed cards and compare `/etc/machine-id` and SSH host
  fingerprints; both identities must differ. SSH must wait for host-key generation.
- After account setup and the first authenticated login, complete the desktop
  welcome flow locally and through the browser desktop. Check language/keyboard
  on the next login, date format, theme, accent, panel position/design and resume.
  Finish must suppress automatic reopening for that account; Settings must reopen it.
- If testing optional RDP, follow [the masked installation and SSH tunnel](rdp.md),
  confirm no non-loopback 3389 listener, reject root/nonmember logins and measure
  idle/active RAM. Check that disconnection reclaims the session after 60 seconds.
- Complete Turkish and English first-boot flows separately.
- Confirm that no default account, password, SSH listener or telemetry endpoint exists.
- Confirm that the regular greeter starts only after first boot completes.
- Boot once with no HDMI display; complete setup from `https://fidan.local`
  over Ethernet using the one-time setup password.
- Repeat without Ethernet through the temporary `Fidan-Setup-*` WPA2 network;
  confirm that the access point and provisioning API disappear after setup.

## Hardware

- Ethernet, 2.4/5 GHz Wi-Fi and Bluetooth pairing.
- HDMI at 720p, 1080p and 4K/2×; HDMI audio and USB audio.
- USB keyboard, mouse and storage; unplug/replug each device.
- Temperature and throttling indicators under a 10-minute CPU load.
- Clean restart and shutdown; ten cold boots without filesystem repair.

## Desktop and security

- Panel, dock, launcher, quick settings, notifications and three workspaces.
- Lock using the panel and Super+L; try Alt+Tab, compositor shortcuts, monitor
  reconnect, shell restart and malformed PAM input while locked.
- Wrong passwords must never create a session. Correct authentication must
  return to the same session.
- Package update, interrupted update recovery and package rollback from cached
  `.deb` files.
- Use the browser desktop with no display attached; verify keyboard, pointer,
  clipboard, reconnect and session lock behavior.
- Confirm that raw VNC is unreachable from another host and that disabling
  remote desktop removes the HTTPS desktop endpoint.

## Performance targets

- Login screen within 30 seconds on subsequent A2 microSD boots.
- Under 900 MiB total idle memory and under 3% average idle CPU.
- 60 FPS target for 1080p shell transitions; capture frame timing when missed.

