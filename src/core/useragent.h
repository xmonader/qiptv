#pragma once

namespace kiptv {

// User-Agent sent with every player_api.php request (QNetworkAccessManager
// sends none by default) and to mpv for stream downloads. Some provider
// front-ends (nginx) reject requests without a User-Agent outright (403),
// which made login and playback impossible. Any non-empty value is accepted;
// keep it honest and identifiable. Keep in sync with the project version.
inline constexpr char kUserAgent[] = "kiptv/0.1.0";

} // namespace kiptv
