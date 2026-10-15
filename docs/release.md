# Release process

Project Fidan publishes only from tags matching `v*`. The release job runs on
the trusted, self-hosted ARM64 runner labeled `fidan-image-builder`.

Releases and issue reports use `Taxperia/PiOS`, configured through `githubRepo`
and `supportUrl` in `config/product.json`. Alpha and
beta tags must contain a hyphen, for example `v0.1.0-alpha.2`; the workflow marks
those GitHub releases as prereleases so the updater's matching channel can find
them.

The repository secret `FIDAN_RELEASE_GPG_KEY` must contain an ASCII-armored
export of the release signing subkey. The offline root key must never be added
to GitHub or copied to the runner. The signing script imports the subkey into a
temporary `GNUPGHOME`, signs the APT metadata and release checksums, exports
only the public key, and removes the temporary keyring when it exits.

Each release contains the compressed Pi image, Debian packages, SBOM,
`SHA256SUMS`, `SHA256SUMS.asc`, and a compressed signed APT repository. Before
creating a tag, complete every item in the hardware acceptance checklist and
verify the tag matches `config/product.json`.

The public half of the release key must be installed in released images at the
`updateKeyring` path declared in `config/product.json`. The updater checks GitHub
automatically, but refuses to download installable packages when this keyring is
missing, when `SHA256SUMS.asc` is invalid, or when a package hash is absent.
