use eframe::egui;
use egui::{CentralPanel, TopBottomPanel, SidePanel, ScrollArea};
use rfd::FileDialog;

mod models;
mod parsers;

use models::Playlist;
use parsers::parse_playlist;

struct KiptvApp {
    playlists: Vec<Playlist>,
    current_playlist: Option<usize>,
    search_query: String,
    selected_channel: Option<String>,
}

impl Default for KiptvApp {
    fn default() -> Self {
        Self {
            playlists: Vec::new(),
            current_playlist: None,
            search_query: String::new(),
            selected_channel: None,
        }
    }
}

impl eframe::App for KiptvApp {
    fn update(&mut self, ctx: &egui::Context, _frame: &mut eframe::Frame) {
        TopBottomPanel::top("menu_bar").show(ctx, |ui| {
            egui::menu::bar(ui, |ui| {
                ui.menu_button("File", |ui| {
                    if ui.button("Open Playlist").clicked() {
                        if let Some(file) = FileDialog::new()
                            .add_filter("Playlists", &["m3u", "m3u8", "xspf"])
                            .pick_file()
                        {
                            match parse_playlist(file.to_string_lossy().as_ref()) {
                                Ok(playlist) => {
                                    println!("Loaded: {} channels", playlist.channels.len());
                                    self.playlists.push(playlist);
                                }
                                Err(e) => {
                                    eprintln!("Failed to load playlist: {}", e);
                                }
                            }
                        }
                        ui.close_menu();
                    }
                    ui.separator();
                    if ui.button("Quit").clicked() {
                        std::process::exit(0);
                    }
                });
            });
        });

        SidePanel::left("playlists").width_range(150.0..=250.0).show(ctx, |ui| {
            ui.heading("Playlists");
            ui.separator();
            for (idx, playlist) in self.playlists.iter().enumerate() {
                let response = ui.selectable_label(self.current_playlist == Some(idx), &playlist.name);
                if response.clicked() {
                    self.current_playlist = Some(idx);
                    self.search_query.clear();
                }
            }
        });

        SidePanel::right("channels").width_range(200.0..=350.0).show(ctx, |ui| {
            ui.heading("Channels");
            ui.separator();
            
            ui.horizontal(|ui| {
                ui.label("🔍");
                ui.text_edit_singleline(&mut self.search_query);
            });

            if let Some(idx) = self.current_playlist {
                let playlist = &self.playlists[idx];
                let channels: Vec<_> = if self.search_query.is_empty() {
                    playlist.channels.iter().collect()
                } else {
                    playlist.search(&self.search_query)
                };

                ui.label(format!("{} channels", channels.len()));
                ui.separator();

                ScrollArea::vertical().show(ui, |ui| {
                    for channel in &channels {
                        let response = ui.selectable_label(
                            self.selected_channel.as_ref() == Some(&channel.url),
                            &channel.name,
                        );
                        if response.clicked() {
                            self.selected_channel = Some(channel.url.clone());
                            println!("Playing: {} - {}", channel.name, channel.url);
                        }
                    }
                });
            } else {
                ui.label("No playlist loaded");
            }
        });

        CentralPanel::default().show(ctx, |ui| {
            ui.vertical_centered(|ui| {
                ui.heading("kiptv - IPTV Player");
                if self.playlists.is_empty() {
                    ui.label("File → Open Playlist to get started");
                } else if let Some(idx) = self.current_playlist {
                    let playlist = &self.playlists[idx];
                    ui.label(format!("Playlist: {}", playlist.name));
                    ui.label(format!("{} channels loaded", playlist.channels.len()));
                    
                    if let Some(ref url) = self.selected_channel {
                        ui.separator();
                        ui.label(format!("Selected: {}", url));
                    }
                } else {
                    ui.label("Select a playlist from the left panel");
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
