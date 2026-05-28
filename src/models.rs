use serde::{Deserialize, Serialize};

#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct Channel {
    pub name: String,
    pub url: String,
    pub group: String,
    pub logo: Option<String>,
}

#[derive(Clone, Debug)]
pub struct Playlist {
    pub name: String,
    pub channels: Vec<Channel>,
}

impl Playlist {
    pub fn new(name: String) -> Self {
        Self {
            name,
            channels: Vec::new(),
        }
    }

    pub fn add_channel(&mut self, channel: Channel) {
        self.channels.push(channel);
    }

    pub fn channels_by_group(&self) -> std::collections::HashMap<String, Vec<&Channel>> {
        let mut groups = std::collections::HashMap::new();
        for channel in &self.channels {
            groups.entry(channel.group.clone()).or_insert_with(Vec::new).push(channel);
        }
        groups
    }

    pub fn search(&self, query: &str) -> Vec<&Channel> {
        let query = query.to_lowercase();
        self.channels
            .iter()
            .filter(|c| c.name.to_lowercase().contains(&query))
            .collect()
    }
}
