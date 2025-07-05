use raylib::prelude::*;
use crate::types::*;

pub struct Player {
    pub position: Vector2,
    pub size: f32,
    pub inventory: Inventory,
}

impl Player {
    pub fn new(x: f32, y: f32) -> Self {
        Self {
            position: Vector2::new(x, y),
            size: 20.0,
            inventory: Inventory::new(),
        }
    }
    
    pub fn update(&mut self, rl: &RaylibHandle, camera: &Camera2D) {
        let mut movement = Vector2::zero();
        
        // Player movement with WASD
        if rl.is_key_down(KeyboardKey::KEY_W) || rl.is_key_down(KeyboardKey::KEY_UP) {
            movement.y -= 1.0;  // Screen "up"
        }
        if rl.is_key_down(KeyboardKey::KEY_S) || rl.is_key_down(KeyboardKey::KEY_DOWN) {
            movement.y += 1.0;  // Screen "down"
        }
        if rl.is_key_down(KeyboardKey::KEY_A) || rl.is_key_down(KeyboardKey::KEY_LEFT) {
            movement.x -= 1.0;  // Screen "left"
        }
        if rl.is_key_down(KeyboardKey::KEY_D) || rl.is_key_down(KeyboardKey::KEY_RIGHT) {
            movement.x += 1.0;  // Screen "right"
        }
        
        // Normalize diagonal movement
        if movement.length() > 0.0 {
            movement = movement.normalized();
            
            // FIXED: Rotate movement vector to counteract camera rotation
            // We need to rotate by the NEGATIVE camera rotation to compensate
            let rotation_rad = -camera.rotation * std::f32::consts::PI / 180.0;
            
            // Rotate the movement vector
            let rotated_movement = Vector2::new(
                movement.x * rotation_rad.cos() - movement.y * rotation_rad.sin(),
                movement.x * rotation_rad.sin() + movement.y * rotation_rad.cos()
            );
            
            self.position = self.position + rotated_movement * PLAYER_SPEED * rl.get_frame_time();
        }
    }
    
    pub fn draw(&self, d: &mut RaylibMode2D<RaylibDrawHandle>) {
        // Player body (darker blue)
        d.draw_circle_v(self.position, self.size, Color::DARKBLUE);
        
        // Player highlight (lighter blue)
        d.draw_circle_v(self.position, self.size - 4.0, Color::SKYBLUE);
        
        // Player center dot
        d.draw_circle_v(self.position, 3.0, Color::WHITE);
        
        // Player direction indicator (small line showing "front")
        let front_pos = Vector2::new(
            self.position.x,
            self.position.y - self.size + 2.0
        );
        d.draw_circle_v(front_pos, 2.0, Color::YELLOW);
    }
}