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
  "id": "org.gimp.GIMP",
  "description": "A factual description of what the application does.",
  "features": ["Image editing", "Layers", "Filters"],
  "website": "https://apps.flathub.org/org.gimp.GIMP",
  "architectures": ["amd64", "arm64"]
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

The current catalog contains 40 applications. Verify Flatpak IDs and architectures
with `flatpak remote-ls --user --app --arch='*' --columns=application,arch flathub`.
The store disables installation for incompatible catalog architectures. The
`FIDAN_STORE_CATALOG` environment variable selects a development/test catalog.

## Details and progress

Click a card to open its detail page. Descriptions and feature lists come from
the catalog. Download size, installed size, architecture, version/commit and
available license fields are queried asynchronously from `apt-cache` or
`flatpak remote-info`. Missing data is shown as unavailable; there are no
invented ratings, download counts, screenshots or package sizes. Flatpak shared
runtimes can require additional space beyond the application's package size.

Details and actions use a stable application ID, so filtering or a model refresh
does not silently change the target of an installation. The Downloads page
shows the active/last transaction, its real percentage when provided, phase,
bounded output log and error/retry control. Only one package writer runs at a
time. APT reports structured `dlstatus`/`pmstatus` progress. The unprivileged
`store-transaction` helper gives Flatpak a pseudo-terminal to expose its real
progress output. Unknown progress remains indeterminate; completion is shown
only after the package manager exits successfully and installed state is checked.

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

PiOS desktop updates use a separate signed GitHub release updater and system
timer. Installed PiOS packages are checked and updated every six hours when
automatic updates are enabled. The store manages third-party APT/Flatpak apps;
it does not silently upgrade every application. See [release maintenance](release.md).

Before publishing the first image, install the project release public key at
the `updateKeyring` path in `config/product.json`. The corresponding private
release subkey stays in the protected release workflow and must never be added
to the image or repository.
