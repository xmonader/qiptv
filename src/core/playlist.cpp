#include "core/playlist.h"

#include <utility>

namespace qiptv {

Playlist::Playlist(QVector<PlaylistEntry> entries, int current, bool wraps, bool autoAdvance)
    : m_wraps(wraps)
    , m_autoAdvance(autoAdvance)
{
    if (current >= 0 && current < entries.size()) {
        m_entries = std::move(entries);
        m_current = current;
    }
}

int Playlist::targetIndex(int delta) const
{
    const int n = size();
    if (n < 2 || delta == 0) {
        return -1;
    }
    const int raw = m_current + delta;
    if (m_wraps) {
        return ((raw % n) + n) % n;
    }
    return (raw >= 0 && raw < n) ? raw : -1;
}

bool Playlist::canStep(int delta) const
{
    const int t = targetIndex(delta);
    return t >= 0 && t != m_current;
}

bool Playlist::step(int delta)
{
    if (!canStep(delta)) {
        return false;
    }
    m_current = targetIndex(delta);
    return true;
}

} // namespace qiptv
