# Unified desktop welcome

The default device setup now runs inside the shell on the ordinary desktop
wallpaper. Its eight steps cover welcome, region, account, network, appearance,
privacy/access, updates and completion. Settings are saved through the existing
restricted FirstBoot D-Bus service. The portal only transports the real desktop.

The selected theme, accent, dock, panel, date format, language, keyboard and
motion preference are copied to the new account by a fixed helper running as
that account. Completion refuses a failed profile write and remains retryable.
Passwords are excluded. A completed device setup also completes the new
account's welcome, avoiding a second setup window. Already-created accounts
retain their own settings and may finish their existing personal welcome.

The old standalone purple installer is retained and disabled by default.
`FIDAN_LEGACY_FIRSTBOOT_UI=1` selects it explicitly in the setup environment.
The old portal artwork and completion screen are also retained behind the HTML
flags documented in [headless management](headless-management.md).

## Local maintenance payload, 2026-11-14

`D:\PiOS\pios-unified-welcome-20261114\pios-unified-welcome.tar.gz` updates an
existing ARM64 PiOS 0.1.0-alpha.4 device. This is a local maintenance artifact,
not a published or signed release. The existing raw IMG has not been rebuilt.
The installer verifies every payload hash, preserves account and SSH settings,
and saves root-only rollback copies before replacing files. It updates the
temporary setup profile's cached autostart if present, leaving regular users'
preferences alone. Services switch to the new code after reboot.

From Windows PowerShell:

```powershell
scp "D:\PiOS\pios-unified-welcome-20261114\pios-unified-welcome.tar.gz" taxperia@fidan.local:~/
ssh -t taxperia@fidan.local "mkdir -p ~/pios-update && tar -xzf ~/pios-unified-welcome.tar.gz -C ~/pios-update && sudo python3 ~/pios-update/pios-unified-welcome/install.py && sudo reboot"
```

Enter the existing account password only in the terminal. After reboot reopen
`https://fidan.local` and sign in as `taxperia`. A failed install prevents the
chained reboot. To roll back, run the retained installer with `--rollback` and
the exact `/var/backups/pios-unified-welcome-...` path printed during install,
then reboot. Later file changes cause rollback to stop instead of overwriting
them.

Validation: 15 native FirstBoot tests passed (one optional compositor review
skipped), including the real QML/client/D-Bus eight-step flow and a failed
profile write. The 27 remote boundary tests, 22 browser fixture checks, two
profile-helper checks and setup-session check passed. The staged ARM64 shell
loaded both welcome variants and produced desktop screenshots inside the
isolated installed-image filesystem. Turkish welcome and appearance captures
were visually checked. Physical Pi installation still requires the commands
above and a subsequent device check.
