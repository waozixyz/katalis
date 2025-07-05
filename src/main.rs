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
use render::{draw_world, draw_ui};

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
        rotation: -20.0,  // Factorio-style slight rotation
        zoom: 1.0,        // Adjusted zoom for better view
    };

    let mut camera_target = player.position;

    while !rl.window_should_close() {
        // Update
        player.update(&rl, &camera);
        update_camera(&mut camera, &mut camera_target, &player, &rl);
        handle_input(&mut world, &camera, &rl, &player);
        
        // Draw
        let mut d = rl.begin_drawing(&thread);
        d.clear_background(Color::new(45, 55, 45, 255));
        
        {
            let mut d2d = d.begin_mode2D(camera);
            draw_world(&mut d2d, &world, &camera);  // CHANGED: Pass camera for frustum culling
            player.draw(&mut d2d);
        }
        
        draw_ui(&mut d, &player);
    }
}