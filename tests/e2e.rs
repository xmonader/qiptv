use kiptv::models::Playlist;
use kiptv::parsers::{parse_playlist, m3u::M3UParser, xspf::XSPFParser};

#[test]
fn test_e2e_m3u_parsing() {
    let playlist = parse_playlist("test-playlist.m3u").expect("Failed to parse M3U");
    assert_eq!(playlist.channels.len(), 5);
    assert_eq!(playlist.channels[0].name, "News 24");
    assert_eq!(playlist.channels[0].group, "News");
    assert_eq!(playlist.channels[1].name, "Sports HD");
    assert_eq!(playlist.channels[1].group, "Sports");
}

#[test]
fn test_e2e_xspf_parsing() {
    let playlist = parse_playlist("test-playlist.xspf").expect("Failed to parse XSPF");
    assert_eq!(playlist.channels.len(), 2);
    assert_eq!(playlist.channels[0].name, "XSPF Channel 1");
    assert_eq!(playlist.channels[0].group, "Test");
}

#[test]
fn test_playlist_search() {
    let playlist = parse_playlist("test-playlist.m3u").expect("Failed to parse");
    let results = playlist.search("news");
    assert_eq!(results.len(), 1);
    assert_eq!(results[0].name, "News 24");
}

#[test]
fn test_playlist_groups() {
    let playlist = parse_playlist("test-playlist.m3u").expect("Failed to parse");
    let groups = playlist.channels_by_group();
    assert!(groups.contains_key("News"));
    assert!(groups.contains_key("Sports"));
    assert!(groups.contains_key("Entertainment"));
}
