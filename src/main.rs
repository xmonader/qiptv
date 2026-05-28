use eframe::egui;
use egui::{CentralPanel, TopBottomPanel, SidePanel};

mod models;
mod parsers;

use models::Playlist;
use parsers::parse_playlist;

struct KiptvApp {
    playlists: Vec<Playlist>,
    current_playlist: Option<usize>,
    search_query: String,
}

impl Default for KiptvApp {
    fn default() -> Self {
        Self {
            playlists: Vec::new(),
            current_playlist: None,
            search_query: String::new(),
        }
    }
}

impl eframe::App for KiptvApp {
    fn update(&mut self, ctx: &egui::Context, _frame: &mut eframe::Frame) {
        TopBottomPanel::top("menu_bar").show(ctx, |ui| {
            egui::menu::bar(ui, |ui| {
                ui.menu_button("File", |ui| {
                    if ui.button("Open Playlist").clicked() {
                        // TODO: Integrate rfd for file dialog
                        println!("Open playlist dialog");
                    }
                    if ui.button("Quit").clicked() {
                        std::process::exit(0);
                    }
                });
                ui.menu_button("View", |ui| {
                    // TODO: Add view options
                });
            });
        });

        SidePanel::left("playlists").show(ctx, |ui| {
            ui.heading("Playlists");
            for (idx, playlist) in self.playlists.iter().enumerate() {
                if ui.selectable_label(self.current_playlist == Some(idx), &playlist.name).clicked() {
                    self.current_playlist = Some(idx);
                }
            }
        });

        SidePanel::right("channels").show(ctx, |ui| {
            ui.heading("Channels");
            ui.horizontal(|ui| {
                ui.label("Search:");
                ui.text_edit_singleline(&mut self.search_query);
            });

            if let Some(idx) = self.current_playlist {
                let playlist = &self.playlists[idx];
                let channels = if self.search_query.is_empty() {
                    playlist.channels.iter().collect::<Vec<_>>()
                } else {
                    playlist.search(&self.search_query)
                };

                for channel in channels {
                    if ui.button(&channel.name).clicked() {
                        println!("Playing: {} - {}", channel.name, channel.url);
                        // TODO: Start playback
                    }
                }
            }
        });

        CentralPanel::default().show(ctx, |ui| {
            ui.vertical_centered(|ui| {
                ui.heading("kiptv - IPTV Player");
                if self.playlists.is_empty() {
                    ui.label("Open a playlist to get started");
                } else if let Some(idx) = self.current_playlist {
                    let playlist = &self.playlists[idx];
                    ui.label(format!("{} channels loaded", playlist.channels.len()));
                }
            });
        });
    }
}

fn main() -> eframe::Result<()> {
    let options = eframe::NativeOptions {
        viewport: egui::ViewportBuilder::default()
            .with_inner_size([1200.0, 800.0])
            .with_title("kiptv"),
        ..Default::default()
    };

    eframe::run_native(
        "kiptv",
        options,
        Box::new(|_cc| Ok(Box::new(KiptvApp::default()))),
    )
}
