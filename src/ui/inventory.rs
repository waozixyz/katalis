use raylib::prelude::*;
use crate::types::*;
use crate::crafting::*;
use crate::assets::AssetManager;
use crate::Player;
use crate::world::World;
use super::common::*;
use super::inventory_layout::InventoryLayout;

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
            selected_category: CraftingCategory::Tools,
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
    
    pub fn handle_mouse_input(&mut self, rl: &RaylibHandle, player: &mut Player, crafting_system: &CraftingSystem, world: &World, tech_tree: &crate::tech_tree::TechTree) -> Option<crate::PlacementState> {
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
                self.handle_crafting_input(rl, player, crafting_system, world, mouse_pos, tech_tree);
            }
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
    
    fn handle_crafting_input(&mut self, rl: &RaylibHandle, player: &mut Player, crafting_system: &CraftingSystem, world: &World, mouse_pos: Vector2, tech_tree: &crate::tech_tree::TechTree) {
        let panel_x = 650;
        let panel_y = 100;
        let tab_y = panel_y + 50;
        let tab_height = 35;
        let panel_width = 480;
        let tabs_per_row = 3;
        let tab_width = panel_width / tabs_per_row as i32;
        
        let categories = [
            CraftingCategory::Tools,
            CraftingCategory::Materials,
            CraftingCategory::Metals,
            CraftingCategory::Textiles,
            CraftingCategory::Food,
            CraftingCategory::Power,
            CraftingCategory::Buildings,
            CraftingCategory::Automation,
            CraftingCategory::Consumables,
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
        
        if let Some(clicked_item) = self.get_crafting_item_at_mouse(mouse_pos, 650, 100, tech_tree) {
            if let Some(recipe) = crafting_system.recipes.get(&clicked_item) {
                let craft_status = crafting_system.get_craft_status(&clicked_item, &player.inventory, Some(world), Some(player.position));
                
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
                        CraftStatus::BuildingNotAvailable(msg) => println!("Cannot craft: {}", msg),
                        CraftStatus::NoRecipe => println!("No recipe found for {:?}", clicked_item),
                        CraftStatus::CanCraft => {} // This shouldn't happen
                    }
                }
            }
        }
    }
    
    pub fn draw_with_world(&self, d: &mut RaylibDrawHandle, world: &World, inventory: &Inventory, crafting_system: &CraftingSystem, assets: &AssetManager, mouse_pos: Vector2, player: &Player, tech_tree: &crate::tech_tree::TechTree) {
        if !self.is_open {
            return;
        }
        
        let screen_width = 1200;
        let screen_height = 800;
        
        d.draw_rectangle(0, 0, screen_width, screen_height, Color::new(0, 0, 0, 128));
        
        // Draw left panel (always player inventory)
        self.inventory_layout.draw_inventory_with_tooltips(d, player, assets, mouse_pos);
        
        // Draw right panel based on mode
        let right_panel_x = 650;
        let right_panel_y = 100;
        let right_panel_width = 480;
        let right_panel_height = 600;
        
        match self.right_panel_mode {
            RightPanelMode::Crafting => {
                // Don't draw a separate panel background for crafting - the crafting panel draws its own
                self.draw_crafting_panel(d, crafting_system, inventory, assets, world, player, right_panel_x, right_panel_y, mouse_pos, tech_tree);
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
                
                // Show recipes that can be crafted with this building
                self.draw_building_recipes(d, crafting_system, inventory, assets, world, player, building_type, right_panel_x, right_panel_y, right_panel_width, mouse_pos);
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
    }
    
    fn draw_crafting_panel(&self, d: &mut RaylibDrawHandle, crafting_system: &CraftingSystem, inventory: &Inventory, assets: &AssetManager, world: &World, player: &Player, x: i32, y: i32, mouse_pos: Vector2, tech_tree: &crate::tech_tree::TechTree) {
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
            CraftingCategory::Tools,
            CraftingCategory::Materials,
            CraftingCategory::Metals,
            CraftingCategory::Textiles,
            CraftingCategory::Food,
            CraftingCategory::Power,
            CraftingCategory::Buildings,
            CraftingCategory::Automation,
            CraftingCategory::Consumables,
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
            let text_width = d.measure_text(tab_text, 12) as i32;
            let text_x = tab_x + (tab_width - text_width) / 2;
            d.draw_text(tab_text, text_x, current_tab_y + 12, 12, Color::WHITE);
        }
        
        // Items grid for selected category (filter by tech tree)
        let all_items = self.selected_category.get_items();
        let items: Vec<_> = all_items.into_iter()
            .filter(|item| tech_tree.is_recipe_unlocked(item))
            .collect();
        let num_tab_rows = (categories.len() + tabs_per_row - 1) / tabs_per_row; // Calculate number of tab rows
        let grid_start_y = tab_y + (tab_height * num_tab_rows as i32) + 20; // Account for actual number of tab rows
        let item_size = 50i32; // Reduced from 60
        let items_per_row = 7; // Reduced from 8 to fit smaller panel
        let item_spacing = 8i32; // Reduced spacing
        
        // Track which item is being hovered for tooltip
        let mut hovered_item: Option<&CraftableItem> = None;
        
        for (i, item) in items.iter().enumerate() {
            let grid_x = x + 15 + ((i % items_per_row) * (item_size + item_spacing) as usize) as i32;
            let grid_y = grid_start_y + ((i / items_per_row) * (item_size + item_spacing + 10) as usize) as i32;
            
            let craft_status = crafting_system.get_craft_status(item, inventory, Some(world), Some(player.position));
            
            // Check if mouse is hovering over this item
            let is_hovering = mouse_pos.x >= grid_x as f32 && 
                             mouse_pos.x <= (grid_x + item_size as i32) as f32 &&
                             mouse_pos.y >= grid_y as f32 && 
                             mouse_pos.y <= (grid_y + item_size as i32) as f32;
            
            // Remember hovered item for tooltip drawing later
            if is_hovering {
                hovered_item = Some(item);
            }
            
            // Item slot background (brighter when hovering)
            let slot_color = match craft_status {
                CraftStatus::CanCraft => if is_hovering { Color::new(80, 160, 80, 255) } else { Color::new(60, 120, 60, 255) },
                CraftStatus::MissingResources(_) => if is_hovering { Color::new(160, 80, 80, 255) } else { Color::new(120, 60, 60, 255) },
                CraftStatus::NeedsStructure(_) => if is_hovering { Color::new(160, 160, 80, 255) } else { Color::new(120, 120, 60, 255) },
                CraftStatus::BuildingNotAvailable(_) => if is_hovering { Color::new(180, 60, 60, 255) } else { Color::new(140, 40, 40, 255) },
                CraftStatus::NoRecipe => if is_hovering { Color::new(80, 80, 80, 255) } else { Color::new(60, 60, 60, 255) },
            };
            
            // Draw crafting slot background using texture or fallback
            if let Some(slot_texture) = assets.get_ui_texture("crafting_slot") {
                let tint_color = match craft_status {
                    CraftStatus::CanCraft => if is_hovering { Color::new(255, 255, 255, 255) } else { Color::new(200, 255, 200, 255) },
                    CraftStatus::MissingResources(_) => Color::new(255, 200, 200, 255),
                    CraftStatus::NeedsStructure(_) => Color::new(255, 255, 200, 255),
                    CraftStatus::BuildingNotAvailable(_) => Color::new(255, 150, 150, 255),
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
                d.draw_rectangle(grid_x, grid_y, item_size as i32, item_size as i32, slot_color);
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
                d.draw_rectangle(grid_x + 3, grid_y + 3, item_size as i32 - 6, item_size as i32 - 6, icon_color);
            }
        }
        
        // Draw tooltip AFTER all slots to ensure it appears on top
        if let Some(item) = hovered_item {
            self.draw_crafting_tooltip(d, crafting_system, inventory, item, mouse_pos);
        }
    }
    
    fn draw_building_recipes(&self, d: &mut RaylibDrawHandle, crafting_system: &CraftingSystem, inventory: &Inventory, assets: &AssetManager, world: &World, player: &Player, building_type: BuildingType, x: i32, y: i32, _width: i32, mouse_pos: Vector2) {
        let recipes = crafting_system.get_recipes_for_building(building_type);
        if recipes.is_empty() {
            return;
        }
        
        // Draw recipes section below the building interface
        let recipes_y = y + 300; // Position below building slots
        d.draw_text("Recipes:", x + 20, recipes_y, 18, Color::WHITE);
        
        let recipes_start_y = recipes_y + 25;
        let item_size = 40;
        let item_spacing = 8;
        let items_per_row = 6;
        
        for (i, item) in recipes.iter().enumerate() {
            let grid_x = x + 20 + ((i % items_per_row) * (item_size + item_spacing)) as i32;
            let grid_y = recipes_start_y + ((i / items_per_row) * (item_size + item_spacing + 20)) as i32;
            
            let craft_status = crafting_system.get_craft_status(item, inventory, Some(world), Some(player.position));
            
            // Check if mouse is hovering over this item
            let is_hovering = mouse_pos.x >= grid_x as f32 && 
                             mouse_pos.x <= (grid_x + item_size as i32) as f32 &&
                             mouse_pos.y >= grid_y as f32 && 
                             mouse_pos.y <= (grid_y + item_size as i32) as f32;
            
            // Item slot background (with appropriate color coding)
            let slot_color = match craft_status {
                CraftStatus::CanCraft => if is_hovering { Color::new(80, 160, 80, 255) } else { Color::new(60, 120, 60, 255) },
                CraftStatus::MissingResources(_) => if is_hovering { Color::new(160, 80, 80, 255) } else { Color::new(120, 60, 60, 255) },
                CraftStatus::NeedsStructure(_) => if is_hovering { Color::new(160, 160, 80, 255) } else { Color::new(120, 120, 60, 255) },
                CraftStatus::BuildingNotAvailable(_) => if is_hovering { Color::new(180, 60, 60, 255) } else { Color::new(140, 40, 40, 255) },
                CraftStatus::NoRecipe => if is_hovering { Color::new(80, 80, 80, 255) } else { Color::new(60, 60, 60, 255) },
            };
            
            d.draw_rectangle(grid_x, grid_y, item_size as i32, item_size as i32, slot_color);
            d.draw_rectangle_lines(grid_x, grid_y, item_size as i32, item_size as i32, Color::BLACK);
            
            // Draw item icon
            if let Some(icon_texture) = assets.get_crafting_icon(*item) {
                d.draw_texture_pro(
                    icon_texture,
                    Rectangle::new(0.0, 0.0, icon_texture.width as f32, icon_texture.height as f32),
                    Rectangle::new(grid_x as f32 + 4.0, grid_y as f32 + 4.0, (item_size - 8) as f32, (item_size - 8) as f32),
                    Vector2::zero(),
                    0.0,
                    Color::WHITE
                );
            } else {
                // Fallback to colored square
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
                d.draw_rectangle(grid_x + 3, grid_y + 3, item_size as i32 - 6, item_size as i32 - 6, icon_color);
            }
            
            // Draw tooltip if hovering
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
    
    fn get_crafting_item_at_mouse(&self, mouse_pos: Vector2, panel_x: i32, panel_y: i32, tech_tree: &crate::tech_tree::TechTree) -> Option<CraftableItem> {
        let all_items = self.selected_category.get_items();
        let items: Vec<_> = all_items.into_iter()
            .filter(|item| tech_tree.is_recipe_unlocked(item))
            .collect();
        let categories = [
            CraftingCategory::Tools,
            CraftingCategory::Materials,
            CraftingCategory::Metals,
            CraftingCategory::Textiles,
            CraftingCategory::Food,
            CraftingCategory::Power,
            CraftingCategory::Buildings,
            CraftingCategory::Automation,
            CraftingCategory::Consumables,
        ];
        let tabs_per_row = 3;
        let tab_height = 35;
        let tab_y = panel_y + 50;
        let num_tab_rows = (categories.len() + tabs_per_row - 1) / tabs_per_row;
        let grid_start_y = tab_y + (tab_height * num_tab_rows as i32) + 20; // Account for actual number of tab rows
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
            BuildingType::Campfire => "Campfire",
            BuildingType::CharcoalPit => "Charcoal Pit",
            BuildingType::CrudeFurnace => "Crude Furnace",
            BuildingType::BloomeryFurnace => "Bloomery Furnace",
            BuildingType::StoneAnvil => "Stone Anvil",
            BuildingType::SpinningWheel => "Spinning Wheel",
            BuildingType::WeavingMachine => "Weaving Machine",
            BuildingType::ConveyorBelt => "Conveyor Belt",
            BuildingType::AdvancedForge => "Advanced Forge",
            BuildingType::WheatFarm => "Wheat Farm",
            BuildingType::Windmill => "Windmill",
            BuildingType::WaterMill => "Water Mill",
            BuildingType::StoneOven => "Stone Oven",
            BuildingType::GrainSilo => "Grain Silo",
            BuildingType::SteamBoiler => "Steam Boiler",
            BuildingType::SteamDistributionHub => "Steam Distribution Hub",
            BuildingType::WaterPump => "Water Pump",
            BuildingType::SteamPump => "Steam Pump",
            BuildingType::SteamHammer => "Steam Hammer",
            BuildingType::SortingMachine => "Sorting Machine",
            BuildingType::SteamEngine => "Steam Engine",
        }
    }
    
    fn draw_building_interface(&self, d: &mut RaylibDrawHandle, assets: &AssetManager, inventory: &crate::world::BuildingInventory, panel_x: i32, panel_y: i32, panel_width: i32) {
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