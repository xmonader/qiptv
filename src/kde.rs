use anyhow::{Context, Result};
use zbus::Connection;
use std::sync::Arc;
use tokio::sync::Mutex;

use crate::player::Player;

pub struct KDEIntegration {
    connection: Option<Connection>,
    player: Arc<Mutex<Player>>,
}

impl KDEIntegration {
    pub fn new(player: Player) -> Self {
        Self {
            connection: None,
            player: Arc::new(Mutex::new(player)),
        }
    }

    pub async fn init(&mut self) -> Result<()> {
        let conn = Connection::session().await
            .context("Failed to connect to D-Bus session bus")?;
        
        self.connection = Some(conn);
        Ok(())
    }

    pub async fn register_media_player(&self, app_name: &str) -> Result<()> {
        let conn = self.connection.as_ref()
            .context("D-Bus not initialized")?;

        // Register with MPRIS (Media Player Remote Interoperability Specification)
        // This allows KDE media keys to control the player
        let bus_name = format!("org.mpris.MediaPlayer2.{}", app_name.to_lowercase());
        
        conn.request_name(zbus::names::WellKnownName::try_from(bus_name.as_str())?)
            .await
            .context("Failed to request D-Bus bus name")?;

        Ok(())
    }

    pub async fn send_notification(&self, summary: &str, body: &str) -> Result<()> {
        let conn = self.connection.as_ref()
            .context("D-Bus not initialized")?;

        // Use freedesktop notifications via D-Bus
        let proxy = zbus::Proxy::new(
            conn,
            "org.freedesktop.Notifications",
            "/org/freedesktop/Notifications",
            "org.freedesktop.Notifications",
        ).await.context("Failed to create notification proxy")?;

        let _: u32 = proxy.call(
            "Notify",
            &(
                "kiptv",      // app_name
                0u32,         // replaces_id
                "kiptv",      // app_icon
                summary,      // summary
                body,         // body
                Vec::<String>::new(), // actions
                std::collections::HashMap::<String, zbus::zvariant::Value>::new(), // hints
                5000i32,      // timeout
            ),
        ).await.context("Failed to send notification")?;

        Ok(())
    }

    pub async fn play_url(&self, url: &str) -> Result<()> {
        let player = self.player.lock().await;
        player.play_url(url)?;
        
        // Extract channel name from URL for notification
        let channel_name = url.split('/').last().unwrap_or(url);
        let _ = self.send_notification(
            &format!("Playing: {}", channel_name),
            &format!("Stream: {}", url),
        ).await;

        Ok(())
    }

    pub async fn pause(&self) -> Result<()> {
        let player = self.player.lock().await;
        player.pause()?;
        Ok(())
    }

    pub async fn stop(&self) -> Result<()> {
        let player = self.player.lock().await;
        player.stop()?;
        Ok(())
    }
}
