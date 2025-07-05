use raylib::prelude::*;
use crate::types::*;
use crate::world::World;
use crate::player::Player;

pub fn handle_input(
    world: &mut World,
    camera: &Camera2D,
    rl: &RaylibHandle,
    player: &Player
) {
    // NEW: Laser shooting with left mouse button
    if rl.is_mouse_button_pressed(MouseButton::MOUSE_BUTTON_LEFT) {
        let mouse_screen_pos = rl.get_mouse_position();
        let mouse_world_pos = rl.get_screen_to_world2D(mouse_screen_pos, *camera);
        
        // Calculate direction from player to mouse
        let direction = Vector2::new(
            mouse_world_pos.x - player.position.x,
            mouse_world_pos.y - player.position.y
        ).normalized();
        
        world.shoot_laser(player.position, direction);
    }
    
    // For future functionality
    if rl.is_key_pressed(KeyboardKey::KEY_R) {
        // Future: Could add resource gathering or other actions
    }
    
    let _ = (camera);
}