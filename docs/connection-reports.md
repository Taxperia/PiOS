# First-boot connection reports

An RFB/WebSocket close code 1006 only indicates an abnormal transport closure.
It does not establish that setup completed or identify a compositor/capture error.

During first boot, an authenticated setup owner can select **Desktop controls →
Download connection report** after a failed desktop connection. The browser saves
`pios-connection-report.json` locally. The report includes the most recent setup
capture failure, fixed service state properties, input-device count and at most
60 journal records, capped to 20,000 bytes of journal text. Collection times out
after five seconds per command and never follows the journal. Lines mentioning
passwords, tokens, cookies, authorization or secrets are redacted. Provisioning
files, credentials, user files and environment variables are not read.

The HTTPS gateway requires an active setup login. The private root broker also
checks the setup token, expiry and first-boot state before reading anything.
Normal desktop logins and revoked/completed setup tokens cannot retrieve reports.
No new TCP listener or SSH account is created. The report is downloaded only
when the owner selects the button; it is not automatically uploaded.

The original local alpha.4 image from 2026-11-14 predates this report feature.
For that exact image, the physical-card helper prepares a one-shot local update:

```powershell
python scripts/fidan-repair-connection --boot E:\
```

The helper verifies BOOT markers and the original boot command, saves its backup,
then stages four files and fixed target paths. On the next Pi boot, installed
file hashes must match the supported alpha.4 image or the staged update. Only
`broker.py`, `gateway.py`, `index.html` and `ui.js` are updated. Account records,
setup history and access preferences are untouched. Normal boot is restored and
the Pi reboots after success. Failed validation changes no target file and
restores normal boot; installation failure rolls back changed files. Results are
saved to `fidan-connection-repair/result.json` on BOOT. This is a local maintenance
update, not a signed release, and does not itself resolve an unknown VNC failure.

Validation covers setup-only authorization, token revocation, redaction, bounded
commands and IPC replies, plus card-update integrity and state preservation.
The four base hashes were checked against the actual alpha.4 root filesystem;
card-update tests also ran using ARM64 Python under QEMU. Physical deployment
and the connection error diagnosis are recorded separately.

## Alpha.4 capture-launch correction

The first physical report showed a running setup desktop and a capture failure
`PermissionError: [Errno 1] Operation not permitted`, before the capture unit
existed. The same credential-switch operation failed in a local transient unit
with the broker's actual sandbox properties. Having PID 1 create the fixed user
helper succeeded under those properties. The broker now uses `systemd-run`
with the authenticated account's UID and fixed helper arguments to prepare the
output; it does not use a local `setuid`/`setgroups` child. No sandbox protections,
SSH settings or authentication checks are loosened.

For a card already carrying the first report update:

```powershell
python scripts/fidan-repair-connection --boot E:\ --capture-fix
```

This uses a separate `fidan-capture-repair` directory, keeps the previous repair
and boot backups, and accepts the known installed report broker hash. Fresh
alpha.4 cards can use the default helper, which now includes the correction.
New reports identify the launcher as `systemd-user-unit-v2` and include bounded
source-file/line metadata for capture errors, without local variables or tokens.
The original alpha.4 raw image remains unchanged.
