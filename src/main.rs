use eframe::egui;
use egui::{CentralPanel, TopBottomPanel, SidePanel};

struct KiptvApp {
    playlists: Vec<String>,
    current_playlist: Option<String>,
    channels: Vec<Channel>,
}

impl Default for KiptvApp {
    fn default() -> Self {
        Self {
            playlists: Vec::new(),
            current_playlist: None,
            channels: Vec::new(),
        }
    }
}

impl eframe::App for KiptvApp {
    fn update(&mut self, ctx: &egui::Context, _frame: &mut eframe::Frame) {
        TopBottomPanel::top("menu_bar").show(ctx, |ui| {
            egui::menu::bar(ui, |ui| {
                ui.menu_button("File", |ui| {
                    if ui.button("Open Playlist").clicked() {
                        // TODO: Open file dialog
                        println!("Open playlist");
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
            for playlist in &self.playlists {
                if ui.selectable_label(self.current_playlist.as_ref() == Some(playlist), playlist).clicked() {
                    self.current_playlist = Some(playlist.clone());
                }
            }
        });

        SidePanel::right("channels").show(ctx, |ui| {
            ui.heading("Channels");
            // TODO: Channel list
        });

        CentralPanel::default().show(ctx, |ui| {
            ui.vertical_centered(|ui| {
                ui.heading("kiptv - IPTV Player");
                ui.label("Open a playlist to get started");
            });
        });
    }
}

#[derive(Clone)]
struct Channel {
    name: String,
    url: String,
    group: String,
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
