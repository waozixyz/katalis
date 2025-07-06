use raylib::prelude::*;
use crate::types::*;
use crate::crafting::*;
use crate::assets::AssetManager;

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
        }
    }
    
    pub fn toggle(&mut self) {
        self.is_open = !self.is_open;
        // Clear drag state when closing inventory
        if !self.is_open {
            self.dragged_item = None;
        }
    }
    
    pub fn handle_mouse_input(&mut self, rl: &RaylibHandle, inventory: &mut Inventory) {
        if !self.is_open {
            return;
        }
        
        let mouse_pos = rl.get_mouse_position();
        
        if rl.is_mouse_button_pressed(MouseButton::MOUSE_BUTTON_LEFT) {
            // Handle inventory slot clicks
            if let Some(slot_index) = self.get_slot_at_mouse(mouse_pos, 50, 100) {
                if let Some(slot) = inventory.get_slot(slot_index) {
                    if !slot.is_empty() {
                        // Start dragging
                        self.dragged_item = Some(DraggedItem {
                            resource_type: slot.resource_type.unwrap(),
                            amount: slot.amount,
                            source_slot: slot_index,
                        });
                        self.drag_offset = Vector2::zero();
                    }
                }
            }
            
            // Handle crafting panel tabs (existing code)
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
        }
        
        if rl.is_mouse_button_released(MouseButton::MOUSE_BUTTON_LEFT) {
            if let Some(dragged) = &self.dragged_item {
                // Try to drop in a new slot
                if let Some(target_slot) = self.get_slot_at_mouse(mouse_pos, 50, 100) {
                    if target_slot != dragged.source_slot {
                        // Attempt to move item
                        if let Some(target) = inventory.get_slot(target_slot) {
                            if target.is_empty() || target.resource_type == Some(dragged.resource_type) {
                                // Remove from source
                                if let Some(source) = inventory.get_slot_mut(dragged.source_slot) {
                                    source.remove(dragged.amount);
                                }
                                // Add to target
                                if let Some(target) = inventory.get_slot_mut(target_slot) {
                                    target.add(dragged.resource_type, dragged.amount);
                                }
                            }
                        }
                    }
                }
            }
            self.dragged_item = None;
        }
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
    
    pub fn update(&mut self, rl: &RaylibHandle) {
        if rl.is_key_pressed(KeyboardKey::KEY_E) {
            self.toggle();
        }
    }
    
    pub fn draw(&self, d: &mut RaylibDrawHandle, inventory: &Inventory, crafting_system: &CraftingSystem, assets: &AssetManager, mouse_pos: Vector2) {
        if !self.is_open {
            return;
        }
        
        let screen_width = 1200;
        let screen_height = 800;
        
        d.draw_rectangle(0, 0, screen_width, screen_height, Color::new(0, 0, 0, 128));
        
        self.draw_inventory_panel(d, inventory, assets, 50, 100);
        self.draw_crafting_panel(d, crafting_system, inventory, assets, 650, 100, mouse_pos);
        
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
        let panel_width = 480; // Reduced from 600
        let panel_height = 550; // Reduced from 650
        
        // Panel background
        d.draw_rectangle(x, y, panel_width, panel_height, Color::new(40, 40, 40, 240));
        d.draw_rectangle_lines(x, y, panel_width, panel_height, Color::new(200, 200, 200, 255));

        // Title
        d.draw_text("Crafting", x + 20, y + 15, 24, Color::WHITE);
        
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
        
        for (i, item) in items.iter().enumerate() {
            let grid_x = x + 15 + ((i % items_per_row) * (item_size + item_spacing) as usize) as i32;
            let grid_y = grid_start_y + ((i / items_per_row) * (item_size + item_spacing + 25) as usize) as i32;
            
            let craft_status = crafting_system.get_craft_status(item, inventory);
            
            // Check if mouse is hovering over this item
            let is_hovering = mouse_pos.x >= grid_x as f32 && 
                             mouse_pos.x <= (grid_x + item_size) as f32 &&
                             mouse_pos.y >= grid_y as f32 && 
                             mouse_pos.y <= (grid_y + item_size) as f32;
            
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
            let icon_color = match item.get_category() {
                CraftingCategory::BasicMaterials => Color::BROWN,
                CraftingCategory::Metallurgy => Color::LIGHTGRAY,
                CraftingCategory::Textiles => Color::BEIGE,
                CraftingCategory::Structures => Color::DARKGRAY,
                CraftingCategory::Automation => Color::BLUE,
            };
            d.draw_rectangle(grid_x + 3, grid_y + 3, item_size - 6, item_size - 6, icon_color);
            
            // Item name below slot
            let name = item.get_name();
            let name_width = d.measure_text(name, 9);
            let name_x = grid_x + (item_size - name_width) / 2;
            d.draw_text(name, name_x, grid_y + item_size + 3, 9, Color::WHITE);
            
            // Show detailed tooltip on hover
            if is_hovering {
                self.draw_crafting_tooltip(d, crafting_system, inventory, item, mouse_pos);
            }
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
                    let requirement_text = format!("• {} {} (have: {})", required_amount, resource_type.get_name(), available);
                    
                    d.draw_text(&requirement_text, tooltip_x + padding, current_y, 12, text_color);
                    current_y += line_height;
                }
            } else {
                d.draw_text("• No materials required", tooltip_x + padding, current_y, 12, Color::WHITE);
                current_y += line_height;
            }
            
            // Structure requirement
            if let Some(structure) = &recipe.requires_structure {
                let structure_text = if *structure == StructureType::Manual {
                    "• Can be crafted manually".to_string()
                } else {
                    format!("• Requires: {}", structure.get_name())
                };
                d.draw_text(&structure_text, tooltip_x + padding, current_y, 12, Color::YELLOW);
                current_y += line_height;
            }
            
            // Crafting time
            let time_text = format!("• Crafting time: {:.1}s", recipe.crafting_time);
            d.draw_text(&time_text, tooltip_x + padding, current_y, 12, Color::LIGHTGRAY);
        }
    }
}