mod types;
mod player;
mod world;
mod camera;
mod input;
mod render;
mod assets;
mod animation; // NEW

use raylib::prelude::*;
use types::*;
use player::Player; // Import the Player struct
use world::World;
use camera::update_camera;
use input::handle_input;
use render::{draw_world, draw_ui, draw_time_display};
use assets::AssetManager;

const SCREEN_WIDTH: i32 = 1200;
const SCREEN_HEIGHT: i32 = 800;

fn main() {
    let (mut rl, thread) = raylib::init()
        .size(SCREEN_WIDTH, SCREEN_HEIGHT)
        .title("Katalis - Factory Builder")
        .vsync()
        .build();

    rl.set_target_fps(60);

    // Initialize asset manager and load assets
    let mut assets = AssetManager::new();
    if let Err(e) = assets.load_assets(&mut rl, &thread) {
        println!("Warning: Failed to load some assets: {}", e);
    }

    // Initialize world
    let mut world = World::new(100, 100);
    world.generate_simple();

    // Initialize player at world center - use a different variable name to avoid confusion
    let mut game_player = Player::new(
        50.0 * TILE_SIZE as f32,
        50.0 * TILE_SIZE as f32
    );

    // Load player sprite (add this file to your assets folder)
    if let Ok(player_texture) = rl.load_texture(&thread, "assets/player_spritesheet.png") {
        game_player.load_sprite(player_texture);
        println!("Loaded player spritesheet");
    } else {
        println!("Warning: Could not load player spritesheet, using fallback graphics");
    }

    // Factorio-style camera setup with proper angle
    let mut camera = Camera2D {
        target: game_player.position,
        offset: Vector2::new(SCREEN_WIDTH as f32 / 2.0, SCREEN_HEIGHT as f32 / 2.0),
        rotation: 0.0,
        zoom: 1.0,
    };

    let mut camera_target = game_player.position;

    while !rl.window_should_close() {
        let delta_time = rl.get_frame_time();
        
        // Update
        game_player.update(&rl, &camera);
        update_camera(&mut camera, &mut camera_target, &game_player, &rl);
        handle_input(&mut world, &camera, &rl, &mut game_player);
        
        // Update world systems and collect resources
        let (wood_gained, stone_gained, iron_gained, coal_gained, clay_gained, copper_gained) = world.update(delta_time, &mut game_player);
        
        // Add gained resources to inventory
        if wood_gained > 0 {
            game_player.inventory.add_resource(ResourceType::Wood, wood_gained);
            println!("Added {} wood to inventory. Total wood: {}", wood_gained, game_player.inventory.get_amount(&ResourceType::Wood));
        }
        if stone_gained > 0 {
            game_player.inventory.add_resource(ResourceType::Stone, stone_gained);
            println!("Added {} stone to inventory. Total stone: {}", stone_gained, game_player.inventory.get_amount(&ResourceType::Stone));
        }
        if iron_gained > 0 {
            game_player.inventory.add_resource(ResourceType::IronOre, iron_gained);
            println!("Added {} iron ore to inventory. Total iron ore: {}", iron_gained, game_player.inventory.get_amount(&ResourceType::IronOre));
        }
        if coal_gained > 0 {
            game_player.inventory.add_resource(ResourceType::Coal, coal_gained);
            println!("Added {} coal to inventory. Total coal: {}", coal_gained, game_player.inventory.get_amount(&ResourceType::Coal));
        }
        if clay_gained > 0 {
            game_player.inventory.add_resource(ResourceType::Clay, clay_gained);
            println!("Added {} clay to inventory. Total clay: {}", clay_gained, game_player.inventory.get_amount(&ResourceType::Clay));
        }
        if copper_gained > 0 {
            game_player.inventory.add_resource(ResourceType::CopperOre, copper_gained);
            println!("Added {} copper ore to inventory. Total copper ore: {}", copper_gained, game_player.inventory.get_amount(&ResourceType::CopperOre));
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
                // Draw mining range circle
                d2d.draw_circle_lines_v(game_player.position, MINING_RANGE, Color::new(255, 255, 0, 100));
                
                // Draw target indicator at mouse position
                d2d.draw_circle_lines_v(mouse_world_pos, 8.0, Color::YELLOW);
                
                // Show what resource/tree is being targeted
                let tile_x = (mouse_world_pos.x / TILE_SIZE as f32).floor() as usize;
                let tile_y = (mouse_world_pos.y / TILE_SIZE as f32).floor() as usize;
                
                if let Some(tile) = world.get_tile(tile_x, tile_y) {
                    if let Some(vein) = &tile.resource_vein {
                        // Draw resource info near cursor
                        let info_text = format!("{}: {}", vein.vein_type.get_name(), vein.richness);
                        let text_pos = Vector2::new(mouse_world_pos.x + 15.0, mouse_world_pos.y - 10.0);
                        
                        // Background for text
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
                // Draw out-of-range indicator
                d2d.draw_circle_lines_v(mouse_world_pos, 8.0, Color::RED);
            }
        }
        
        draw_ui(&mut d, &game_player);
        draw_time_display(&mut d, &world.game_time);
    }
}