use raylib::prelude::*;
use crate::types::*;

pub struct Player {
    pub position: Vector2,
    pub size: f32,
    pub inventory: Inventory,
    pub facing_angle: f32,  // NEW: Store the angle the player is facing
}

impl Player {
    pub fn new(x: f32, y: f32) -> Self {
        Self {
            position: Vector2::new(x, y),
            size: 20.0,
            inventory: Inventory::new(),
            facing_angle: 0.0,  // NEW: Initialize facing angle
        }
    }
    
    pub fn update(&mut self, rl: &RaylibHandle, camera: &Camera2D) {
        let mut movement = Vector2::zero();
        
        // Player movement with WASD
        if rl.is_key_down(KeyboardKey::KEY_W) || rl.is_key_down(KeyboardKey::KEY_UP) {
            movement.y -= 1.0;
        }
        if rl.is_key_down(KeyboardKey::KEY_S) || rl.is_key_down(KeyboardKey::KEY_DOWN) {
            movement.y += 1.0;
        }
        if rl.is_key_down(KeyboardKey::KEY_A) || rl.is_key_down(KeyboardKey::KEY_LEFT) {
            movement.x -= 1.0;
        }
        if rl.is_key_down(KeyboardKey::KEY_D) || rl.is_key_down(KeyboardKey::KEY_RIGHT) {
            movement.x += 1.0;
        }
        
        // Normalize diagonal movement
        if movement.length() > 0.0 {
            movement = movement.normalized();
            
            // Rotate movement vector to account for camera rotation
            let rotation_rad = -camera.rotation * std::f32::consts::PI / 180.0;
            let rotated_movement = Vector2::new(
                movement.x * rotation_rad.cos() - movement.y * rotation_rad.sin(),
                movement.x * rotation_rad.sin() + movement.y * rotation_rad.cos()
            );
            
            self.position = self.position + rotated_movement * PLAYER_SPEED * rl.get_frame_time();
        }
        
        // NEW: Calculate facing angle based on mouse position
        let mouse_screen_pos = rl.get_mouse_position();
        let mouse_world_pos = rl.get_screen_to_world2D(mouse_screen_pos, *camera);
        
        // Calculate angle from player to mouse
        let direction = Vector2::new(
            mouse_world_pos.x - self.position.x,
            mouse_world_pos.y - self.position.y
        );
        
        self.facing_angle = direction.y.atan2(direction.x);
    }
    
    pub fn draw(&self, d: &mut RaylibMode2D<RaylibDrawHandle>) {
        // Player body (darker blue)
        d.draw_circle_v(self.position, self.size, Color::DARKBLUE);
        
        // Player highlight (lighter blue)
        d.draw_circle_v(self.position, self.size - 4.0, Color::SKYBLUE);
        
        // Player center dot
        d.draw_circle_v(self.position, 3.0, Color::WHITE);
        
        // NEW: Draw player direction indicator pointing toward mouse
        let direction_distance = self.size - 2.0;
        let front_pos = Vector2::new(
            self.position.x + direction_distance * self.facing_angle.cos(),
            self.position.y + direction_distance * self.facing_angle.sin()
        );
        d.draw_circle_v(front_pos, 2.0, Color::YELLOW);
        
        // NEW: Optional - draw a line from center to direction indicator
        d.draw_line_v(self.position, front_pos, Color::new(255, 255, 0, 100));
    }
}