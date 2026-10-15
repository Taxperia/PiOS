# Fidan Store catalog

The store reads [`data/store/catalog.json`](../data/store/catalog.json). The
catalog is packaged at `/usr/share/fidan/store/catalog.json`; changing the QML
application is not required when an entry is added or removed.

## Add an application

Add one object to the `apps` array:

```json
{
  "name": "GIMP",
  "summary": "Image editing and graphic design",
  "category": "Graphics",
  "icon": "org.gimp.GIMP",
  "source": "flatpak",
  "id": "org.gimp.GIMP"
}
```

Supported sources are:

- `flatpak`: `id` is the exact Flathub application ID. Installation is per
  user and does not need root access. The store adds the user's Flathub remote
  on the first install. Test an ID with `flatpak remote-info flathub ID`.
- `apt`: `id` is the exact Debian/Raspberry Pi OS package name. Installation
  opens a PolicyKit authorization prompt. Test it with `apt-cache show PACKAGE`.

Use a stable application or package ID. The store rejects IDs containing shell
syntax and starts package programs directly without a shell. Keep application
names and summaries factual; the catalog is a curated list, not an arbitrary
command launcher.

The development session starts `lxqt-policykit-agent` for APT authentication.
Run `./scripts/setup-debian-dev` to install `pkexec`, `polkitd`, Flatpak and the
GTK desktop portal. Flathub setup during installation is asynchronous, so the
store remains responsive while the source is prepared. Reopen the launcher to
discover newly installed Debian and Flatpak applications.

After editing the catalog, run:

```sh
cmake --build --preset dev
ctest --preset dev --output-on-failure
./build/dev/src/store/fidan-store
```

## Remove an application

Delete its complete object from the `apps` array. This removes it from future
catalogs but does not uninstall it from existing computers. Users can remove an
installed application with the store before the entry is removed, or with
`flatpak uninstall ID` / `sudo apt remove PACKAGE` afterward.

## GitHub updates

`fidan-update check --notify` runs daily from a systemd user timer. Feature
updates are never installed silently. `fidan-update download` accepts only
packages listed in a GPG-signed `SHA256SUMS`; `fidan-update install` then opens
PolicyKit authorization.

Before publishing the first image, install the project release public key at
the `updateKeyring` path in `config/product.json`. The corresponding private
release subkey stays in the protected release workflow and must never be added
to the image or repository.
