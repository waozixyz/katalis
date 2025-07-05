use raylib::prelude::*;
use crate::types::*;
use crate::world::World;
use crate::player::Player;

pub fn handle_input(
    world: &mut World,
    camera: &Camera2D,
    rl: &RaylibHandle,
    player: &mut Player // Changed to mutable
) {
    // Mining with left mouse button
    if rl.is_mouse_button_pressed(MouseButton::MOUSE_BUTTON_LEFT) {
        let mouse_screen_pos = rl.get_mouse_position();
        let mouse_world_pos = rl.get_screen_to_world2D(mouse_screen_pos, *camera);
        
        // Check if target is within mining range
        let distance = player.position.distance_to(mouse_world_pos);
        if distance <= MINING_RANGE {
            // Try to start mining
            world.try_start_mining(player, mouse_world_pos);
        }
    }
    
    // Cancel mining if right mouse button is pressed
    if rl.is_mouse_button_pressed(MouseButton::MOUSE_BUTTON_RIGHT) {
        player.current_mining = None;
    }
}