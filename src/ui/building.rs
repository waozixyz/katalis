use raylib::prelude::*;
use crate::types::*;
use crate::assets::AssetManager;
use crate::Player;
use crate::world::{World, BuildingInventory};
use super::common::*;
use super::inventory_layout::InventoryLayout;

// Keep the old BuildingUI for now for compatibility, but mark as deprecated
#[deprecated(note = "Use DualPanelUI instead")]
pub struct BuildingUI {
    pub is_open: bool,
    pub building_pos: Option<(usize, usize)>,
    pub building_type: Option<BuildingType>,
    pub dragged_item: Option<DraggedItem>,
    pub drag_offset: Vector2,
    inventory_layout: InventoryLayout,
}

impl BuildingUI {
    pub fn new() -> Self {
        Self {
            is_open: false,
            building_pos: None,
            building_type: None,
            dragged_item: None,
            drag_offset: Vector2::zero(),
            inventory_layout: InventoryLayout::new_left_panel(),
        }
    }
    
    pub fn open(&mut self, building_pos: (usize, usize), building_type: BuildingType) {
        self.is_open = true;
        self.building_pos = Some(building_pos);
        self.building_type = Some(building_type);
        self.dragged_item = None;
    }
    
    pub fn close(&mut self) {
        self.is_open = false;
        self.building_pos = None;
        self.building_type = None;
        self.dragged_item = None;
    }
    
    pub fn update(&mut self, rl: &RaylibHandle) -> bool {
        if !self.is_open {
            return false;
        }
        
        // Close on ESC
        if rl.is_key_pressed(KeyboardKey::KEY_ESCAPE) {
            self.close();
            return true; // Consumed ESC
        }
        
        false
    }
    
    pub fn handle_mouse_input(&mut self, rl: &RaylibHandle, player: &mut Player, world: &mut World) -> bool {
        if !self.is_open {
            return false;
        }
        
        let mouse_pos = rl.get_mouse_position();
        
        if rl.is_mouse_button_pressed(MouseButton::MOUSE_BUTTON_LEFT) {
            // Handle start dragging
            if self.dragged_item.is_none() {
                // Check player inventory slots
                if let Some((slot_index, slot)) = self.inventory_layout.get_slot_at_mouse(mouse_pos, player) {
                    if !slot.is_empty() {
                        self.dragged_item = Some(DraggedItem {
                            resource_type: slot.resource_type.unwrap(),
                            amount: slot.amount,
                            source_slot: slot_index,
                        });
                        self.drag_offset = Vector2::zero();
                        
                        // Clear the source slot
                        player.inventory.clear_slot(slot_index);
                        return true;
                    }
                }
                
                // Check building inventory slots
                if let Some(building_pos) = self.building_pos {
                    if let Some(inventory) = world.building_inventories.get_mut(&building_pos) {
                        // Check input slots
                        if let Some((slot_index, slot)) = self.get_building_input_slot_at_mouse(mouse_pos, inventory) {
                            if !slot.is_empty() {
                                self.dragged_item = Some(DraggedItem {
                                    resource_type: slot.resource_type.unwrap(),
                                    amount: slot.amount,
                                    source_slot: 1000 + slot_index, // Use 1000+ to indicate building slot
                                });
                                self.drag_offset = Vector2::zero();
                                
                                // Clear the source slot
                                inventory.input_slots[slot_index] = InventorySlot::new();
                                return true;
                            }
                        }
                        
                        // Check output slots
                        if let Some((slot_index, slot)) = self.get_building_output_slot_at_mouse(mouse_pos, inventory) {
                            if !slot.is_empty() {
                                self.dragged_item = Some(DraggedItem {
                                    resource_type: slot.resource_type.unwrap(),
                                    amount: slot.amount,
                                    source_slot: 2000 + slot_index, // Use 2000+ to indicate output slot
                                });
                                self.drag_offset = Vector2::zero();
                                
                                // Clear the source slot
                                inventory.output_slots[slot_index] = InventorySlot::new();
                                return true;
                            }
                        }
                    }
                }
            }
        }
        
        if rl.is_mouse_button_released(MouseButton::MOUSE_BUTTON_LEFT) {
            // Handle drop
            if let Some(dragged) = self.dragged_item.take() {
                let mut dropped = false;
                
                // Try to drop in player inventory
                if let Some((slot_index, _)) = self.inventory_layout.get_slot_at_mouse(mouse_pos, player) {
                    player.inventory.add_to_slot(slot_index, dragged.resource_type, dragged.amount);
                    dropped = true;
                }
                
                // Try to drop in building slots
                if !dropped {
                    if let Some(building_pos) = self.building_pos {
                        if let Some(inventory) = world.building_inventories.get_mut(&building_pos) {
                            // Check input slots
                            if let Some((slot_index, _)) = self.get_building_input_slot_at_mouse(mouse_pos, inventory) {
                                if inventory.input_slots[slot_index].is_empty() {
                                    inventory.input_slots[slot_index] = InventorySlot {
                                        resource_type: Some(dragged.resource_type),
                                        amount: dragged.amount,
                                    };
                                    dropped = true;
                                }
                            }
                        }
                    }
                }
                
                // If not dropped anywhere, return to source
                if !dropped {
                    if dragged.source_slot < 1000 {
                        // Player inventory
                        player.inventory.add_to_slot(dragged.source_slot, dragged.resource_type, dragged.amount);
                    } else if dragged.source_slot < 2000 {
                        // Building input slot
                        if let Some(building_pos) = self.building_pos {
                            if let Some(inventory) = world.building_inventories.get_mut(&building_pos) {
                                let slot_index = dragged.source_slot - 1000;
                                inventory.input_slots[slot_index] = InventorySlot {
                                    resource_type: Some(dragged.resource_type),
                                    amount: dragged.amount,
                                };
                            }
                        }
                    } else {
                        // Building output slot
                        if let Some(building_pos) = self.building_pos {
                            if let Some(inventory) = world.building_inventories.get_mut(&building_pos) {
                                let slot_index = dragged.source_slot - 2000;
                                inventory.output_slots[slot_index] = InventorySlot {
                                    resource_type: Some(dragged.resource_type),
                                    amount: dragged.amount,
                                };
                            }
                        }
                    }
                }
                
                self.dragged_item = None;
                return true;
            }
        }
        
        false
    }
    
    fn get_building_name(&self, building_type: BuildingType) -> &'static str {
        match building_type {
            BuildingType::CharcoalPit => "Charcoal Pit",
            BuildingType::BloomeryFurnace => "Bloomery Furnace",
            BuildingType::StoneAnvil => "Stone Anvil",
            BuildingType::SpinningWheel => "Spinning Wheel",
            BuildingType::WeavingMachine => "Weaving Machine",
            BuildingType::ConveyorBelt => "Conveyor Belt",
        }
    }
    
    fn get_player_slot_at_mouse<'a>(&self, mouse_pos: Vector2, player: &'a Player) -> Option<(usize, &'a InventorySlot)> {
        let inv_panel_x = 50;
        let inv_panel_y = 150;
        let inv_start_x = inv_panel_x + 20;
        let inv_start_y = inv_panel_y + 60;
        let slot_size = 32;
        let slot_spacing = 6;
        let slots_per_row = 8;
        
        let mouse_x = mouse_pos.x as i32;
        let mouse_y = mouse_pos.y as i32;
        
        // Check each slot (only first 16 slots shown)
        for i in 0..16.min(player.inventory.slots.len()) {
            let row = i / slots_per_row;
            let col = i % slots_per_row;
            let slot_x = inv_start_x + col as i32 * (slot_size + slot_spacing);
            let slot_y = inv_start_y + row as i32 * (slot_size + slot_spacing);
            
            if mouse_x >= slot_x && mouse_x < slot_x + slot_size &&
               mouse_y >= slot_y && mouse_y < slot_y + slot_size {
                return Some((i, &player.inventory.slots[i]));
            }
        }
        
        None
    }
    
    fn get_building_input_slot_at_mouse<'a>(&self, mouse_pos: Vector2, inventory: &'a BuildingInventory) -> Option<(usize, &'a InventorySlot)> {
        let building_panel_x = 650;
        let building_panel_y = 150;
        let input_x = building_panel_x + 50;
        let input_y = building_panel_y + 100;
        let slot_size = 48;
        let slot_spacing = 8;
        
        let mouse_x = mouse_pos.x as i32;
        let mouse_y = mouse_pos.y as i32;
        
        for (i, slot) in inventory.input_slots.iter().enumerate() {
            let slot_x = input_x + i as i32 * (slot_size + slot_spacing);
            let slot_y = input_y;
            
            if mouse_x >= slot_x && mouse_x < slot_x + slot_size &&
               mouse_y >= slot_y && mouse_y < slot_y + slot_size {
                return Some((i, slot));
            }
        }
        
        None
    }
    
    fn get_building_output_slot_at_mouse<'a>(&self, mouse_pos: Vector2, inventory: &'a BuildingInventory) -> Option<(usize, &'a InventorySlot)> {
        let building_panel_x = 650;
        let building_panel_y = 150;
        let building_panel_width = 480;
        let output_x = building_panel_x + building_panel_width - 100 - inventory.output_slots.len() as i32 * (48 + 8);
        let output_y = building_panel_y + 100;
        let slot_size = 48;
        let slot_spacing = 8;
        
        let mouse_x = mouse_pos.x as i32;
        let mouse_y = mouse_pos.y as i32;
        
        for (i, slot) in inventory.output_slots.iter().enumerate() {
            let slot_x = output_x + i as i32 * (slot_size + slot_spacing);
            let slot_y = output_y;
            
            if mouse_x >= slot_x && mouse_x < slot_x + slot_size &&
               mouse_y >= slot_y && mouse_y < slot_y + slot_size {
                return Some((i, slot));
            }
        }
        
        None
    }
    
    pub fn draw(&self, d: &mut RaylibDrawHandle, world: &World, player: &Player, assets: &AssetManager, mouse_pos: Vector2) {
        if !self.is_open {
            return;
        }
        
        let Some(building_type) = self.building_type else { return; };
        let Some(building_pos) = self.building_pos else { return; };
        let Some(inventory) = world.building_inventories.get(&building_pos) else { return; };
        
        // Semi-transparent overlay
        d.draw_rectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Color::new(0, 0, 0, 128));
        
        // Draw player inventory using layout
        self.inventory_layout.draw_inventory(d, player, assets);
        
        // Building panel (right side)
        let building_panel_width = 480;
        let building_panel_height = 500;
        let building_panel_x = 650;
        let building_panel_y = 150;
        
        draw_panel(d, building_panel_x, building_panel_y, building_panel_width, building_panel_height);
        
        // Building title
        let title = self.get_building_name(building_type);
        draw_panel_title(d, title, building_panel_x, building_panel_y, building_panel_width, 24);
        
        // Draw building interface
        self.draw_building_interface(d, assets, inventory, building_panel_x, building_panel_y, building_panel_width);
        
        // Draw dragged item
        if let Some(ref dragged) = self.dragged_item {
            let icon_size = 32;
            let drag_x = mouse_pos.x - icon_size as f32 / 2.0;
            let drag_y = mouse_pos.y - icon_size as f32 / 2.0;
            
            if let Some(texture) = assets.get_icon_texture(dragged.resource_type) {
                d.draw_texture_ex(
                    texture,
                    Vector2::new(drag_x, drag_y),
                    0.0,
                    icon_size as f32 / texture.width as f32,
                    Color::new(255, 255, 255, 200)
                );
            } else {
                let color = dragged.resource_type.get_color();
                d.draw_rectangle(drag_x as i32, drag_y as i32, icon_size, icon_size, Color::new(color.r, color.g, color.b, 200));
            }
            
            // Draw amount
            let amount_text = format!("{}", dragged.amount);
            d.draw_text(&amount_text, drag_x as i32 + icon_size - 12, drag_y as i32 + icon_size - 12, 10, Color::WHITE);
        }
    }
    
    fn draw_building_interface(&self, d: &mut RaylibDrawHandle, assets: &AssetManager, inventory: &BuildingInventory, panel_x: i32, panel_y: i32, panel_width: i32) {
        let slot_size = 48;
        let slot_spacing = 8;
        
        // Input section
        let input_x = panel_x + 50;
        let input_y = panel_y + 100;
        let input_label = "Input";
        let input_label_width = d.measure_text(input_label, 16);
        d.draw_text(input_label, input_x + (inventory.input_slots.len() as i32 * (slot_size + slot_spacing) - input_label_width) / 2, input_y - 25, 16, Color::LIGHTGRAY);
        
        // Draw input slots
        for (i, slot) in inventory.input_slots.iter().enumerate() {
            let slot_x = input_x + i as i32 * (slot_size + slot_spacing);
            self.draw_building_slot(d, assets, slot, slot_x, input_y, slot_size);
        }
        
        // Arrow and progress bar
        let arrow_x = panel_x + panel_width / 2;
        let arrow_y = input_y + slot_size / 2;
        d.draw_text("→", arrow_x - 10, arrow_y - 8, 32, Color::LIGHTGRAY);
        
        let progress_bar_width = 100;
        let progress_bar_height = 20;
        let progress_bar_x = arrow_x - progress_bar_width / 2;
        let progress_bar_y = arrow_y + 25;
        
        d.draw_rectangle(progress_bar_x, progress_bar_y, progress_bar_width, progress_bar_height, Color::new(40, 40, 40, 255));
        d.draw_rectangle_lines(progress_bar_x, progress_bar_y, progress_bar_width, progress_bar_height, Color::new(100, 100, 100, 255));
        
        if inventory.max_progress > 0.0 {
            let progress_width = ((inventory.progress / inventory.max_progress) * progress_bar_width as f32) as i32;
            d.draw_rectangle(progress_bar_x, progress_bar_y, progress_width, progress_bar_height, Color::new(100, 200, 100, 255));
        }
        
        // Output section
        let output_x = panel_x + panel_width - 100 - inventory.output_slots.len() as i32 * (slot_size + slot_spacing);
        let output_y = input_y;
        let output_label = "Output";
        let output_label_width = d.measure_text(output_label, 16);
        d.draw_text(output_label, output_x + (inventory.output_slots.len() as i32 * (slot_size + slot_spacing) - output_label_width) / 2, output_y - 25, 16, Color::LIGHTGRAY);
        
        // Draw output slots
        for (i, slot) in inventory.output_slots.iter().enumerate() {
            let slot_x = output_x + i as i32 * (slot_size + slot_spacing);
            self.draw_building_slot(d, assets, slot, slot_x, output_y, slot_size);
        }
        
        // Instructions
        let instructions = "Click and drag items to move them. Press ESC to close.";
        let inst_size = 14;
        let inst_width = d.measure_text(instructions, inst_size);
        d.draw_text(instructions, panel_x + (panel_width - inst_width) / 2, panel_y + 500 - 40, inst_size, Color::LIGHTGRAY);
    }
    
    fn draw_building_slot(&self, d: &mut RaylibDrawHandle, assets: &AssetManager, slot: &InventorySlot, slot_x: i32, slot_y: i32, slot_size: i32) {
        // Draw slot background
        d.draw_rectangle(slot_x, slot_y, slot_size, slot_size, SLOT_BACKGROUND_COLOR);
        d.draw_rectangle_lines(slot_x, slot_y, slot_size, slot_size, SLOT_BORDER_COLOR);
        
        // Draw item if present
        if !slot.is_empty() {
            if let Some(resource_type) = slot.resource_type {
                draw_item_in_slot(d, assets, resource_type, slot.amount, slot_x, slot_y, slot_size);
            }
        }
    }
}