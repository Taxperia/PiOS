# Security policy

Please report vulnerabilities privately to the maintainers rather than opening
a public issue. Until a final project domain and security mailbox exist, use a
private GitHub security advisory in the project repository.

The sensitive trust boundaries are:

- `greetd` owns login authentication; the QML greeter never reads password
  databases directly.
- `gtklock` owns PAM authentication and `ext-session-lock-v1`; the shell cannot
  unlock a session.
- `org.fidan.FirstBoot1` runs as root and accepts calls only from root or the
  dedicated local `fidan-setup` system user.
- shutdown and restart use logind/Polkit. The desktop shell does not run as root.
- no default password, SSH service, telemetry, or remote administration is
  enabled in the image.

