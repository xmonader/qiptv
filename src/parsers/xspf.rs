use anyhow::{Context, Result};
use quick_xml::events::Event;
use quick_xml::Reader;
use std::fs;

use crate::models::{Channel, Playlist};

pub struct XSPFParser;

impl XSPFParser {
    pub fn parse_file(path: &str) -> Result<Playlist> {
        let content = fs::read_to_string(path)
            .with_context(|| format!("Failed to read XSPF file: {}", path))?;
        Self::parse(&content, path)
    }

    pub fn parse(content: &str, source: &str) -> Result<Playlist> {
        let mut playlist = Playlist::new(source.to_string());
        let mut reader = Reader::from_str(content);
        let mut buf = Vec::new();
        let mut current_track: Option<Channel> = None;
        let mut current_field = String::new();

        loop {
            match reader.read_event_into(&mut buf) {
                Ok(Event::Start(e)) | Ok(Event::Empty(e)) => {
                    let name = String::from_utf8_lossy(e.name().as_ref()).to_string();
                    match name.as_str() {
                        "track" => {
                            current_track = Some(Channel {
                                name: String::new(),
                                url: String::new(),
                                group: String::new(),
                                logo: None,
                            });
                        }
                        "location" => current_field = "location".to_string(),
                        "title" => current_field = "title".to_string(),
                        "album" => current_field = "group".to_string(),
                        "image" => current_field = "logo".to_string(),
                        _ => {}
                    }
                }
                Ok(Event::Text(e)) => {
                    let text = e.unescape()?;
                    if let Some(ref mut track) = current_track {
                        match current_field.as_str() {
                            "location" => track.url = text.to_string(),
                            "title" => track.name = text.to_string(),
                            "group" => track.group = text.to_string(),
                            "logo" => track.logo = Some(text.to_string()),
                            _ => {}
                        }
                    }
                    current_field.clear();
                }
                Ok(Event::End(e)) => {
                    let name = String::from_utf8_lossy(e.name().as_ref()).to_string();
                    if name == "track" {
                        if let Some(track) = current_track.take() {
                            playlist.add_channel(track);
                        }
                    }
                }
                Ok(Event::Eof) => break,
                Err(e) => anyhow::bail!("XML parse error: {:?}", e),
                _ => {}
            }
            buf.clear();
        }

        Ok(playlist)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_parse_simple_xspf() {
        let content = r#"<?xml version="1.0" encoding="UTF-8"?>
<playlist xmlns="http://xspf.org/ns/0/">
    <trackList>
        <track>
            <location>http://example.com/stream1</location>
            <title>Channel 1</title>
        </track>
        <track>
            <location>http://example.com/stream2</location>
            <title>Channel 2</title>
        </track>
    </trackList>
</playlist>"#;
        let playlist = XSPFParser::parse(content, "test.xspf").unwrap();
        assert_eq!(playlist.channels.len(), 2);
        assert_eq!(playlist.channels[0].name, "Channel 1");
        assert_eq!(playlist.channels[1].name, "Channel 2");
    }

    #[test]
    fn test_parse_xspf_with_metadata() {
        let content = r#"<?xml version="1.0" encoding="UTF-8"?>
<playlist xmlns="http://xspf.org/ns/0/">
    <trackList>
        <track>
            <location>http://example.com/news</location>
            <title>News Channel</title>
            <album>News</album>
            <image>http://example.com/logo.png</image>
        </track>
    </trackList>
</playlist>"#;
        let playlist = XSPFParser::parse(content, "test.xspf").unwrap();
        assert_eq!(playlist.channels[0].name, "News Channel");
        assert_eq!(playlist.channels[0].group, "News");
        assert_eq!(playlist.channels[0].logo, Some("http://example.com/logo.png".to_string()));
    }

    #[test]
    fn test_empty_xspf() {
        let content = r#"<?xml version="1.0" encoding="UTF-8"?>
<playlist xmlns="http://xspf.org/ns/0/">
    <trackList>
    </trackList>
</playlist>"#;
        let playlist = XSPFParser::parse(content, "test.xspf").unwrap();
        assert_eq!(playlist.channels.len(), 0);
    }
}
