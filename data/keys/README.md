# PiOS update trust key

`fidan-release-keyring.gpg` is a public OpenPGP keyring used by `gpgv`.

Primary fingerprint: `10855324032F285AFB73CCD4C41FF8075817D3B6`.

The primary key certifies a dedicated Ed25519 signing subkey. The repository
contains no private key material. Release automation receives the signing
subkey through the encrypted `FIDAN_RELEASE_GPG_KEY` GitHub Actions secret.
See [release maintenance](../../docs/release.md) before changing this key.
