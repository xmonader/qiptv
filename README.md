# kiptv

A native **KDE** desktop IPTV client written in **C++**, targeting the
**Xtream Codes** API with **libmpv** playback.

Log in with your provider's server URL and credentials to browse Live TV,
Movies (VOD) and Series, and play them with hardware-accelerated libmpv.

## Features

- 📺 **Live TV, Movies & Series** — browse by category from any Xtream Codes provider
- 👥 **Multiple profiles** — save several accounts; auto-login to the last used
- ▶️ **libmpv playback** — hardware decoding, seeking (VOD/series), volume, fullscreen
- ⏭️ **Channel zapping** — next/previous live channel via buttons or media keys
- 🔍 **Filtering** — live text filter over the current list
- 🎬 **Series detail** — season selector and episode list
- 🔔 **KDE integration** — system tray, global media keys, notifications
- ⚙️ **Settings** — live container, default volume, network cache, minimize-to-tray

## Tech stack

| Concern | Choice |
|---------|--------|
| Language | C++17 |
| Build | CMake + extra-cmake-modules |
| UI | KDE Frameworks 6 (KXmlGui, QWidgets) |
| Playback | libmpv |
| HTTP / JSON | Qt Network + Qt JSON |
| Storage | KConfig (passwords obfuscated; KWallet when available) |

## Build & run

Prerequisites: a C++17 compiler, CMake ≥ 3.20, Qt 6, KDE Frameworks 6
(CoreAddons, XmlGui, Config, ConfigWidgets, I18n, Notifications, GlobalAccel,
KIO, WidgetsAddons, IconThemes), and `libmpv`.

```bash
make build   # configure + compile
make test    # run the unit/integration test suite
make run     # launch the app
make lint    # clang-tidy (if available)
make clean
```

The `Makefile` is a thin wrapper over CMake; the build tree lives in `build/`.

## Usage

1. Launch kiptv. On first run you'll see the login screen.
2. Enter your provider **Server URL** (e.g. `http://example.com:8080`),
   **Username** and **Password**. Keep "Save this profile" checked to store it.
3. After authentication, pick **Live TV**, **Movies** or **Series** in the left
   rail, choose a category, and double-click an item to play.
4. In the player: Space toggles pause, ←/→ seek, ↑/↓ volume, `F` fullscreen,
   `N`/`P` (or media keys) change channel, Back returns to browsing.

## Documentation

- [Architecture](docs/ARCHITECTURE.md)
- [Development](docs/DEVELOPMENT.md)
- [Testing & live-provider verification](docs/TESTING.md)
- [Distribution](docs/DISTRIBUTION.md)

## Project layout

```
src/
├── core/        parse helpers, ImageLoader
├── api/         UrlBuilder, XtreamClient
├── models/      Category, ServerInfo, LiveStream, VodStream, Series, …
├── config/      Profile, ProfileStore, Settings
├── auth/        LoginWidget
├── browse/      BrowseWidget, SeriesDetail
├── player/      MpvWidget, PlayerWidget
└── app/         MainWindow, SettingsDialog, main
tests/           QtTest suites (UI-free logic + offscreen widgets)
data/            .desktop and AppStream metainfo
packaging/       Flatpak manifest
```

## License

MIT. See [LICENSE](LICENSE).

## Disclaimer

For use with IPTV services you are legally entitled to access. kiptv ships no
content and no provider credentials.
