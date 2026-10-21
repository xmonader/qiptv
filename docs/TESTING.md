# Testing

## Automated suite

```bash
make test
```

Runs nine CTest suites:

| Suite | Covers |
|-------|--------|
| `parse` | JSON coercion helpers against messy Xtream shapes |
| `models` | All model parsers + a 5000-item performance check |
| `urlbuilder` | player_api and stream URL construction + encoding |
| `xtreamclient` | the async client end to end via an embedded mock HTTP server |
| `profilestore` | profile persistence round-trips (incl. plaintext-leak guard) |
| `settings` | preference persistence and clamping |
| `imageloader` | cached/dedup image fetching (cache-hit = no second request) |
| `loginwidget` | login validation and signal emission (offscreen) |
| `appstreamtest` | AppStream metainfo validation |

All logic that does not require a display is unit-tested. Widget tests run
under `QT_QPA_PLATFORM=offscreen`.

## Live-provider verification (manual)

Authentication and playback cannot be fully automated without a real Xtream
provider and a display/GPU. The mock-HTTP `xtreamclient` suite exercises the
client's request/parse/emit paths; the steps below verify the real thing.

You will need: a working Xtream Codes account (server URL, username, password)
and a desktop session (X11 or Wayland).

1. `make run`
2. Log in with the provider credentials. **Expected:** the browser appears and
   the window title shows your username. A wrong password shows an inline error
   and stays on the login screen.
3. Select **Live TV** → a category. **Expected:** channels list with logos
   loading in; the status line shows the channel count.
4. Double-click a channel. **Expected:** libmpv starts playback within a few
   seconds; the seek bar is disabled (live).
5. Press `N`/`P`. **Expected:** zaps to the next/previous channel.
6. Back → **Movies** → a movie. **Expected:** playback with a working seek bar.
7. Back → **Series** → a series → a season → an episode. **Expected:** the
   episode plays.
8. Toggle fullscreen (`F`), adjust volume (↑/↓), pause (Space). Restart the app
   and confirm the volume and last profile are remembered.

If playback fails, check the terminal for mpv/libmpv messages and confirm the
stream URL is reachable (the `playbackError` notification reports the cause).

### Known environment notes

- Window embedding uses mpv's `wid`; a pure-Wayland session may route through
  XWayland. A GPU render-API path is a possible future enhancement.
- `KStatusNotifierItem` is unavailable on some hosts; the tray falls back to
  `QSystemTrayIcon`.
- `KWallet` is optional; without it passwords are stored base64-obfuscated in
  `qiptvrc` (obfuscation, **not** encryption).
