# Distribution

## Native install

```bash
make build
cmake --install build --prefix /usr/local
```

Installs the `kiptv` binary plus the desktop entry, AppStream metainfo,
KXmlGui `.rc`, and `kiptv.notifyrc` to the standard KDE locations.

## Flatpak

A manifest is provided at `packaging/io.github.xmonader.kiptv.yaml`. It uses the
KDE runtime and bundles libmpv + libplacebo, with ffmpeg supplied by the
freedesktop ffmpeg-full extension.

> ⚠️ This manifest has not been built in the project's development environment
> (no `flatpak-builder` / KDE runtime there). Treat module versions as a
> starting point.

```bash
flatpak install flathub org.kde.Platform//6.8 org.kde.Sdk//6.8
flatpak-builder --user --install --force-clean build-flatpak \
    packaging/io.github.xmonader.kiptv.yaml
flatpak run io.github.xmonader.kiptv
```

If a libmpv/libplacebo build fails, bump the `tag:` fields to current releases.

## AppImage (alternative)

Build with a release prefix, then use `linuxdeploy` with the Qt and KF6
plugins. libmpv and its codecs must be bundled or relied upon from the host.
This path is not scripted in-tree yet.
