# Changelog

All notable changes to qiptv are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.1.1] - 2026-11-03

### Fixed
- Player controls no longer steal keyboard focus, so arrow keys seek and
  change volume after clicking a button or slider.
- Volume above 130% now takes effect (mpv `volume-max` raised to 150).

### Added
- ±10 s seek buttons; mouse-wheel and `+`/`-` volume in 5% steps; volume %.
- Next/previous walk the list playback started from: live channels (wrap),
  movies in the category, and every episode of a series across seasons.
- Auto-advance to the next episode when one finishes.

## [0.1.0] - 2026-05-29

### Added

- Native Qt6 desktop IPTV client targeting the Xtream Codes API.
- Browse Live TV, Movies (VOD), and Series by category.
- libmpv playback with hardware decoding, seeking, volume, and fullscreen.
- Multiple profiles with auto-login to the last used profile.
- Channel zapping (next/previous), live text filter, and series detail view.
- System tray integration, media keys, desktop notifications.
- Settings dialog (live container, default volume, network cache, minimize-to-tray).
- Debian packaging (debhelper 13) and a Flatpak manifest.
- Desktop entry, AppStream metadata, and application icon.
- Open-source scaffolding: license, contributing guide, code of conduct,
  security policy, issue/PR templates, and CI.

[Unreleased]: https://github.com/xmonader/qiptv/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/xmonader/qiptv/releases/tag/v0.1.0
