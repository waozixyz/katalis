use raylib::prelude::*;
use crate::types::*;
use crate::player::Player;

pub fn update_camera(
    camera: &mut Camera2D, 
    camera_target: &mut Vector2, 
    player: &Player, 
    rl: &RaylibHandle
) {
    let dt = rl.get_frame_time();
    
    // Smooth camera following (Factorio-style)
    let target_offset = Vector2::zero(); // Can add slight offset here if needed
    let desired_target = player.position + target_offset;
    
    // Smoothly interpolate camera target towards player
    *camera_target = camera_target.lerp(desired_target, CAMERA_SMOOTHNESS * dt);
    camera.target = *camera_target;
    
    // Zoom with mouse wheel
    let wheel = rl.get_mouse_wheel_move();
    if wheel != 0.0 {
        camera.zoom += wheel * 0.15;
        camera.zoom = camera.zoom.clamp(0.5, 3.0);
    }
    
    // Optional: Hold middle mouse or shift for free camera mode
    if rl.is_key_down(KeyboardKey::KEY_LEFT_SHIFT) {
        let pan_speed = 400.0 / camera.zoom;
        
        if rl.is_key_down(KeyboardKey::KEY_I) { camera.target.y -= pan_speed * dt; }
        if rl.is_key_down(KeyboardKey::KEY_K) { camera.target.y += pan_speed * dt; }
        if rl.is_key_down(KeyboardKey::KEY_J) { camera.target.x -= pan_speed * dt; }
        if rl.is_key_down(KeyboardKey::KEY_L) { camera.target.x += pan_speed * dt; }
        
        *camera_target = camera.target;
    }
}
