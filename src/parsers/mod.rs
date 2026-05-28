pub mod m3u;
pub mod xspf;

use anyhow::Result;
use std::path::Path;

use crate::models::Playlist;

pub fn parse_playlist(path: &str) -> Result<Playlist> {
    let ext = Path::new(path)
        .extension()
        .and_then(|e| e.to_str())
        .map(|e| e.to_lowercase());

    match ext.as_deref() {
        Some("m3u") | Some("m3u8") => m3u::M3UParser::parse_file(path),
        Some("xspf") => xspf::XSPFParser::parse_file(path),
        _ => anyhow::bail!("Unsupported playlist format: {:?}", ext),
    }
}
