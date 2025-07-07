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
    // Mining with left mouse button - support both click and hold
    if rl.is_mouse_button_down(MouseButton::MOUSE_BUTTON_LEFT) {
        let mouse_screen_pos = rl.get_mouse_position();
        let mouse_world_pos = rl.get_screen_to_world2D(mouse_screen_pos, *camera);
        
        // Check if target is within mining range
        let distance = player.position.distance_to(mouse_world_pos);
        if distance <= MINING_RANGE {
            // Try to start mining (this will only start if not already mining)
            world.try_start_mining(player, mouse_world_pos);
            
            // Store the target position for continuous mining
            player.set_mining_target(mouse_world_pos);
        }
    }
    
    // Stop continuous mining when mouse button is released
    if rl.is_mouse_button_released(MouseButton::MOUSE_BUTTON_LEFT) {
        player.clear_mining_target();
    }
    
    // Cancel mining immediately if right mouse button is pressed
    if rl.is_mouse_button_pressed(MouseButton::MOUSE_BUTTON_RIGHT) {
        player.current_mining = None;
        player.clear_mining_target();
    }
}