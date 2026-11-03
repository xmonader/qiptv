#pragma once

#include <QString>
#include <QVector>

namespace qiptv {

struct PlaylistEntry {
    QString url;
    QString title;
};

/**
 * An ordered snapshot of playable items plus a cursor, taken when playback
 * starts (the channels of a category, the movies of a category, or every
 * episode of a series). Owning a snapshot keeps next/previous working after
 * the user browses elsewhere while the player stays open.
 *
 * Wrapping playlists (live channels) cycle; non-wrapping ones (episodes,
 * movies) stop at either end.
 */
class Playlist {
public:
    Playlist() = default;
    /// An out-of-range @p current yields an empty playlist.
    /// @param autoAdvance  play the next entry when one finishes (episodes)
    Playlist(QVector<PlaylistEntry> entries, int current, bool wraps, bool autoAdvance = false);

    bool isEmpty() const { return m_entries.isEmpty(); }
    int size() const { return static_cast<int>(m_entries.size()); }
    int currentIndex() const { return m_current; }
    bool autoAdvance() const { return m_autoAdvance; }
    /// Requires !isEmpty().
    const PlaylistEntry &current() const { return m_entries.at(m_current); }

    /// Whether step(delta) would move to a different entry.
    bool canStep(int delta) const;
    /// Move the cursor by @p delta; returns false (cursor unchanged) if it can't.
    bool step(int delta);

private:
    int targetIndex(int delta) const; ///< -1 when there is no valid target

    QVector<PlaylistEntry> m_entries;
    int m_current = -1;
    bool m_wraps = false;
    bool m_autoAdvance = false;
};

} // namespace qiptv
