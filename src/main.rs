mod types;
mod player;
mod world;
mod camera;
mod input;
mod render;
mod assets;
mod animation;
mod crafting; // NEW
mod ui; // NEW

use raylib::prelude::*;
use types::*;
use player::Player;
use world::World;
use camera::update_camera;
use input::handle_input;
use render::{draw_world, draw_time_display};
use assets::AssetManager;
use crafting::CraftingSystem; // NEW
use ui::{InventoryUI, PauseMenu}; // NEW

const SCREEN_WIDTH: i32 = 1200;
const SCREEN_HEIGHT: i32 = 800;

fn main() {
    let (mut rl, thread) = raylib::init()
        .size(SCREEN_WIDTH, SCREEN_HEIGHT)
        .title("Katalis - Factory Builder")
        .vsync()
        .build();
    
    // Disable ESC key to exit window (we want to handle ESC ourselves)
    rl.set_exit_key(None);

    rl.set_target_fps(60);

    // Initialize systems
    let mut assets = AssetManager::new();
    if let Err(e) = assets.load_assets(&mut rl, &thread) {
        println!("Warning: Failed to load some assets: {}", e);
    }

    let mut world = World::new(100, 100);
    world.generate_simple();

    let mut game_player = Player::new(
        50.0 * TILE_SIZE as f32,
        50.0 * TILE_SIZE as f32
    );

    // Load player sprite
    if let Ok(player_texture) = rl.load_texture(&thread, "assets/player/player_spritesheet.png") {
        game_player.load_sprite(player_texture);
        println!("Loaded player spritesheet");
    } else {
        println!("Warning: Could not load player spritesheet, using fallback graphics");
    }

    // NEW: Initialize crafting and UI systems
    let crafting_system = CraftingSystem::new();
    let mut inventory_ui = InventoryUI::new();
    let mut pause_menu = PauseMenu::new();

    let mut camera = Camera2D {
        target: game_player.position,
        offset: Vector2::new(SCREEN_WIDTH as f32 / 2.0, SCREEN_HEIGHT as f32 / 2.0),
        rotation: 0.0,
        zoom: 1.0,
    };

    let mut camera_target = game_player.position;

    loop {
        // Check if window should close
        if rl.window_should_close() {
            break;
        }
        let delta_time = rl.get_frame_time();
        
        // Update UI systems first
        let esc_consumed_by_inventory = inventory_ui.update(&rl);
        let should_quit = pause_menu.update(&rl, esc_consumed_by_inventory);
        
        // Check if we should quit the game
        if should_quit {
            break;
        }
        
        // Only update game systems if not paused
        if !pause_menu.is_open {
            game_player.update(&rl, &camera);
            update_camera(&mut camera, &mut camera_target, &game_player, &rl);
            handle_input(&mut world, &camera, &rl, &mut game_player);
            inventory_ui.handle_mouse_input(&rl, &mut game_player, &crafting_system);

        }
        
        // Update world and collect resources only if not paused
        if !pause_menu.is_open {
            let (wood_gained, stone_gained, iron_gained, coal_gained, clay_gained, copper_gained, cotton_gained) = world.update(delta_time, &mut game_player);
            
            // Add gained resources to inventory
            if wood_gained > 0 { game_player.inventory.add_resource(ResourceType::Wood, wood_gained); }
            if stone_gained > 0 { game_player.inventory.add_resource(ResourceType::Stone, stone_gained); }
            if iron_gained > 0 { game_player.inventory.add_resource(ResourceType::IronOre, iron_gained); }
            if coal_gained > 0 { game_player.inventory.add_resource(ResourceType::Coal, coal_gained); }
            if clay_gained > 0 { game_player.inventory.add_resource(ResourceType::Clay, clay_gained); }
            if copper_gained > 0 { game_player.inventory.add_resource(ResourceType::CopperOre, copper_gained); }
            if cotton_gained > 0 { game_player.inventory.add_resource(ResourceType::Cotton, cotton_gained); }
        }
    
        // Get mouse position BEFORE starting drawing
        let mouse_screen_pos = rl.get_mouse_position();
        let mouse_world_pos = rl.get_screen_to_world2D(mouse_screen_pos, camera);
        let distance = game_player.position.distance_to(mouse_world_pos);
        
        // Draw
        let mut d = rl.begin_drawing(&thread);
        d.clear_background(Color::new(45, 55, 45, 255));
        
        {
            let mut d2d = d.begin_mode2D(camera);
            draw_world(&mut d2d, &world, &camera, &assets);
            game_player.draw(&mut d2d);
            
            // Draw mining range indicator when player is close to mouse
            if distance <= MINING_RANGE {
                d2d.draw_circle_lines_v(game_player.position, MINING_RANGE, Color::new(255, 255, 0, 100));
                d2d.draw_circle_lines_v(mouse_world_pos, 8.0, Color::YELLOW);
                
                // Show what resource/tree is being targeted
                let tile_x = (mouse_world_pos.x / TILE_SIZE as f32).floor() as usize;
                let tile_y = (mouse_world_pos.y / TILE_SIZE as f32).floor() as usize;
                
                if let Some(tile) = world.get_tile(tile_x, tile_y) {
                    if let Some(vein) = &tile.resource_vein {
                        let info_text = format!("{}: {}", vein.vein_type.get_name(), vein.richness);
                        let text_pos = Vector2::new(mouse_world_pos.x + 15.0, mouse_world_pos.y - 10.0);
                        
                        d2d.draw_rectangle(
                            text_pos.x as i32 - 2,
                            text_pos.y as i32 - 2,
                            (info_text.len() * 8) as i32,
                            16,
                            Color::new(0, 0, 0, 180)
                        );
                        
                        d2d.draw_text(
                            &info_text,
                            text_pos.x as i32,
                            text_pos.y as i32,
                            12,
                            Color::WHITE
                        );
                    }
                }
            } else {
                d2d.draw_circle_lines_v(mouse_world_pos, 8.0, Color::RED);
            }
        }
        
        inventory_ui.draw(&mut d, &game_player.inventory, &crafting_system, &assets, mouse_screen_pos, &game_player);
        
        // Draw minimal UI when inventory is closed
        if !inventory_ui.is_open {
            draw_minimal_ui(&mut d, &game_player);
        }
        
        // Always draw quick slot bar at bottom (even when inventory is open)
        draw_quick_slot_bar(&mut d, &game_player, &assets);
        
        draw_time_display(&mut d, &world.game_time);
        
        // Draw pause menu last (on top of everything)
        pause_menu.draw(&mut d);
    }
}

// NEW: Minimal UI function when inventory is closed
fn draw_minimal_ui(d: &mut RaylibDrawHandle, _player: &Player) {
    // Small info panel - simplified to just show the instruction
    d.draw_rectangle(10, 10, 200, 30, Color::new(47, 47, 47, 220));
    d.draw_rectangle_lines(10, 10, 200, 30, Color::new(150, 150, 150, 255));
    
    d.draw_text("Press E for Inventory", 20, 20, 16, Color::WHITE);
}

// Quick slot bar showing first row of inventory
fn draw_quick_slot_bar(d: &mut RaylibDrawHandle, player: &Player, assets: &AssetManager) {
    let screen_width = 1200;
    let screen_height = 800;
    let slot_size = 40;
    let slot_spacing = 4;
    let slots_per_row = 8;
    let total_width = slots_per_row * slot_size + (slots_per_row - 1) * slot_spacing;
    let start_x = (screen_width - total_width) / 2;
    let start_y = screen_height - slot_size - 20; // 20px from bottom
    
    // Draw background for the entire quick slot bar
    let padding = 8;
    d.draw_rectangle(
        start_x - padding, 
        start_y - padding, 
        total_width + padding * 2, 
        slot_size + padding * 2, 
        Color::new(40, 40, 40, 200)
    );
    d.draw_rectangle_lines(
        start_x - padding, 
        start_y - padding, 
        total_width + padding * 2, 
        slot_size + padding * 2, 
        Color::new(100, 100, 100, 255)
    );
    
    // Draw the first 8 inventory slots
    for i in 0..slots_per_row {
        let slot_x = start_x + i * (slot_size + slot_spacing);
        let slot_y = start_y;
        
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
            d.draw_rectangle(slot_x, slot_y, slot_size, slot_size, Color::new(60, 60, 60, 255));
            d.draw_rectangle_lines(slot_x, slot_y, slot_size, slot_size, Color::new(100, 100, 100, 255));
        }
        
        // Draw item if slot has one
        if let Some(slot) = player.inventory.get_slot(i as usize) {
            if !slot.is_empty() {
                // Draw item icon
                if let Some(texture) = assets.get_icon_texture(slot.resource_type.unwrap()) {
                    let icon_size = slot_size - 4;
                    d.draw_texture_ex(
                        texture,
                        Vector2::new((slot_x + 2) as f32, (slot_y + 2) as f32),
                        0.0,
                        icon_size as f32 / texture.width as f32,
                        Color::WHITE
                    );
                } else {
                    let icon_color = slot.resource_type.unwrap().get_color();
                    let icon_size = slot_size - 4;
                    d.draw_rectangle(slot_x + 2, slot_y + 2, icon_size, icon_size, icon_color);
                }
                
                // Draw amount
                let amount_text = format!("{}", slot.amount);
                let font_size = 10;
                let text_width = d.measure_text(&amount_text, font_size);
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
                    font_size, 
                    Color::WHITE
                );
            }
        }
    }
}