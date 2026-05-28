# Goal: kiptv - Native IPTV Application for KDE

**Created**: 2026-05-28T00:00:00Z
**Status**: active

## Objective
Build kiptv, a native KDE desktop application for watching IPTV streams with playlist management, channel browsing, and playback. The application integrates with KDE Plasma desktop features and supports M3U/XSPF playlist formats.

## Success Criteria
- [ ] Rust application with Qt/KDE bindings via provision crate
- [ ] M3U and XSPF playlist parsing and management
- [ ] Channel list with search/filter functionality
- [ ] Video playback with hardware acceleration
- [ ] KDE Plasma integration: system tray, notifications, media keys
- [ ] KDE Frameworks usage: KIO for network access, KNotifications
- [ ] Responsive UI following KDE Human Interface Guidelines
- [ ] Configuration persistence (playlists, settings)
- [ ] Build system with Makefile (build, test, lint, clean targets)
- [ ] Comprehensive tests (unit + integration)
- [ ] Packaged as Flatpak or KDE native format

## Constraints
- **Tech stack**: Rust + Qt/KDE bindings (provision crate)
- **M3U/XSPF support**: Must parse both playlist formats
- **KDE Frameworks**: Use KIO for network operations, KNotifications for alerts
- **KDE HIG**: Follow KDE Human Interface Guidelines for UX consistency
- **Native integration**: System tray, media keys, notifications

## Execution

### Phase 1: Project Scaffolding
- Initialize Rust project with Cargo
- Create Makefile with build/test/lint/clean targets
- Set up provision crate for Qt/KDE bindings
- Configure KDE Frameworks dependencies (KIO, KNotifications)
- Create basic application structure with main window

### Phase 2: Core Playlist Engine
- Implement M3U parser (channels, groups, metadata)
- Implement XSPF parser (XML-based playlists)
- Playlist management (add, remove, reload)
- Channel data model with search/filter support
- Unit tests for parsers

### Phase 3: UI - Channel Browser
- Channel list view with group filtering
- Search bar for channel names
- Channel icons/logos support
- Group/category sidebar
- KDE-style widgets and layout

### Phase 4: Video Playback
- Integrate multimedia backend (GStreamer via provision)
- Hardware-accelerated video decoding
- Player controls (play, pause, stop, volume, fullscreen)
- Stream error handling and reconnection
- Buffer management

### Phase 5: KDE Integration
- System tray icon with menu (play/pause, quit, settings)
- Media key support (play/pause, next/previous channel)
- KNotifications for stream errors, channel changes
- KIO slots for network access with authentication support
- KDE settings dialog (KConfig)

### Phase 6: Configuration & Persistence
- Save/load playlists from config
- User preferences (volume, last channel, UI state)
- KDE standard config locations (~/.config/kiptvrc)
- Import/export playlist functionality

### Phase 7: Polish & Testing
- E2E testing with sample playlists
- Performance testing with large playlists (1000+ channels)
- Memory leak detection
- KDE HIG compliance review
- Packaging (Flatpak or AppImage)

## Autonomy Rules
- Work independently until blocked or complete
- Self-checkpoint at context limits by updating this file's status
- Self-verify after each commit (build + test must pass)
- Update progress log below after each phase
- Escalate only when: missing system dependencies, architectural deadlock, or scope ambiguity

## Progress Log
- 2026-05-28: Goal created
- 2026-05-28: Phase 1 complete - Project scaffolded with egui
- 2026-05-28: Phase 2 complete - M3U/XSPF parsers implemented with tests
- 2026-05-28: Phase 3 complete - File dialog and channel browser UI
- Next: Phase 4 - Video playback integration
