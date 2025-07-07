use raylib::prelude::*;
use crate::types::*;
use crate::crafting::*;
use crate::assets::AssetManager;
use crate::Player;
use crate::world::{World, BuildingInventory};

// Common UI layout constants
const SCREEN_WIDTH: i32 = 1200;
const SCREEN_HEIGHT: i32 = 800;
const PANEL_BACKGROUND_COLOR: Color = Color::new(40, 40, 40, 240);
const PANEL_BORDER_COLOR: Color = Color::new(200, 200, 200, 255);
const SLOT_BACKGROUND_COLOR: Color = Color::new(60, 60, 60, 255);
const SLOT_BORDER_COLOR: Color = Color::new(100, 100, 100, 255);

// Common UI drawing functions
fn draw_panel(d: &mut RaylibDrawHandle, x: i32, y: i32, width: i32, height: i32) {
    d.draw_rectangle(x, y, width, height, PANEL_BACKGROUND_COLOR);
    d.draw_rectangle_lines(x, y, width, height, PANEL_BORDER_COLOR);
}

fn draw_panel_title(d: &mut RaylibDrawHandle, title: &str, x: i32, y: i32, width: i32, size: i32) {
    let title_width = d.measure_text(title, size);
    d.draw_text(title, x + (width - title_width) / 2, y + 20, size, Color::WHITE);
}

fn draw_inventory_slots(
    d: &mut RaylibDrawHandle,
    player: &Player,
    assets: &AssetManager,
    start_x: i32,
    start_y: i32,
    slot_size: i32,
    slot_spacing: i32,
    slots_per_row: usize,
    max_slots: usize,
) {
    for i in 0..max_slots.min(player.inventory.slots.len()) {
        let row = i / slots_per_row;
        let col = i % slots_per_row;
        let slot_x = start_x + col as i32 * (slot_size + slot_spacing);
        let slot_y = start_y + row as i32 * (slot_size + slot_spacing);
        
        // Draw slot background
        if let Some(slot_texture) = assets.get_ui_texture("inventory_slot") {
            d.draw_texture_ex(
                slot_texture,
                Vector2::new(slot_x as f32, slot_y as f32),
                0.0,
                slot_size as f32 / slot_texture.width as f32,
                Color::WHITE
            );
        } else {
            d.draw_rectangle(slot_x, slot_y, slot_size, slot_size, SLOT_BACKGROUND_COLOR);
            d.draw_rectangle_lines(slot_x, slot_y, slot_size, slot_size, SLOT_BORDER_COLOR);
        }
        
        // Draw item if present
        if let Some(slot) = player.inventory.get_slot(i) {
            if !slot.is_empty() {
                if let Some(resource_type) = slot.resource_type {
                    draw_item_in_slot(d, assets, resource_type, slot.amount, slot_x, slot_y, slot_size);
                }
            }
        }
    }
}

fn draw_item_in_slot(
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
    let text_width = d.measure_text(&amount_text, text_size);
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

struct InventoryLayout {
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

#[derive(Debug, Clone, Copy)]
pub enum RightPanelMode {
    Crafting,
    Building(BuildingType),
}

impl InventoryLayout {
    fn new_left_panel() -> Self {
        let panel_x = 50;
        let panel_y = 100;
        let panel_width = 500;
        let panel_height = 600;
        
        Self {
            panel_x,
            panel_y,
            panel_width,
            panel_height,
            start_x: panel_x + 20,
            start_y: panel_y + 60,
            slot_size: 32,
            slot_spacing: 6,
            slots_per_row: 8,
            max_slots: 48,
        }
    }
    
    fn draw_inventory(&self, d: &mut RaylibDrawHandle, player: &Player, assets: &AssetManager) {
        draw_panel(d, self.panel_x, self.panel_y, self.panel_width, self.panel_height);
        draw_panel_title(d, "Player Inventory", self.panel_x, self.panel_y, self.panel_width, 20);
        draw_inventory_slots(
            d, player, assets, 
            self.start_x, self.start_y, 
            self.slot_size, self.slot_spacing, 
            self.slots_per_row, self.max_slots
        );
    }
    
    fn get_slot_at_mouse<'a>(&self, mouse_pos: Vector2, player: &'a Player) -> Option<(usize, &'a InventorySlot)> {
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
            let text_width = d.measure_text(instruction, 14);
            let text_x = panel_x + (panel_width - text_width) / 2;
            d.draw_text(instruction, text_x, panel_y + y_offset, 14, Color::LIGHTGRAY);
            y_offset += 20;
        }
    }
}

#[derive(Clone, Debug)]
pub struct DraggedItem {
    pub resource_type: ResourceType,
    pub amount: u32,
    pub source_slot: usize,
}

pub struct InventoryUI {
    pub is_open: bool,
    pub selected_recipe: Option<CraftableItem>,
    pub selected_category: CraftingCategory,
    pub dragged_item: Option<DraggedItem>,
    pub drag_offset: Vector2,
    // Centralized slot configuration
    pub slot_size: i32,
    pub slot_spacing_x: i32,
    pub slot_spacing_y: i32,
    // NEW: Right panel mode and building support
    pub right_panel_mode: RightPanelMode,
    pub building_pos: Option<(usize, usize)>,
    inventory_layout: InventoryLayout,
}

impl InventoryUI {
    pub fn new() -> Self {
        Self {
            is_open: false,
            selected_recipe: None,
            selected_category: CraftingCategory::BasicMaterials,
            dragged_item: None,
            drag_offset: Vector2::zero(),
            slot_size: 32,
            slot_spacing_x: 6,
            slot_spacing_y: 8,
            right_panel_mode: RightPanelMode::Crafting,
            building_pos: None,
            inventory_layout: InventoryLayout::new_left_panel(),
        }
    }
    
    pub fn toggle(&mut self) {
        self.is_open = !self.is_open;
        // Clear drag state when closing inventory
        if !self.is_open {
            self.dragged_item = None;
        }
    }
    
    pub fn open_crafting(&mut self) {
        self.is_open = true;
        self.right_panel_mode = RightPanelMode::Crafting;
        self.building_pos = None;
        self.dragged_item = None;
    }
    
    pub fn open_building(&mut self, building_pos: (usize, usize), building_type: BuildingType) {
        self.is_open = true;
        self.right_panel_mode = RightPanelMode::Building(building_type);
        self.building_pos = Some(building_pos);
        self.dragged_item = None;
    }
    
    pub fn close(&mut self) {
        self.is_open = false;
        self.dragged_item = None;
    }
    
    pub fn update(&mut self, rl: &RaylibHandle) -> bool {
        if !self.is_open {
            return false;
        }
        
        // ESC closes the UI
        if rl.is_key_pressed(KeyboardKey::KEY_ESCAPE) {
            self.close();
            return true; // Consumed ESC
        }
        
        false
    }
    pub fn handle_mouse_input(&mut self, rl: &RaylibHandle, player: &mut Player, crafting_system: &CraftingSystem) -> Option<crate::PlacementState> {
        if !self.is_open {
            return None;
        }
        
        let mouse_pos = rl.get_mouse_position();
        
        if rl.is_mouse_button_pressed(MouseButton::MOUSE_BUTTON_LEFT) {
            // Handle inventory slot clicks
            if let Some((slot_index, slot)) = self.inventory_layout.get_slot_at_mouse(mouse_pos, player) {
                // slot is already available from get_slot_at_mouse
                if !slot.is_empty() {
                    let resource_type = slot.resource_type.unwrap();
                    
                    // Check if this is a building item
                    if let Some(building_type) = BuildingType::from_resource_type(&resource_type) {
                        // Start placement mode
                        self.is_open = false; // Close inventory
                        return Some(crate::PlacementState {
                            building_type,
                            resource_type,
                            source_slot: slot_index,
                        });
                    } else {
                        // Start dragging for non-building items
                        self.dragged_item = Some(DraggedItem {
                            resource_type,
                            amount: slot.amount,
                            source_slot: slot_index,
                        });
                        self.drag_offset = Vector2::zero();
                    }
                }
            }
            
            // Handle crafting panel tabs only when in crafting mode
            if matches!(self.right_panel_mode, RightPanelMode::Crafting) {
                let panel_x = 650;
                let panel_y = 100;
            let tab_y = panel_y + 50;
            let tab_height = 35;
            let panel_width = 480;
            let tabs_per_row = 3;
            let tab_width = panel_width / tabs_per_row as i32;
            
            let categories = [
                CraftingCategory::BasicMaterials,
                CraftingCategory::Metallurgy,
                CraftingCategory::Textiles,
                CraftingCategory::Structures,
                CraftingCategory::Automation,
            ];
            
            for (i, category) in categories.iter().enumerate() {
                let row = i / tabs_per_row;
                let col = i % tabs_per_row;
                let tab_x = panel_x + (col as i32 * tab_width);
                let current_tab_y = tab_y + (row as i32 * tab_height);
                
                if mouse_pos.x >= tab_x as f32 && 
                   mouse_pos.x <= (tab_x + tab_width) as f32 &&
                   mouse_pos.y >= current_tab_y as f32 && 
                   mouse_pos.y <= (current_tab_y + tab_height) as f32 {
                    self.selected_category = *category;
                    break;
                }
            }
            if let Some(clicked_item) = self.get_crafting_item_at_mouse(mouse_pos, 650, 100) {
                if let Some(recipe) = crafting_system.recipes.get(&clicked_item) {
                    let craft_status = crafting_system.get_craft_status(&clicked_item, &player.inventory);
                    
                    // ONLY proceed if we can actually craft
                    if matches!(craft_status, CraftStatus::CanCraft) {
                        // Double-check ingredients before consuming (safety check)
                        let mut can_craft = true;
                        for (resource_type, required_amount) in &recipe.inputs {
                            if player.inventory.get_amount(resource_type) < *required_amount {
                                can_craft = false;
                                println!("Not enough {}: need {}, have {}", 
                                         resource_type.get_name(), 
                                         required_amount, 
                                         player.inventory.get_amount(resource_type));
                                break;
                            }
                        }
                        
                        if can_craft {
                            // Now consume ingredients
                            let mut successfully_consumed = true;
                            for (resource_type, required_amount) in &recipe.inputs {
                                if !player.inventory.remove_resource(*resource_type, *required_amount) {
                                    successfully_consumed = false;
                                    println!("Failed to consume {} {}", required_amount, resource_type.get_name());
                                    break;
                                }
                            }
                            
                            if successfully_consumed {
                                // Start crafting or add to queue
                                if !player.is_crafting() {
                                    player.start_crafting(clicked_item, recipe.clone());
                                    println!("Started crafting: {}", clicked_item.get_name());
                                } else {
                                    player.add_to_crafting_queue(clicked_item, recipe.output.1);
                                    println!("Added {} to crafting queue", clicked_item.get_name());
                                }
                            } else {
                                println!("Failed to consume all ingredients for crafting");
                            }
                        } else {
                            println!("Cannot craft {}: insufficient resources", clicked_item.get_name());
                        }
                    } else {
                        // Print why we can't craft
                        match craft_status {
                            CraftStatus::MissingResources(msg) => println!("Cannot craft: {}", msg),
                            CraftStatus::NeedsStructure(msg) => println!("Cannot craft: {}", msg),
                            CraftStatus::NoRecipe => println!("No recipe found for {:?}", clicked_item),
                            CraftStatus::CanCraft => {} // This shouldn't happen
                        }
                    }
                }
            }
            } // End crafting mode handling

        }
        
        if rl.is_mouse_button_released(MouseButton::MOUSE_BUTTON_LEFT) {
            if let Some(dragged) = &self.dragged_item {
                // Try to drop in a new slot
                if let Some((target_slot, _)) = self.inventory_layout.get_slot_at_mouse(mouse_pos, player) {
                    if target_slot != dragged.source_slot {
                        // Attempt to move item
                        if let Some(target) = player.inventory.get_slot(target_slot) {
                            if target.is_empty() || target.resource_type == Some(dragged.resource_type) {
                                // Remove from source
                                if let Some(source) = player.inventory.get_slot_mut(dragged.source_slot) {
                                    source.remove(dragged.amount);
                                }
                                // Add to target
                                if let Some(target) = player.inventory.get_slot_mut(target_slot) {
                                    target.add(dragged.resource_type, dragged.amount);
                                }
                            }
                        }
                    }
                }
            }
            self.dragged_item = None;
        }
        
        None
    }
    
    fn get_slot_at_mouse(&self, mouse_pos: Vector2, panel_x: i32, panel_y: i32) -> Option<usize> {
        let slots_per_row = 8i32;
        let start_x = panel_x + 20;
        let start_y = panel_y + 60;
        
        for row in 0..6i32 {
            for col in 0..slots_per_row {
                let slot_index = (row * slots_per_row + col) as usize;
                if slot_index >= 48 { break; }
                
                let slot_x = start_x + (col * (self.slot_size + self.slot_spacing_x));
                let slot_y = start_y + (row * (self.slot_size + self.slot_spacing_y));
                
                if mouse_pos.x >= slot_x as f32 && 
                   mouse_pos.x <= (slot_x + self.slot_size) as f32 &&
                   mouse_pos.y >= slot_y as f32 && 
                   mouse_pos.y <= (slot_y + self.slot_size) as f32 {
                    return Some(slot_index);
                }
            }
        }
        None
    }
    
    
    pub fn draw(&self, d: &mut RaylibDrawHandle, inventory: &Inventory, crafting_system: &CraftingSystem, assets: &AssetManager, mouse_pos: Vector2, player: &Player) {
        if !self.is_open {
            return;
        }
        
        let screen_width = 1200;
        let screen_height = 800;
        
        d.draw_rectangle(0, 0, screen_width, screen_height, Color::new(0, 0, 0, 128));
        
        // Draw left panel (always player inventory)
        self.inventory_layout.draw_inventory(d, player, assets);
        
        // Draw right panel based on mode
        let right_panel_x = 650;
        let right_panel_y = 100;
        let right_panel_width = 480;
        let right_panel_height = 600;
        
        match self.right_panel_mode {
            RightPanelMode::Crafting => {
                // Don't draw a separate panel background for crafting - the crafting panel draws its own
                self.draw_crafting_panel(d, crafting_system, inventory, assets, right_panel_x, right_panel_y, mouse_pos);
            }
            RightPanelMode::Building(building_type) => {
                draw_panel(d, right_panel_x, right_panel_y, right_panel_width, right_panel_height);
                let title = self.get_building_name(building_type);
                draw_panel_title(d, title, right_panel_x, right_panel_y, right_panel_width, 24);
                // For building mode, we need access to world - this will be handled by parameter change
            }
        }
        
        // Draw dragged item
        if let Some(dragged) = &self.dragged_item {
            let drag_pos = Vector2::new(mouse_pos.x - (self.slot_size as f32 / 2.0), mouse_pos.y - (self.slot_size as f32 / 2.0));
            
            // Draw slot background
            if let Some(slot_texture) = assets.get_ui_texture("inventory_slot") {
                d.draw_texture_ex(slot_texture, drag_pos, 0.0, self.slot_size as f32 / slot_texture.width as f32, Color::new(255, 255, 255, 200));
            } else {
                d.draw_rectangle(drag_pos.x as i32, drag_pos.y as i32, self.slot_size, self.slot_size, Color::new(60, 60, 60, 200));
            }
            
            // Draw item icon
            if let Some(texture) = assets.get_icon_texture(dragged.resource_type) {
                let icon_size = self.slot_size - 4;
                d.draw_texture_ex(texture, Vector2::new(drag_pos.x + 2.0, drag_pos.y + 2.0), 0.0, icon_size as f32 / texture.width as f32, Color::new(255, 255, 255, 200));
            } else {
                let color = dragged.resource_type.get_color();
                let icon_size = self.slot_size - 4;
                d.draw_rectangle(drag_pos.x as i32 + 2, drag_pos.y as i32 + 2, icon_size, icon_size, Color::new(color.r, color.g, color.b, 200));
            }
            
            // Draw amount
            let amount_text = format!("{}", dragged.amount);
            let font_size = 10; // Smaller font for smaller slots
            d.draw_text(&amount_text, drag_pos.x as i32 + self.slot_size - 12, drag_pos.y as i32 + self.slot_size - 12, font_size, Color::WHITE);
        }

        self.draw_crafting_queue(d, player, 50, 500);

    }
    
    // Method to draw when in building mode - will be called from main with world parameter
    pub fn draw_with_world(&self, d: &mut RaylibDrawHandle, world: &World, inventory: &Inventory, crafting_system: &CraftingSystem, assets: &AssetManager, mouse_pos: Vector2, player: &Player) {
        if !self.is_open {
            return;
        }
        
        let screen_width = 1200;
        let screen_height = 800;
        
        d.draw_rectangle(0, 0, screen_width, screen_height, Color::new(0, 0, 0, 128));
        
        // Draw left panel (always player inventory)
        self.inventory_layout.draw_inventory(d, player, assets);
        
        // Draw right panel based on mode
        let right_panel_x = 650;
        let right_panel_y = 100;
        let right_panel_width = 480;
        let right_panel_height = 600;
        
        match self.right_panel_mode {
            RightPanelMode::Crafting => {
                // Don't draw a separate panel background for crafting - the crafting panel draws its own
                self.draw_crafting_panel(d, crafting_system, inventory, assets, right_panel_x, right_panel_y, mouse_pos);
            }
            RightPanelMode::Building(building_type) => {
                draw_panel(d, right_panel_x, right_panel_y, right_panel_width, right_panel_height);
                let title = self.get_building_name(building_type);
                draw_panel_title(d, title, right_panel_x, right_panel_y, right_panel_width, 24);
                
                if let Some(building_pos) = self.building_pos {
                    if let Some(building_inventory) = world.building_inventories.get(&building_pos) {
                        self.draw_building_interface(d, assets, building_inventory, right_panel_x, right_panel_y, right_panel_width);
                    }
                }
            }
        }
        
        // Draw dragged item
        if let Some(dragged) = &self.dragged_item {
            let drag_pos = Vector2::new(mouse_pos.x - (self.slot_size as f32 / 2.0), mouse_pos.y - (self.slot_size as f32 / 2.0));
            
            // Draw slot background
            if let Some(slot_texture) = assets.get_ui_texture("inventory_slot") {
                d.draw_texture_ex(slot_texture, drag_pos, 0.0, self.slot_size as f32 / slot_texture.width as f32, Color::new(255, 255, 255, 200));
            } else {
                d.draw_rectangle(drag_pos.x as i32, drag_pos.y as i32, self.slot_size, self.slot_size, Color::new(60, 60, 60, 200));
            }
            
            // Draw item icon
            if let Some(texture) = assets.get_icon_texture(dragged.resource_type) {
                let icon_size = self.slot_size - 4;
                d.draw_texture_ex(texture, Vector2::new(drag_pos.x + 2.0, drag_pos.y + 2.0), 0.0, icon_size as f32 / texture.width as f32, Color::new(255, 255, 255, 200));
            } else {
                let color = dragged.resource_type.get_color();
                let icon_size = self.slot_size - 4;
                d.draw_rectangle(drag_pos.x as i32 + 2, drag_pos.y as i32 + 2, icon_size, icon_size, Color::new(color.r, color.g, color.b, 200));
            }
            
            // Draw amount
            let amount_text = format!("{}", dragged.amount);
            let font_size = 10; // Smaller font for smaller slots
            d.draw_text(&amount_text, drag_pos.x as i32 + self.slot_size - 12, drag_pos.y as i32 + self.slot_size - 12, font_size, Color::WHITE);
        }

        self.draw_crafting_queue(d, player, 50, 500);

    }
    

    fn draw_crafting_queue(&self, d: &mut RaylibDrawHandle, player: &Player, x: i32, y: i32) {
        if player.crafting_queue.is_empty() && !player.is_crafting() {
            return;
        }
        
        let panel_width = 200;
        let panel_height = 100;
        
        d.draw_rectangle(x, y, panel_width, panel_height, Color::new(40, 40, 40, 240));
        d.draw_rectangle_lines(x, y, panel_width, panel_height, Color::new(200, 200, 200, 255));
        
        d.draw_text("Crafting Queue", x + 10, y + 10, 16, Color::WHITE);
        
        let mut line_y = y + 35;
        
        // Show current crafting
        if let Some(current) = player.get_current_crafting_item() {
            let progress = (player.get_crafting_progress() * 100.0) as i32;
            d.draw_text(&format!("Crafting: {} ({}%)", current.get_name(), progress), x + 10, line_y, 12, Color::GREEN);
            line_y += 15;
        }
        
        // Show queue
        for queued in &player.crafting_queue {
            d.draw_text(&format!("Queued: {} x{}", queued.item.get_name(), queued.quantity), x + 10, line_y, 12, Color::LIGHTGRAY);
            line_y += 15;
        }
    }


    fn draw_inventory_panel(&self, d: &mut RaylibDrawHandle, inventory: &Inventory, assets: &AssetManager, x: i32, y: i32) {
        let panel_width = 500;
        let panel_height = 600;
        
        d.draw_rectangle(x, y, panel_width, panel_height, Color::new(40, 40, 40, 240));
        d.draw_rectangle_lines(x, y, panel_width, panel_height, Color::new(200, 200, 200, 255));
        
        d.draw_text("Inventory", x + 20, y + 20, 24, Color::WHITE);
        
        let slots_per_row = 8i32;
        let start_x = x + 20;
        let start_y = y + 60;
        
        // Draw all 48 slots (6 rows x 8 columns)
        for row in 0..6i32 {
            for col in 0..slots_per_row {
                let slot_index = (row * slots_per_row + col) as usize;
                if slot_index >= inventory.slot_count { break; }
                
                let slot_x = start_x + (col * (self.slot_size + self.slot_spacing_x));
                let slot_y = start_y + (row * (self.slot_size + self.slot_spacing_y));
                
                // Skip drawing if this slot is being dragged
                let is_being_dragged = self.dragged_item.as_ref()
                    .map(|d| d.source_slot == slot_index)
                    .unwrap_or(false);
                
                // Draw slot background
                if let Some(slot_texture) = assets.get_ui_texture("inventory_slot") {
                    let alpha = if is_being_dragged { 100 } else { 255 };
                    d.draw_texture_ex(
                        slot_texture,
                        Vector2::new(slot_x as f32, slot_y as f32),
                        0.0,
                        self.slot_size as f32 / slot_texture.width as f32,
                        Color::new(255, 255, 255, alpha)
                    );
                } else {
                    let alpha = if is_being_dragged { 100 } else { 255 };
                    d.draw_rectangle(slot_x, slot_y, self.slot_size, self.slot_size, Color::new(60, 60, 60, alpha));
                    d.draw_rectangle_lines(slot_x, slot_y, self.slot_size, self.slot_size, Color::new(100, 100, 100, alpha));
                }
                
                // Draw item if slot has one and it's not being dragged
                if let Some(slot) = inventory.get_slot(slot_index) {
                    if !slot.is_empty() && !is_being_dragged {
                        // Draw item icon
                        if let Some(texture) = assets.get_icon_texture(slot.resource_type.unwrap()) {
                            let icon_size = self.slot_size - 4;
                            d.draw_texture_ex(
                                texture,
                                Vector2::new((slot_x + 2) as f32, (slot_y + 2) as f32),
                                0.0,
                                icon_size as f32 / texture.width as f32,
                                Color::WHITE
                            );
                        } else {
                            let icon_color = slot.resource_type.unwrap().get_color();
                            let icon_size = self.slot_size - 4;
                            d.draw_rectangle(slot_x + 2, slot_y + 2, icon_size, icon_size, icon_color);
                        }
                        
                        // Draw amount with smaller font
                        let amount_text = format!("{}", slot.amount);
                        let font_size = 10; // Smaller font for smaller slots
                        let text_width = d.measure_text(&amount_text, font_size);
                        d.draw_rectangle(
                            slot_x + self.slot_size - text_width - 4, 
                            slot_y + self.slot_size - 12, 
                            text_width + 2, 
                            10, 
                            Color::new(0, 0, 0, 180)
                        );
                        d.draw_text(
                            &amount_text, 
                            slot_x + self.slot_size - text_width - 3, 
                            slot_y + self.slot_size - 11, 
                            font_size, 
                            Color::WHITE
                        );
                    }
                }
            }
        }
    }
    
    fn draw_crafting_panel(&self, d: &mut RaylibDrawHandle, crafting_system: &CraftingSystem, inventory: &Inventory, assets: &AssetManager, x: i32, y: i32, mouse_pos: Vector2) {
        let panel_width = 480;
        let panel_height = 600;
        
        // Panel background
        d.draw_rectangle(x, y, panel_width, panel_height, Color::new(40, 40, 40, 240));
        d.draw_rectangle_lines(x, y, panel_width, panel_height, Color::new(200, 200, 200, 255));

        // Title
        d.draw_text("Crafting", x + 20, y + 20, 24, Color::WHITE);
        
        // Category tabs - 2 rows
        let tab_height = 35;
        let tab_y = y + 50;
        let categories = [
            CraftingCategory::BasicMaterials,
            CraftingCategory::Metallurgy,
            CraftingCategory::Textiles,
            CraftingCategory::Structures,
            CraftingCategory::Automation,
        ];
        
        let tabs_per_row = 3;
        let tab_width = panel_width / tabs_per_row as i32;
        
        for (i, category) in categories.iter().enumerate() {
            let row = i / tabs_per_row;
            let col = i % tabs_per_row;
            let tab_x = x + (col as i32 * tab_width);
            let current_tab_y = tab_y + (row as i32 * tab_height);
            
            // Tab background
            let tab_color = if *category == self.selected_category {
                Color::new(80, 120, 80, 255) // Active tab
            } else {
                Color::new(60, 60, 60, 255) // Inactive tab
            };
            
            d.draw_rectangle(tab_x, current_tab_y, tab_width, tab_height, tab_color);
            d.draw_rectangle_lines(tab_x, current_tab_y, tab_width, tab_height, Color::new(100, 100, 100, 255));
            
            // Tab text (centered)
            let tab_text = category.get_name();
            let text_width = d.measure_text(tab_text, 12);
            let text_x = tab_x + (tab_width - text_width) / 2;
            d.draw_text(tab_text, text_x, current_tab_y + 12, 12, Color::WHITE);
        }
        
        // Items grid for selected category
        let items = self.selected_category.get_items();
        let grid_start_y = tab_y + (tab_height * 2) + 20; // Account for 2 rows of tabs
        let item_size = 50i32; // Reduced from 60
        let items_per_row = 7; // Reduced from 8 to fit smaller panel
        let item_spacing = 8i32; // Reduced spacing
        
        // Track which item is being hovered for tooltip
        let mut hovered_item: Option<&CraftableItem> = None;
        
        for (i, item) in items.iter().enumerate() {
            let grid_x = x + 15 + ((i % items_per_row) * (item_size + item_spacing) as usize) as i32;
            let grid_y = grid_start_y + ((i / items_per_row) * (item_size + item_spacing + 25) as usize) as i32;
            
            let craft_status = crafting_system.get_craft_status(item, inventory);
            
            // Check if mouse is hovering over this item
            let is_hovering = mouse_pos.x >= grid_x as f32 && 
                             mouse_pos.x <= (grid_x + item_size) as f32 &&
                             mouse_pos.y >= grid_y as f32 && 
                             mouse_pos.y <= (grid_y + item_size) as f32;
            
            // Remember hovered item for tooltip drawing later
            if is_hovering {
                hovered_item = Some(item);
            }
            
            // Item slot background (brighter when hovering)
            let slot_color = match craft_status {
                CraftStatus::CanCraft => if is_hovering { Color::new(80, 160, 80, 255) } else { Color::new(60, 120, 60, 255) },
                CraftStatus::MissingResources(_) => if is_hovering { Color::new(160, 80, 80, 255) } else { Color::new(120, 60, 60, 255) },
                CraftStatus::NeedsStructure(_) => if is_hovering { Color::new(160, 160, 80, 255) } else { Color::new(120, 120, 60, 255) },
                CraftStatus::NoRecipe => if is_hovering { Color::new(80, 80, 80, 255) } else { Color::new(60, 60, 60, 255) },
            };
            
            // Draw crafting slot background using texture or fallback
            if let Some(slot_texture) = assets.get_ui_texture("crafting_slot") {
                let tint_color = match craft_status {
                    CraftStatus::CanCraft => if is_hovering { Color::new(255, 255, 255, 255) } else { Color::new(200, 255, 200, 255) },
                    CraftStatus::MissingResources(_) => Color::new(255, 200, 200, 255),
                    CraftStatus::NeedsStructure(_) => Color::new(255, 255, 200, 255),
                    CraftStatus::NoRecipe => Color::new(150, 150, 150, 255),
                };
                
                d.draw_texture_ex(
                    slot_texture,
                    Vector2::new(grid_x as f32, grid_y as f32),
                    0.0,
                    item_size as f32 / slot_texture.width as f32,
                    tint_color
                );
            } else {
                // Fallback to rectangle
                d.draw_rectangle(grid_x, grid_y, item_size, item_size, slot_color);
                d.draw_rectangle_lines(grid_x, grid_y, item_size, item_size, Color::new(150, 150, 150, 255));
            }
            
            // Item icon (placeholder - colored square for now)
            if let Some(icon_texture) = assets.get_crafting_icon(*item) {
                let icon_size = item_size - 6;
                d.draw_texture_ex(
                    icon_texture,
                    Vector2::new((grid_x + 3) as f32, (grid_y + 3) as f32),
                    0.0,
                    icon_size as f32 / icon_texture.width as f32,
                    Color::WHITE
                );
            } else {
                // Fallback to colored square
                let icon_color = match item.get_category() {
                    CraftingCategory::BasicMaterials => Color::BROWN,
                    CraftingCategory::Metallurgy => Color::LIGHTGRAY,
                    CraftingCategory::Textiles => Color::BEIGE,
                    CraftingCategory::Structures => Color::DARKGRAY,
                    CraftingCategory::Automation => Color::BLUE,
                };
                d.draw_rectangle(grid_x + 3, grid_y + 3, item_size - 6, item_size - 6, icon_color);
            }
            
            // Item name below slot
            let name = item.get_name();
            let name_width = d.measure_text(name, 9);
            let name_x = grid_x + (item_size - name_width) / 2;
            d.draw_text(name, name_x, grid_y + item_size + 3, 9, Color::WHITE);
        }
        
        // Draw tooltip AFTER all slots to ensure it appears on top
        if let Some(item) = hovered_item {
            self.draw_crafting_tooltip(d, crafting_system, inventory, item, mouse_pos);
        }
    }
    
    fn draw_crafting_tooltip(&self, d: &mut RaylibDrawHandle, crafting_system: &CraftingSystem, inventory: &Inventory, item: &CraftableItem, mouse_pos: Vector2) {
        if let Some(recipe) = crafting_system.recipes.get(item) {
            let tooltip_width = 280;
            let line_height = 16;
            let padding = 8;
            
            // Calculate tooltip height based on content
            let mut content_lines = 1; // Item name
            content_lines += recipe.inputs.len(); // Resource requirements
            if let Some(_) = &recipe.requires_structure {
                content_lines += 1; // Structure requirement
            }
            content_lines += 1; // Crafting time
            
            let tooltip_height = content_lines as i32 * line_height + padding * 2;
            
            // Position tooltip near mouse but keep it on screen
            let mut tooltip_x = mouse_pos.x as i32 + 15;
            let mut tooltip_y = mouse_pos.y as i32 - tooltip_height / 2;
            
            // Keep tooltip on screen
            if tooltip_x + tooltip_width > 1200 { tooltip_x = mouse_pos.x as i32 - tooltip_width - 15; }
            if tooltip_y < 0 { tooltip_y = 0; }
            if tooltip_y + tooltip_height > 800 { tooltip_y = 800 - tooltip_height; }
            
            // Draw tooltip background
            d.draw_rectangle(tooltip_x, tooltip_y, tooltip_width, tooltip_height, Color::new(25, 25, 25, 240));
            d.draw_rectangle_lines(tooltip_x, tooltip_y, tooltip_width, tooltip_height, Color::new(150, 150, 150, 255));
            
            let mut current_y = tooltip_y + padding;
            
            // Item name
            d.draw_text(item.get_name(), tooltip_x + padding, current_y, 14, Color::WHITE);
            current_y += line_height;
            
            // Resource requirements
            if !recipe.inputs.is_empty() {
                for (resource_type, required_amount) in &recipe.inputs {
                    let available = inventory.get_amount(resource_type);
                    let has_enough = available >= *required_amount;
                    
                    let text_color = if has_enough { Color::WHITE } else { Color::RED };
                    let requirement_text = format!("- {} {} (have: {})", required_amount, resource_type.get_name(), available);
                    
                    d.draw_text(&requirement_text, tooltip_x + padding, current_y, 12, text_color);
                    current_y += line_height;
                }
            } else {
                d.draw_text("- No materials required", tooltip_x + padding, current_y, 12, Color::WHITE);
                current_y += line_height;
            }
            
            // Structure requirement
            if let Some(structure) = &recipe.requires_structure {
                let structure_text = if *structure == StructureType::Manual {
                    "- Can be crafted manually".to_string()
                } else {
                    format!("- Requires: {}", structure.get_name())
                };
                d.draw_text(&structure_text, tooltip_x + padding, current_y, 12, Color::YELLOW);
                current_y += line_height;
            }
            
            // Crafting time
            let time_text = format!("- Crafting time: {:.1}s", recipe.crafting_time);
            d.draw_text(&time_text, tooltip_x + padding, current_y, 12, Color::LIGHTGRAY);
        }
    }

    
    fn get_crafting_item_at_mouse(&self, mouse_pos: Vector2, panel_x: i32, panel_y: i32) -> Option<CraftableItem> {
        let items = self.selected_category.get_items();
        let grid_start_y = panel_y + 50 + (35 * 2) + 20; // Account for tabs
        let item_size = 50i32;
        let items_per_row = 7;
        let item_spacing = 8i32;
        
        for (i, item) in items.iter().enumerate() {
            let grid_x = panel_x + 15 + ((i % items_per_row) * (item_size + item_spacing) as usize) as i32;
            let grid_y = grid_start_y + ((i / items_per_row) * (item_size + item_spacing + 25) as usize) as i32;
            
            if mouse_pos.x >= grid_x as f32 && 
               mouse_pos.x <= (grid_x + item_size) as f32 &&
               mouse_pos.y >= grid_y as f32 && 
               mouse_pos.y <= (grid_y + item_size) as f32 {
                return Some(*item);
            }
        }
        None
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