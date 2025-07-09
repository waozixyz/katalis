use raylib::prelude::*;

pub struct PauseMenu {
    pub is_open: bool,
}

impl PauseMenu {
    pub fn new() -> Self {
        Self {
            is_open: false,
        }
    }
    
    pub fn toggle(&mut self) {
        self.is_open = !self.is_open;
    }
    
    pub fn update(&mut self, rl: &RaylibHandle, esc_consumed_by_inventory: bool) -> bool {
        // Only handle escape if it wasn't consumed by inventory
        if !esc_consumed_by_inventory && rl.is_key_pressed(KeyboardKey::KEY_ESCAPE) {
            self.is_open = !self.is_open;
        }
        
        // Handle Q key to quit when pause menu is open
        if self.is_open && rl.is_key_pressed(KeyboardKey::KEY_Q) {
            return true; // Signal to quit the game
        }
        
        false // Don't quit
    }
    
    pub fn draw(&self, d: &mut RaylibDrawHandle) {
        if !self.is_open {
            return;
        }
        
        let screen_width = 1200;
        let screen_height = 800;
        
        // Semi-transparent overlay
        d.draw_rectangle(0, 0, screen_width, screen_height, Color::new(0, 0, 0, 128));
        
        // Menu panel
        let panel_width = 300;
        let panel_height = 200;
        let panel_x = (screen_width - panel_width) / 2;
        let panel_y = (screen_height - panel_height) / 2;
        
        d.draw_rectangle(panel_x, panel_y, panel_width, panel_height, Color::new(40, 40, 40, 240));
        d.draw_rectangle_lines(panel_x, panel_y, panel_width, panel_height, Color::new(200, 200, 200, 255));
        
        // Title
        let title = "Game Paused";
        let title_width = d.measure_text(title, 24);
        let title_x = panel_x + (panel_width - title_width) / 2;
        d.draw_text(title, title_x, panel_y + 30, 24, Color::WHITE);
        
        // Instructions
        let instructions = vec![
            "Press ESC to resume",
            "Press Q to quit game",
            "Press E to open inventory",
            "Click and drag to move items",
            "Mouse wheel to zoom",
            "WASD to move player",
        ];
        
        let mut y_offset = 80;
        for instruction in instructions {
            let text_width = d.measure_text(instruction, 14) as i32;
            let text_x = panel_x + (panel_width - text_width) / 2;
            d.draw_text(instruction, text_x, panel_y + y_offset, 14, Color::LIGHTGRAY);
            y_offset += 20;
        }
    }
}