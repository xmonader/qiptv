# Goal: kiptv — Native KDE IPTV Application (C++)

**Created**: 2026-05-29T00:00:00Z
**Status**: active

## Objective
Build kiptv, a native KDE desktop application in **C++** for watching IPTV
content delivered via the **Xtream Codes API**. The user logs in with a server
URL, username, and password; the app browses Live TV, Movies (VOD), and Series
by category, plays streams with **libmpv**, and persists multiple provider
profiles. UI is built on **KDE Frameworks with QWidgets** (KXmlGui), following
KDE Human Interface Guidelines. "Done" = a packaged KDE app that authenticates
against a real Xtream provider, lists categories/streams, and plays live + VOD +
series episodes with player controls.

## Locked Decisions
- **Language**: C++ (C++17 or newer), CMake + extra-cmake-modules (ECM).
- **UI**: KDE Frameworks QWidgets — KXmlGui (KMainWindow, KActionCollection),
  not Kirigami/QML.
- **Source model**: Xtream Codes API only (port of iptvfree feature set).
- **Playback**: libmpv embedded via render API (no QtMultimedia).
- **Storage**: KConfig for settings; profiles persisted with KWallet for
  passwords where available, falling back to KConfig.

## Xtream Codes API Surface (from reference iptvfree)
Base: `{server}/player_api.php?username={u}&password={p}`

| Action | Returns |
|--------|---------|
| (no action) | Auth + server/user info |
| `get_live_categories` | Live categories |
| `get_vod_categories` | Movie categories |
| `get_series_categories` | Series categories |
| `get_live_streams[&category_id]` | Live channels |
| `get_vod_streams[&category_id]` | Movies |
| `get_series[&category_id]` | Series list |
| `get_series_info&series_id={id}` | Series details + episodes |
| `get_vod_info&vod_id={id}` | Movie details |
| `get_simple_data_table&stream_id={id}` | EPG (short) |

Stream URL patterns:
- Live: `{server}/live/{u}/{p}/{streamId}.ts`
- VOD: `{server}/movie/{u}/{p}/{streamId}.{ext}`
- Series: `{server}/series/{u}/{p}/{episodeId}.{ext}`

## Success Criteria
- [ ] C++/CMake project builds with KDE Frameworks (KCoreAddons, KXmlGui,
      KConfig, KConfigWidgets, KIO, KNotifications, KWallet) and libmpv.
- [ ] Makefile wrapping CMake with `build`, `test`, `lint`, `clean` targets.
- [ ] Xtream API client: authenticate + all actions above, with robust JSON
      parsing (handles string/int polymorphism common in Xtream responses).
- [ ] Data models: ServerInfo, UserInfo, Category, LiveStream, VodStream,
      VodInfo, Series, SeriesInfo, Episode, Profile.
- [ ] Login screen: server URL, username, password; validates against server.
- [ ] Profile management: save/switch/delete multiple accounts; auto-login.
- [ ] Main window with navigation (Live / Movies / Series / Settings).
- [ ] Category browsing + stream/poster grid with cached images and fallbacks.
- [ ] Series detail view with season/episode navigation.
- [ ] libmpv player: play/pause/stop, seek (VOD/series), volume, fullscreen,
      hardware decoding, auto-hiding controls, keyboard shortcuts.
- [ ] Live channel zapping (next/previous) and media-key support.
- [ ] KNotifications for stream errors and state changes.
- [ ] Settings dialog (KConfigDialog): default container, buffering, theme.
- [ ] Configuration + secure credential persistence (KConfig + KWallet).
- [ ] Unit tests for API client (mocked HTTP) + JSON parsing edge cases.
- [ ] E2E/integration smoke test path documented for a live provider.
- [ ] Packaged as Flatpak or AppImage.

## Constraints
- **No QtMultimedia** — playback strictly via libmpv.
- **No QML/Kirigami** — QWidget-based KDE app only.
- **Xtream JSON is messy**: numeric fields arrive as strings or ints; parsing
  must coerce defensively and never crash on missing/null fields.
- **Network is hostile**: timeouts, auth failures, partial responses, and slow
  servers must surface as user-visible errors, not hangs.
- **Secrets**: never log credentials; never commit a real provider account.
- **KDE HIG**: native menus, toolbars, shortcuts, and standard config locations.

## Execution

### Phase 1: Project Scaffolding
- CMake + ECM project; find KDE Frameworks + libmpv.
- Makefile wrapping CMake (build/test/lint/clean).
- KMainWindow skeleton with KActionCollection and About dialog.
- CI-friendly build; commit working skeleton.

### Phase 2: Xtream API Client + Models
- HTTP layer (QNetworkAccessManager or KIO) with timeouts.
- URL builder for player_api.php actions and stream URLs.
- Data models with defensive JSON parsing (string/int coercion).
- `authenticate()` + all category/stream/info actions.
- Unit tests with mocked responses incl. malformed/null fields.

### Phase 3: Auth + Profiles
- Login screen (server/user/pass) wired to `authenticate()`.
- Profile model; save/switch/delete; KWallet for passwords.
- Auto-login to last-used profile on launch.

### Phase 4: Browsing UI
- Navigation (Live / Movies / Series / Settings).
- Category list + reusable poster/stream grid widget.
- Cached network images with fallback icons.
- Series detail view: seasons + episodes.
- Refresh action that re-fetches from server.

### Phase 5: libmpv Playback
- Embed libmpv via render API in a QWidget.
- Play/pause/stop, volume, fullscreen, hardware decode.
- Seek bar for VOD/series; auto-hiding controls; keyboard shortcuts.
- Build correct stream URLs per content type.
- Error handling + reconnection; KNotifications on failure.

### Phase 6: KDE Integration
- Media-key support; live channel next/previous zapping.
- System tray with quick controls.
- KConfigDialog settings (container, buffering, theme).
- KConfig persistence for prefs and last state.

### Phase 7: Polish, Test, Package
- Integration smoke test against a live provider (documented).
- Performance with large category/stream lists (1000+).
- KDE HIG review; keyboard accessibility.
- Package as Flatpak or AppImage; update README + docs.

## Autonomy Rules
- Work independently until blocked or complete.
- Build + test after every change; never commit broken builds.
- Micro-commits: one logical change each, `type(scope): subject`.
- Self-verify each criterion before checking it off.
- Escalate only on true blockers (e.g., no test provider available, missing
  system libs that require user install). Set status=blocked and ask.
- Update this file's progress as phases complete.

## Progress Log
- 2026-05-29: Goal created. Stack locked (C++ / KDE QWidgets / libmpv /
  Xtream Codes API). Prior Rust+egui attempt superseded — to be removed.
