use raylib::prelude::*;
use crate::types::*;
use crate::crafting::*;
use crate::assets::AssetManager;

pub struct InventoryUI {
    pub is_open: bool,
    pub selected_recipe: Option<CraftableItem>,
    pub selected_category: CraftingCategory,
}

impl InventoryUI {
    pub fn new() -> Self {
        Self {
            is_open: false,
            selected_recipe: None,
            selected_category: CraftingCategory::BasicMaterials,
        }
    }
    
    pub fn toggle(&mut self) {
        self.is_open = !self.is_open;
    }
    
    pub fn handle_mouse_input(&mut self, rl: &RaylibHandle) {
        if !self.is_open {
            return;
        }
        
        if rl.is_mouse_button_pressed(MouseButton::MOUSE_BUTTON_LEFT) {
            let mouse_pos = rl.get_mouse_position();
            
            // Check if clicking on crafting panel tabs
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
                
                // Check if mouse is within tab bounds
                if mouse_pos.x >= tab_x as f32 && 
                   mouse_pos.x <= (tab_x + tab_width) as f32 &&
                   mouse_pos.y >= current_tab_y as f32 && 
                   mouse_pos.y <= (current_tab_y + tab_height) as f32 {
                    self.selected_category = *category;
                    break;
                }
            }
        }
    }
    
    pub fn update(&mut self, rl: &RaylibHandle) {
        // Toggle inventory with E key
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
        
        // Draw semi-transparent overlay
        d.draw_rectangle(0, 0, screen_width, screen_height, Color::new(0, 0, 0, 128));
        
        // Left panel - Inventory
        self.draw_inventory_panel(d, inventory, assets, 50, 100);
        
        // Right panel - Crafting
        self.draw_crafting_panel(d, crafting_system, inventory, assets, 650, 100, mouse_pos);
    }
    
    fn draw_inventory_panel(&self, d: &mut RaylibDrawHandle, inventory: &Inventory, assets: &AssetManager, x: i32, y: i32) {
        let panel_width = 500;
        let panel_height = 600;
        
        // Panel background
        d.draw_rectangle(x, y, panel_width, panel_height, Color::new(40, 40, 40, 240));
        d.draw_rectangle_lines(x, y, panel_width, panel_height, Color::new(200, 200, 200, 255));
        
        // Title
        d.draw_text("Inventory", x + 20, y + 20, 24, Color::WHITE);
        
        // Draw inventory grid
        let slot_size: i32 = 60; // Increased from 50
        let slots_per_row = 6; // Reduced from 8 for better spacing
        let start_x = x + 20;
        let start_y = y + 60;
        
        let resources = [
            ResourceType::Wood, ResourceType::Stone, ResourceType::IronOre, ResourceType::Coal,
            ResourceType::Clay, ResourceType::CopperOre, ResourceType::Cotton, ResourceType::Charcoal,
            ResourceType::IronBloom, ResourceType::WroughtIron, ResourceType::IronPlates, ResourceType::IronGears,
            ResourceType::MetalRods, ResourceType::Threads, ResourceType::Fabric, ResourceType::ClothStrips,
        ];
        
        for (i, resource) in resources.iter().enumerate() {
            let slot_x = start_x + ((i % slots_per_row) * (slot_size + 15) as usize) as i32; // Increased spacing from 10 to 15
            let slot_y = start_y + ((i / slots_per_row) * (slot_size + 25) as usize) as i32; // Increased spacing from 10 to 25 for text
            let amount = inventory.get_amount(resource);
            
            // Draw slot background
            d.draw_rectangle(slot_x, slot_y, slot_size, slot_size, Color::new(60, 60, 60, 255));
            d.draw_rectangle_lines(slot_x, slot_y, slot_size, slot_size, Color::new(100, 100, 100, 255));
            
            // Draw resource icon (use texture if available, fallback to colored square)
            if let Some(texture) = assets.get_icon_texture(*resource) {
                // Draw icon texture scaled to fit slot with padding
                let icon_size = slot_size - 10;
                d.draw_texture_ex(
                    texture,
                    Vector2::new((slot_x + 5) as f32, (slot_y + 5) as f32),
                    0.0,
                    icon_size as f32 / texture.width as f32,
                    Color::WHITE
                );
            } else {
                // Fallback to colored square
                let icon_color = resource.get_color();
                d.draw_rectangle(slot_x + 5, slot_y + 5, slot_size - 10, slot_size - 10, icon_color);
            }
            
            // Draw amount text with better positioning and background
            if amount > 0 {
                let amount_text = format!("{}", amount);
                // Draw a small background for better readability
                let text_width = d.measure_text(&amount_text, 14);
                d.draw_rectangle(slot_x + slot_size - text_width - 8, slot_y + slot_size - 20, text_width + 6, 16, Color::new(0, 0, 0, 180));
                d.draw_text(&amount_text, slot_x + slot_size - text_width - 5, slot_y + slot_size - 16, 14, Color::WHITE);
            }
            
            // Resource name below slot with better spacing and centering
            let name = resource.get_name();
            let name_width = d.measure_text(name, 11);
            let name_x = slot_x + (slot_size - name_width) / 2; // Center the text
            d.draw_text(name, name_x, slot_y + slot_size + 8, 11, Color::LIGHTGRAY);
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
        let item_size = 50; // Reduced from 60
        let items_per_row = 7; // Reduced from 8 to fit smaller panel
        let item_spacing = 8; // Reduced spacing
        
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
            
            d.draw_rectangle(grid_x, grid_y, item_size, item_size, slot_color);
            d.draw_rectangle_lines(grid_x, grid_y, item_size, item_size, Color::new(150, 150, 150, 255));
            
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
