use raylib::prelude::*;
use crate::types::*;
use crate::assets::AssetManager;

// Common UI layout constants
pub const SCREEN_WIDTH: i32 = 1200;
pub const SCREEN_HEIGHT: i32 = 800;
pub const PANEL_BACKGROUND_COLOR: Color = Color::new(40, 40, 40, 240);
pub const PANEL_BORDER_COLOR: Color = Color::new(200, 200, 200, 255);
pub const SLOT_BACKGROUND_COLOR: Color = Color::new(60, 60, 60, 255);
pub const SLOT_BORDER_COLOR: Color = Color::new(100, 100, 100, 255);

// Common UI drawing functions
pub fn draw_panel(d: &mut RaylibDrawHandle, x: i32, y: i32, width: i32, height: i32) {
    d.draw_rectangle(x, y, width, height, PANEL_BACKGROUND_COLOR);
    d.draw_rectangle_lines(x, y, width, height, PANEL_BORDER_COLOR);
}

pub fn draw_panel_title(d: &mut RaylibDrawHandle, title: &str, x: i32, y: i32, width: i32, size: i32) {
    let title_width = d.measure_text(title, size);
    d.draw_text(title, x + (width - title_width) / 2, y + 20, size, Color::WHITE);
}

pub fn draw_item_in_slot(
    d: &mut RaylibDrawHandle,
    assets: &AssetManager,
    resource_type: ResourceType,
    amount: u32,
    slot_x: i32,
    slot_y: i32,
    slot_size: i32,
) {
    let icon_size = slot_size - 4;
    
    // Draw item icon
    if let Some(texture) = assets.get_icon_texture(resource_type) {
        d.draw_texture_ex(
            texture,
            Vector2::new((slot_x + 2) as f32, (slot_y + 2) as f32),
            0.0,
            icon_size as f32 / texture.width as f32,
            Color::WHITE
        );
    } else {
        let color = resource_type.get_color();
        d.draw_rectangle(slot_x + 2, slot_y + 2, icon_size, icon_size, color);
    }
    
    // Draw amount
    let amount_text = format!("{}", amount);
    let text_size = 10;
    let text_width = d.measure_text(&amount_text, text_size) as i32;
    d.draw_rectangle(
        slot_x + slot_size - text_width - 4, 
        slot_y + slot_size - 12, 
        text_width + 2, 
        10, 
        Color::new(0, 0, 0, 180)
    );
    d.draw_text(
        &amount_text, 
        slot_x + slot_size - text_width - 3, 
        slot_y + slot_size - 11, 
        text_size, 
        Color::WHITE
    );
}

#[derive(Clone, Debug)]
pub struct DraggedItem {
    pub resource_type: ResourceType,
    pub amount: u32,
    pub source_slot: usize,
}

#[derive(Debug, Clone, Copy)]
pub enum RightPanelMode {
    Crafting,
    Building(BuildingType),
}