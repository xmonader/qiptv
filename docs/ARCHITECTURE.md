# Architecture

kiptv is layered so that all non-UI logic lives in a static library
(`kiptv_core`) that the application and the tests both link. UI code lives only
in the `kiptv` executable.

```
            ┌────────────── kiptv (executable) ──────────────┐
            │  app/      MainWindow, SettingsDialog, main      │
            │  auth/     LoginWidget                           │
            │  browse/   BrowseWidget, SeriesDetail            │
            │  player/   MpvWidget (libmpv), PlayerWidget      │
            └───────────────────┬─────────────────────────────┘
                                │ depends on
            ┌───────────────────▼──── kiptv_core ─────────────┐
            │  api/      UrlBuilder, XtreamClient (QObject)     │
            │  models/   Category, ServerInfo, LiveStream, …    │
            │  core/     parse (coercion), ImageLoader          │
            │  config/   Profile, ProfileStore, Settings        │
            └──────────────────────────────────────────────────┘
```

## Data flow

1. **Login** — `LoginWidget` emits a `Profile`; `MainWindow` sets credentials on
   `XtreamClient` and calls `authenticate()`.
2. **Auth** — `XtreamClient` GETs `player_api.php`, parses `ServerInfo`, and
   emits `authenticated` (or `errorOccurred`). On success the profile is saved.
3. **Browse** — `BrowseWidget` requests categories/streams per section; the
   client emits typed `*Ready` signals; posters load lazily via `ImageLoader`.
4. **Play** — activating an item builds a stream URL with `UrlBuilder` and emits
   `playRequested`; `MainWindow` hands it to `PlayerWidget`/`MpvWidget`.

## Key decisions

- **Defensive parsing** (`core/parse`) absorbs Xtream's string/int/null
  inconsistencies so the rest of the code works with clean typed structs.
- **Injectable `QNetworkAccessManager`** in `XtreamClient` and `ImageLoader`
  makes them testable against an embedded mock HTTP server with no real
  provider.
- **`wid`-based mpv embedding** keeps the player simple and robust on X11;
  libmpv events are marshalled from mpv's thread to the GUI thread via a queued
  slot.
- **KConfig everywhere** for profiles and settings; KWallet is used for
  passwords when present, otherwise a documented obfuscation fallback.
