use anyhow::{Context, Result};
use regex::Regex;
use std::fs;

use crate::models::{Channel, Playlist};

pub struct M3UParser;

impl M3UParser {
    pub fn parse_file(path: &str) -> Result<Playlist> {
        let content = fs::read_to_string(path)
            .with_context(|| format!("Failed to read M3U file: {}", path))?;
        Self::parse(&content, path)
    }

    pub fn parse(content: &str, source: &str) -> Result<Playlist> {
        let mut playlist = Playlist::new(source.to_string());
        
        let mut current_channel: Option<Channel> = None;
        let extinf_regex = Regex::new(r"#EXTINF:.*?,(.+)")?;
        let logo_regex = Regex::new(r#"tvg-logo="([^"]*)""#)?;
        let group_regex = Regex::new(r#"group-title="([^"]*)""#)?;

        for line in content.lines() {
            let line = line.trim();
            
            if line.starts_with("#EXTINF:") {
                let name = extinf_regex
                    .captures(line)
                    .and_then(|c| c.get(1))
                    .map(|m| m.as_str().trim().to_string())
                    .unwrap_or_else(|| "Unknown".to_string());

                let logo = logo_regex
                    .captures(line)
                    .and_then(|c| c.get(1))
                    .map(|m| m.as_str().to_string());

                let group = group_regex
                    .captures(line)
                    .and_then(|c| c.get(1))
                    .map(|m| m.as_str().to_string())
                    .unwrap_or_default();

                current_channel = Some(Channel {
                    name,
                    url: String::new(),
                    group,
                    logo,
                });
            } else if !line.is_empty() && !line.starts_with('#') {
                if let Some(mut channel) = current_channel.take() {
                    channel.url = line.to_string();
                    playlist.add_channel(channel);
                }
            }
        }

        Ok(playlist)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_parse_simple_m3u() {
        let content = r#"#EXTM3U
#EXTINF:-1,Channel 1
http://example.com/stream1
#EXTINF:-1,Channel 2
http://example.com/stream2
"#;
        let playlist = M3UParser::parse(content, "test.m3u").unwrap();
        assert_eq!(playlist.channels.len(), 2);
        assert_eq!(playlist.channels[0].name, "Channel 1");
        assert_eq!(playlist.channels[1].name, "Channel 2");
    }

    #[test]
    fn test_parse_m3u_with_metadata() {
        let content = r#"#EXTM3U
#EXTINF:-1 tvg-logo="http://example.com/logo.png" group-title="News",News Channel
http://example.com/news
"#;
        let playlist = M3UParser::parse(content, "test.m3u").unwrap();
        assert_eq!(playlist.channels[0].name, "News Channel");
        assert_eq!(playlist.channels[0].group, "News");
        assert_eq!(playlist.channels[0].logo, Some("http://example.com/logo.png".to_string()));
    }

    #[test]
    fn test_empty_m3u() {
        let content = "#EXTM3U\n";
        let playlist = M3UParser::parse(content, "test.m3u").unwrap();
        assert_eq!(playlist.channels.len(), 0);
    }
}
