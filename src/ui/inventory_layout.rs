use raylib::prelude::*;
use crate::types::*;
use crate::assets::AssetManager;
use crate::Player;
use super::common::*;

pub struct InventoryLayout {
    panel_x: i32,
    panel_y: i32,
    panel_width: i32,
    panel_height: i32,
    start_x: i32,
    start_y: i32,
    slot_size: i32,
    slot_spacing: i32,
    slots_per_row: usize,
    max_slots: usize,
}

impl InventoryLayout {
    pub fn new_left_panel() -> Self {
        let panel_x = 50;
        let panel_y = 100;
        let panel_width = 500;
        let panel_height = 600;
        
        // Take about 1/4 of the panel height from the top for inventory
        let inventory_height = panel_height / 4;
        
        // Calculate slot size and spacing to fit nicely in the available space
        let horizontal_padding = 20;
        let vertical_padding = 60; // Space for title
        let available_width = panel_width - (horizontal_padding * 2);
        let _available_height = inventory_height - vertical_padding - 20; // Extra bottom margin
        
        // Calculate optimal slot size and spacing
        let slots_per_row = 10; // Increase from 8 to 10 for better use of width
        let slot_spacing = 4; // Reduce spacing slightly
        let total_spacing_width = (slots_per_row - 1) * slot_spacing;
        let slot_size = (available_width - total_spacing_width) / slots_per_row;
        
        // Default to 3 rows for inventory display
        let rows_to_display = 3;
        let max_slots = (slots_per_row * rows_to_display) as usize;
        
        Self {
            panel_x,
            panel_y,
            panel_width,
            panel_height,
            start_x: panel_x + horizontal_padding,
            start_y: panel_y + vertical_padding,
            slot_size,
            slot_spacing,
            slots_per_row: slots_per_row as usize,
            max_slots,
        }
    }
    
    pub fn draw_inventory(&self, d: &mut RaylibDrawHandle, player: &Player, assets: &AssetManager) {
        draw_panel(d, self.panel_x, self.panel_y, self.panel_width, self.panel_height);
        draw_panel_title(d, "Player Inventory", self.panel_x, self.panel_y, self.panel_width, 20);
        
        // Draw inventory slots
        self.draw_inventory_slots(d, player, assets);
    }
    
    pub fn draw_inventory_with_tooltips(&self, d: &mut RaylibDrawHandle, player: &Player, assets: &AssetManager, mouse_pos: Vector2) {
        draw_panel(d, self.panel_x, self.panel_y, self.panel_width, self.panel_height);
        draw_panel_title(d, "Player Inventory", self.panel_x, self.panel_y, self.panel_width, 20);
        
        let mut hovered_item: Option<ResourceType> = None;
        
        // Draw inventory slots and detect hover
        for i in 0..self.max_slots.min(player.inventory.slots.len()) {
            let row = i / self.slots_per_row;
            let col = i % self.slots_per_row;
            let slot_x = self.start_x + col as i32 * (self.slot_size + self.slot_spacing);
            let slot_y = self.start_y + row as i32 * (self.slot_size + self.slot_spacing);
            
            // Check if mouse is over this slot
            let is_hovered = mouse_pos.x >= slot_x as f32 && mouse_pos.x < (slot_x + self.slot_size) as f32 &&
                           mouse_pos.y >= slot_y as f32 && mouse_pos.y < (slot_y + self.slot_size) as f32;
            
            // Draw slot background
            if let Some(slot_texture) = assets.get_ui_texture("inventory_slot") {
                d.draw_texture_ex(
                    slot_texture,
                    Vector2::new(slot_x as f32, slot_y as f32),
                    0.0,
                    self.slot_size as f32 / slot_texture.width as f32,
                    Color::WHITE
                );
            } else {
                d.draw_rectangle(slot_x, slot_y, self.slot_size, self.slot_size, SLOT_BACKGROUND_COLOR);
                d.draw_rectangle_lines(slot_x, slot_y, self.slot_size, self.slot_size, SLOT_BORDER_COLOR);
            }
            
            // Draw item if present
            if let Some(slot) = player.inventory.get_slot(i) {
                if !slot.is_empty() {
                    if let Some(resource_type) = slot.resource_type {
                        draw_item_in_slot(d, assets, resource_type, slot.amount, slot_x, slot_y, self.slot_size);
                        
                        // Store hovered item for tooltip
                        if is_hovered {
                            hovered_item = Some(resource_type);
                        }
                    }
                }
            }
        }
        
        // Draw tooltip if hovering over an item
        if let Some(resource_type) = hovered_item {
            self.draw_inventory_tooltip(d, resource_type, mouse_pos);
        }
    }
    
    fn draw_inventory_slots(&self, d: &mut RaylibDrawHandle, player: &Player, assets: &AssetManager) {
        for i in 0..self.max_slots.min(player.inventory.slots.len()) {
            let row = i / self.slots_per_row;
            let col = i % self.slots_per_row;
            let slot_x = self.start_x + col as i32 * (self.slot_size + self.slot_spacing);
            let slot_y = self.start_y + row as i32 * (self.slot_size + self.slot_spacing);
            
            // Draw slot background
            if let Some(slot_texture) = assets.get_ui_texture("inventory_slot") {
                d.draw_texture_ex(
                    slot_texture,
                    Vector2::new(slot_x as f32, slot_y as f32),
                    0.0,
                    self.slot_size as f32 / slot_texture.width as f32,
                    Color::WHITE
                );
            } else {
                d.draw_rectangle(slot_x, slot_y, self.slot_size, self.slot_size, SLOT_BACKGROUND_COLOR);
                d.draw_rectangle_lines(slot_x, slot_y, self.slot_size, self.slot_size, SLOT_BORDER_COLOR);
            }
            
            // Draw item if present
            if let Some(slot) = player.inventory.get_slot(i) {
                if !slot.is_empty() {
                    if let Some(resource_type) = slot.resource_type {
                        draw_item_in_slot(d, assets, resource_type, slot.amount, slot_x, slot_y, self.slot_size);
                    }
                }
            }
        }
    }
    
    fn draw_inventory_tooltip(&self, d: &mut RaylibDrawHandle, resource_type: ResourceType, mouse_pos: Vector2) {
        let tooltip_width = 200;
        let tooltip_height = 60;
        let padding = 8;
        
        // Position tooltip near mouse but keep it on screen
        let mut tooltip_x = mouse_pos.x as i32 + 15;
        let mut tooltip_y = mouse_pos.y as i32 - tooltip_height / 2;
        
        // Keep tooltip on screen
        if tooltip_x + tooltip_width > SCREEN_WIDTH { 
            tooltip_x = mouse_pos.x as i32 - tooltip_width - 15; 
        }
        if tooltip_y < 0 { 
            tooltip_y = 0; 
        }
        if tooltip_y + tooltip_height > SCREEN_HEIGHT { 
            tooltip_y = SCREEN_HEIGHT - tooltip_height; 
        }
        
        // Draw tooltip background
        d.draw_rectangle(tooltip_x, tooltip_y, tooltip_width, tooltip_height, Color::new(25, 25, 25, 240));
        d.draw_rectangle_lines(tooltip_x, tooltip_y, tooltip_width, tooltip_height, Color::new(150, 150, 150, 255));
        
        // Draw item name
        d.draw_text(resource_type.get_name(), tooltip_x + padding, tooltip_y + padding, 14, Color::WHITE);
        
        // Draw stack size info
        let stack_info = format!("Max stack: {}", resource_type.get_max_stack_size());
        d.draw_text(&stack_info, tooltip_x + padding, tooltip_y + padding + 18, 12, Color::LIGHTGRAY);
        
        // Draw building indicator if it's a building
        if resource_type.is_building() {
            d.draw_text("Click to place", tooltip_x + padding, tooltip_y + padding + 32, 10, Color::YELLOW);
        }
    }
    
    pub fn get_slot_at_mouse<'a>(&self, mouse_pos: Vector2, player: &'a Player) -> Option<(usize, &'a InventorySlot)> {
        let mouse_x = mouse_pos.x as i32;
        let mouse_y = mouse_pos.y as i32;
        
        for i in 0..self.max_slots.min(player.inventory.slots.len()) {
            let row = i / self.slots_per_row;
            let col = i % self.slots_per_row;
            let slot_x = self.start_x + col as i32 * (self.slot_size + self.slot_spacing);
            let slot_y = self.start_y + row as i32 * (self.slot_size + self.slot_spacing);
            
            if mouse_x >= slot_x && mouse_x < slot_x + self.slot_size &&
               mouse_y >= slot_y && mouse_y < slot_y + self.slot_size {
                return Some((i, &player.inventory.slots[i]));
            }
        }
        
        None
    }
}