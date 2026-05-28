use anyhow::{Context, Result};
use serde::{Deserialize, Serialize};
use std::fs;
use std::path::PathBuf;

#[derive(Debug, Serialize, Deserialize)]
pub struct Config {
    pub last_playlist: Option<String>,
    pub volume: f64,
    pub window_size: (u32, u32),
}

impl Default for Config {
    fn default() -> Self {
        Self {
            last_playlist: None,
            volume: 1.0,
            window_size: (1200, 800),
        }
    }
}

impl Config {
    pub fn config_dir() -> PathBuf {
        let home = std::env::var("HOME").unwrap_or_else(|_| "/home/user".to_string());
        PathBuf::from(home).join(".config").join("kiptv")
    }

    pub fn config_file() -> PathBuf {
        Self::config_dir().join("kiptv.conf")
    }

    pub fn load() -> Result<Self> {
        let config_file = Self::config_file();
        if !config_file.exists() {
            return Ok(Self::default());
        }

        let content = fs::read_to_string(&config_file)
            .with_context(|| format!("Failed to read config: {:?}", config_file))?;
        
        let config: Config = toml::from_str(&content)
            .with_context(|| "Failed to parse config")?;

        Ok(config)
    }

    pub fn save(&self) -> Result<()> {
        let config_dir = Self::config_dir();
        fs::create_dir_all(&config_dir)
            .with_context(|| format!("Failed to create config dir: {:?}", config_dir))?;

        let config_file = Self::config_file();
        let content = toml::to_string_pretty(self)
            .with_context(|| "Failed to serialize config")?;

        fs::write(&config_file, content)
            .with_context(|| format!("Failed to write config: {:?}", config_file))?;

        Ok(())
    }
}
