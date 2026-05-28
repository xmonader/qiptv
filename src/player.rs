use anyhow::{Context, Result};
use gstreamer as gst;
use gstreamer::prelude::*;

pub struct Player {
    pipeline: gst::Pipeline,
}

impl Player {
    pub fn new() -> Result<Self> {
        gst::init().context("Failed to initialize GStreamer")?;

        let pipeline = gst::Pipeline::new();
        
        let playbin = gst::ElementFactory::make("playbin3")
            .build()
            .context("Failed to create playbin3")?;

        pipeline.add(&playbin).context("Failed to add playbin to pipeline")?;

        Ok(Self { pipeline })
    }

    pub fn play_url(&self, url: &str) -> Result<()> {
        let playbin = self.pipeline.by_name("playbin3")
            .or_else(|| self.pipeline.children().into_iter().next())
            .context("Playbin not found")?;

        playbin.set_property("uri", &url);
        self.pipeline.set_state(gst::State::Playing)
            .context("Failed to set playing state")?;
        Ok(())
    }

    pub fn pause(&self) -> Result<()> {
        self.pipeline.set_state(gst::State::Paused)
            .context("Failed to set paused state")?;
        Ok(())
    }

    pub fn stop(&self) -> Result<()> {
        self.pipeline.set_state(gst::State::Null)
            .context("Failed to set null state")?;
        Ok(())
    }

    pub fn set_volume(&self, volume: f64) {
        if let Some(playbin) = self.pipeline.children().into_iter().next() {
            let _ = playbin.set_property("volume", &volume);
        }
    }

    pub fn volume(&self) -> f64 {
        if let Some(playbin) = self.pipeline.children().into_iter().next() {
            playbin.property::<f64>("volume").unwrap_or(1.0)
        } else {
            1.0
        }
    }
}

impl Drop for Player {
    fn drop(&mut self) {
        let _ = self.stop();
    }
}
