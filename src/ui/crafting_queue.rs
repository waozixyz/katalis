use raylib::prelude::*;
use crate::crafting::*;
use crate::assets::AssetManager;
use crate::Player;
use super::common::*;

pub struct CraftingQueueUI {
    // Position and visibility settings
    x: i32,
    y: i32,
    slot_size: i32,
    slot_spacing: i32,
    slots_per_row: i32,
    max_visible_items: usize,
}

impl CraftingQueueUI {
    pub fn new() -> Self {
        Self {
            x: 20,  // Bottom left position
            y: SCREEN_HEIGHT - 80,  // Position similar to quick slots
            slot_size: 48,
            slot_spacing: 4,
            slots_per_row: 5,
            max_visible_items: 10,
        }
    }
    
    pub fn draw(&self, d: &mut RaylibDrawHandle, player: &Player, assets: &AssetManager) {
        if player.crafting_queue.is_empty() && !player.is_crafting() {
            return;
        }
        
        // Calculate how many items we have to display
        let current_item_count: usize = if player.is_crafting() { 1 } else { 0 };
        let queue_count = player.crafting_queue.len().min(self.max_visible_items - current_item_count);
        let total_items = current_item_count + queue_count;
        
        if total_items == 0 {
            return;
        }
        
        // Calculate panel dimensions based on items
        let rows = ((total_items + self.slots_per_row as usize - 1) / self.slots_per_row as usize) as i32;
        let panel_width = (self.slots_per_row * self.slot_size) + ((self.slots_per_row - 1) * self.slot_spacing) + 16;
        let panel_height = (rows * self.slot_size) + ((rows - 1) * self.slot_spacing) + 16;
        
        // Draw panel background
        d.draw_rectangle(self.x, self.y, panel_width, panel_height, PANEL_BACKGROUND_COLOR);
        d.draw_rectangle_lines(self.x, self.y, panel_width, panel_height, PANEL_BORDER_COLOR);
        
        let mut item_index = 0;
        
        // Draw current crafting item first
        if let Some(current) = player.get_current_crafting_item() {
            let row = (item_index / self.slots_per_row as usize) as i32;
            let col = (item_index % self.slots_per_row as usize) as i32;
            let slot_x = self.x + 8 + (col * (self.slot_size + self.slot_spacing));
            let slot_y = self.y + 8 + (row * (self.slot_size + self.slot_spacing));
            
            self.draw_crafting_queue_slot(d, assets, current, slot_x, slot_y, self.slot_size, Some(player.get_crafting_progress()));
            item_index += 1;
        }
        
        // Draw queued items
        for queued in player.crafting_queue.iter().take(queue_count) {
            let row = (item_index / self.slots_per_row as usize) as i32;
            let col = (item_index % self.slots_per_row as usize) as i32;
            let slot_x = self.x + 8 + (col * (self.slot_size + self.slot_spacing));
            let slot_y = self.y + 8 + (row * (self.slot_size + self.slot_spacing));
            
            self.draw_crafting_queue_slot(d, assets, &queued.item, slot_x, slot_y, self.slot_size, None);
            
            // Draw quantity indicator if more than 1
            if queued.quantity > 1 {
                let quantity_text = format!("{}", queued.quantity);
                let text_size = 10;
                let text_width = d.measure_text(&quantity_text, text_size) as i32;
                
                // Draw background for quantity
                d.draw_rectangle(
                    slot_x + self.slot_size - text_width - 4,
                    slot_y + self.slot_size - 12,
                    text_width + 2,
                    10,
                    Color::new(0, 0, 0, 180)
                );
                
                // Draw quantity text
                d.draw_text(
                    &quantity_text,
                    slot_x + self.slot_size - text_width - 3,
                    slot_y + self.slot_size - 11,
                    text_size,
                    Color::WHITE
                );
            }
            
            item_index += 1;
        }
    }
    
    fn draw_crafting_queue_slot(&self, d: &mut RaylibDrawHandle, assets: &AssetManager, item: &CraftableItem, x: i32, y: i32, size: i32, progress: Option<f32>) {
        // Draw slot background using crafting slot texture
        if let Some(slot_texture) = assets.get_ui_texture("crafting_slot") {
            d.draw_texture_ex(
                slot_texture,
                Vector2::new(x as f32, y as f32),
                0.0,
                size as f32 / slot_texture.width as f32,
                Color::WHITE
            );
        } else {
            // Fallback to rectangle
            d.draw_rectangle(x, y, size, size, SLOT_BACKGROUND_COLOR);
            d.draw_rectangle_lines(x, y, size, size, SLOT_BORDER_COLOR);
        }
        
        // Draw item icon
        if let Some(icon_texture) = assets.get_crafting_icon(*item) {
            let icon_size = size - 6;
            d.draw_texture_ex(
                icon_texture,
                Vector2::new((x + 3) as f32, (y + 3) as f32),
                0.0,
                icon_size as f32 / icon_texture.width as f32,
                Color::WHITE
            );
        } else {
            // Fallback to colored square based on category
            let icon_color = match item.get_category() {
                CraftingCategory::Tools => Color::MAROON,
                CraftingCategory::Materials => Color::BROWN,
                CraftingCategory::Metals => Color::LIGHTGRAY,
                CraftingCategory::Textiles => Color::BEIGE,
                CraftingCategory::Food => Color::new(255, 215, 0, 255),
                CraftingCategory::Power => Color::new(128, 128, 128, 255),
                CraftingCategory::Buildings => Color::DARKGRAY,
                CraftingCategory::Automation => Color::BLUE,
                CraftingCategory::Consumables => Color::PINK,
            };
            d.draw_rectangle(x + 3, y + 3, size - 6, size - 6, icon_color);
        }
        
        // Draw progress bar overlay if provided
        if let Some(progress_value) = progress {
            let progress_height = 4;
            let progress_width = size - 6;
            let progress_x = x + 3;
            let progress_y = y + size - progress_height - 3;
            
            // Draw progress bar background
            d.draw_rectangle(progress_x, progress_y, progress_width, progress_height, Color::new(0, 0, 0, 128));
            
            // Draw progress bar fill
            let fill_width = (progress_width as f32 * progress_value) as i32;
            d.draw_rectangle(progress_x, progress_y, fill_width, progress_height, Color::new(0, 255, 0, 200));
            
            // Draw progress bar border
            d.draw_rectangle_lines(progress_x, progress_y, progress_width, progress_height, Color::new(255, 255, 255, 128));
        }
    }
}