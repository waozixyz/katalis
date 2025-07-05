mod types;
mod player;
mod world;
mod camera;
mod input;
mod render;

use raylib::prelude::*;
use types::*;
use player::Player;
use world::World;
use camera::update_camera;
use input::handle_input;
use render::{draw_world, draw_ui, draw_time_display};

const SCREEN_WIDTH: i32 = 1200;
const SCREEN_HEIGHT: i32 = 800;

fn main() {
    let (mut rl, thread) = raylib::init()
        .size(SCREEN_WIDTH, SCREEN_HEIGHT)
        .title("Katalis - Factory Builder")
        .vsync()
        .build();

    rl.set_target_fps(60);

    // Initialize world
    let mut world = World::new(100, 100);
    world.generate_simple();

    // Initialize player at world center
    let mut player = Player::new(
        50.0 * TILE_SIZE as f32,
        50.0 * TILE_SIZE as f32
    );

    // Factorio-style camera setup with proper angle
    let mut camera = Camera2D {
        target: player.position,
        offset: Vector2::new(SCREEN_WIDTH as f32 / 2.0, SCREEN_HEIGHT as f32 / 2.0),
        rotation: -20.0,
        zoom: 1.0,
    };

    let mut camera_target = player.position;

    while !rl.window_should_close() {
        let delta_time = rl.get_frame_time();
        
        // Update
        player.update(&rl, &camera);
        update_camera(&mut camera, &mut camera_target, &player, &rl);
        handle_input(&mut world, &camera, &rl, &mut player);
        
        // Update world systems and collect resources
        let (wood_gained, stone_gained, iron_gained, coal_gained) = world.update(delta_time, &mut player);
        
        // Add gained resources to inventory
        if wood_gained > 0 {
            player.inventory.add_resource(ResourceType::Wood, wood_gained);
            println!("Added {} wood to inventory. Total wood: {}", wood_gained, player.inventory.get_amount(&ResourceType::Wood));
        }
        if stone_gained > 0 {
            player.inventory.add_resource(ResourceType::Stone, stone_gained);
            println!("Added {} stone to inventory. Total stone: {}", stone_gained, player.inventory.get_amount(&ResourceType::Stone));
        }
        if iron_gained > 0 {
            player.inventory.add_resource(ResourceType::IronOre, iron_gained);
            println!("Added {} iron ore to inventory. Total iron ore: {}", iron_gained, player.inventory.get_amount(&ResourceType::IronOre));
        }
        if coal_gained > 0 {
            player.inventory.add_resource(ResourceType::Coal, coal_gained);
            println!("Added {} coal to inventory. Total coal: {}", coal_gained, player.inventory.get_amount(&ResourceType::Coal));
        }
        
        // Get mouse position BEFORE starting drawing
        let mouse_screen_pos = rl.get_mouse_position();
        let mouse_world_pos = rl.get_screen_to_world2D(mouse_screen_pos, camera);
        let distance = player.position.distance_to(mouse_world_pos);
        
        // Draw
        let mut d = rl.begin_drawing(&thread);
        d.clear_background(Color::new(45, 55, 45, 255));
        
        {
            let mut d2d = d.begin_mode2D(camera);
            draw_world(&mut d2d, &world, &camera);
            player.draw(&mut d2d);
            
            // Draw mining range indicator when player is close to mouse
            if distance <= MINING_RANGE {
                // Draw mining range circle
                d2d.draw_circle_lines_v(player.position, MINING_RANGE, Color::new(255, 255, 0, 100));
                
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
        
        draw_ui(&mut d, &player);
        draw_time_display(&mut d, &world.game_time);
    }
}
